import logging
import os
import shutil
import subprocess
import tempfile

log = logging.getLogger("tts")

_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
PIPER_MODEL= os.environ.get(
    "PIPER_MODEL",os.path.join(_ROOT ,"models", "en_US-lessac-low.onnx" )
)


class TextToSpeech:


    def __init__ ( self) :
        self.piper_bin = shutil.which("piper")
        self.aplay_bin= shutil.which ("aplay" )
        self.espeak_bin =shutil.which ("espeak-ng")or shutil.which( "espeak" )


        if self.piper_bin and os.path.exists(PIPER_MODEL):
            log.info("Using Piper TTS (%s)", PIPER_MODEL)
        elif self.espeak_bin:
            log.info( "Using espeak fallback" )
        else:
            log.warning("No TTS engine found: will log text only" )

    def speak ( self , text: str) :


        if not text:
            return

        if self.piper_bin and os.path.exists(PIPER_MODEL) and self.aplay_bin:
            self._speak_piper(text)
        elif self.espeak_bin:
            self._speak_espeak (text)
        else:
            log.info("[TTS] %s", text)


    def _speak_piper( self,text :str ) :
        try:
            proc=subprocess.Popen(
                [self.piper_bin,"--model",PIPER_MODEL, "--output_raw"] ,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
            )
            audio, _ = proc.communicate(input=text.encode("utf-8"))
            if audio and self.aplay_bin:
                play = subprocess.Popen(
                    [self.aplay_bin, "-r","16000", "-f", "S16_LE","-t", "raw" ] ,
                    stdin=subprocess.PIPE,
                )
                play.communicate(input =audio)
        except Exception as e :
            log.error("Piper TTS failed: %s", e)

            self._speak_espeak( text )

    def _speak_espeak(self, text: str):
        try :
            subprocess.run([self.espeak_bin,text ] ,check =True)
        except Exception as e :
            log.error("espeak failed: %s", e)
