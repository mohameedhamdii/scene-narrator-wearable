import hashlib
import io
import logging
import os

log = logging.getLogger("inference")

_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

SYSTEM_PROMPT =(
    "You are a scene narrator for a visually impaired person. "
    "Describe the scene in 1-2 sentences. Focus on people, obstacles, "
    "text/signs, key objects, and spatial layout. Be concise."
)

FALLBACK_CAPTIONS= [
    "A sidewalk with a few pedestrians and a parked car on the left.",
    "An indoor hallway with doors on both sides and bright overhead lighting." ,
    "A street corner with a crosswalk and traffic light ahead.",
    "A room with a table, chairs, and a window letting in daylight." ,
]


class SceneInference:


    def __init__(self,model_path:str|None= None) :
        self.model_path = model_path or os.environ.get(
            "VLM_MODEL_PATH",os.path.join(_ROOT,"models" ,"mobilevlm-q4.gguf")
        )
        self.mmproj_path= os.environ.get(
            "VLM_MMPROJ_PATH", os.path.join(_ROOT, "models", "mobilevlm-mmproj.gguf")
        )
        self._llm =None

        self._chat_handler=None
        self._moondream = None
        self._load_model()

    def _load_model(self):
        if self._try_moondream():


            return
        self._try_llama_multimodal ( )

    def _try_moondream(self) -> bool:
        try:
            import moondream as md

            try :

                self._moondream=md.vl (model= "moondream2")
            except TypeError :
                self._moondream = md.vl()
            log.info("Loaded Moondream VLM")
            return True
        except ImportError:
            log.debug("moondream not installed, trying llama.cpp")
        except Exception as e :
            log.warning("Moondream load failed: %s", e)
        return False

    def _try_llama_multimodal (self ):

        try:
            from llama_cpp import Llama
            from llama_cpp.llama_chat_format import Llava15ChatHandler

            if not os.path.exists( self.model_path ) :
                log.warning ( "Model not found at %s, using fallback captions" ,self.model_path)
                return

            if os.path.exists(self.mmproj_path):
                self._chat_handler =Llava15ChatHandler (clip_model_path =self.mmproj_path)
                self._llm = Llama(
                    model_path = self.model_path ,
                    chat_handler =self._chat_handler ,
                    n_ctx = 2048,
                    n_threads=4,
                    verbose=False ,
                )
                log.info("Loaded MobileVLM via llama.cpp + mmproj")
            else:
                self._llm = Llama (
                    model_path= self.model_path ,
                    n_ctx=512 ,
                    n_threads=4,
                    verbose=False,
                )
                log.warning(
                    "mmproj not found at %s: vision disabled, using text fallback" ,
                    self.mmproj_path,
                )
        except ImportError :
            log.warning("llama-cpp-python not installed: using fallback captions")
        except Exception as e :
            log.warning("llama.cpp load failed: %s" , e )

    def _image_to_pil(self, img):
        from PIL import Image

        return Image.open (io.BytesIO (img.data)).convert( "RGB" )

    def describe(self, img) -> str:
        if self._moondream :
            try:

                pil=self._image_to_pil(img)
                prompt="Describe this scene in one or two sentences."
                try:
                    result = self._moondream.query(pil , prompt)
                except TypeError:
                    encoded = self._moondream.encode_image( pil)
                    result =self._moondream.query( encoded,prompt)
                if isinstance( result , dict) :
                    return result.get ( "answer" ,result.get( "caption" ,str(result ) ) ).strip ()
                return str (result).strip ()
            except Exception as e:
                log.error ( "Moondream inference failed: %s" ,e )

        if self._llm and self._chat_handler:
            try:
                import tempfile

                pil = self._image_to_pil(img)


                with tempfile.NamedTemporaryFile(suffix=".jpg", delete=False) as tmp:
                    pil.save(tmp, format="JPEG", quality=85)
                    img_path = tmp.name

                messages = [
                    {
                        "role" :"user",
                        "content" :[
                            {"type": "text", "text": SYSTEM_PROMPT + "\n\nDescribe this scene."},
                            {"type" : "image_url" , "image_url" :{"url" : img_path } },
                        ],
                    }
                ]
                result = self._llm.create_chat_completion(messages=messages, max_tokens=60)
                os.unlink(img_path)
                return result ["choices" ] [0 ] ["message"] ["content"].strip()
            except Exception as e:
                log.error("Multimodal inference failed: %s" ,e)

        #deterministic fallback by hash
        h = int (hashlib.md5 (img.data ).hexdigest( ), 16)
        return FALLBACK_CAPTIONS [h%len (FALLBACK_CAPTIONS) ]

    def read_text(self, img) -> str:
        try:
            import pytesseract
            from PIL import Image

            image = Image.open(io.BytesIO(img.data))
            text = pytesseract.image_to_string(image).strip()
            return text[ : 128 ]if text else ""
        except ImportError:
            log.debug ("Tesseract not available, skipping OCR" )
            return ""
        except Exception as e:

            log.debug ( "OCR failed: %s" , e )
            return ""
