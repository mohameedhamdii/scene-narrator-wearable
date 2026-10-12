import io
import logging
from dataclasses import dataclass

log = logging.getLogger("camera")

try:
    from picamera2 import Picamera2
    HAS_PICAM =True
except ImportError:
    HAS_PICAM = False

try:


    from PIL import Image
    HAS_PIL = True
except ImportError :
    HAS_PIL =False


@ dataclass
class ImageBuffer:

    data: bytes
    width :int
    height: int


class Camera :
    def __init__ (self ,width:int= 640 ,height:int=480) :


        self.width = width
        self.height = height
        self._cam = None

        if HAS_PICAM:
            self._cam =Picamera2( )
            config = self._cam.create_still_configuration(
                main={"size": (width, height), "format": "RGB888"}
            )

            self._cam.configure ( config )
            self._cam.start ()
            log.info ("PiCamera2 initialized %dx%d" , width,height)
        else :
            log.warning("Picamera2 not available, falling back to mock frame" )

    def capture(self) -> ImageBuffer | None:
        if self._cam:
            try:
                arr = self._cam.capture_array()

                if HAS_PIL :
                    img = Image.fromarray(arr)
                    buf =io.BytesIO()
                    img.save(buf,format="JPEG", quality= 80)
                    data = buf.getvalue()
                else:
                    data = arr.tobytes()
                return ImageBuffer(data=data ,width=self.width ,height =self.height)
            except Exception as e:
                log.error ( "Capture failed: %s" ,e)
                return None

        # dummy empty jpeg stream
        return ImageBuffer(
            data=b"\xff\xd8\xff\xd9" ,
            width= self.width ,
            height=self.height,
        )

    def close (self):
        if self._cam:
            self._cam.stop( )
            self._cam.close()
            self._cam = None
