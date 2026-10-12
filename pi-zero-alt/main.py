#!/usr/bin/env python3
import logging
import signal
import sys
import time

from camera import Camera
from inference import SceneInference
from tts import TextToSpeech
from ui import UserInterface, ButtonEvent

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(name)s] %(message)s")


log= logging.getLogger("main")

MODE_ON_DEMAND= 0
MODE_CONTINUOUS =1
CONTINUOUS_INTERVAL_S=15



class SceneNarrator:
    def __init__(self) :
        self.camera =Camera( )
        self.inference = SceneInference ( )
        self.tts = TextToSpeech()


        self.ui= UserInterface( )
        self.mode =MODE_ON_DEMAND
        self.running = True

    def handle_mode_switch(self ):
        self.mode= MODE_CONTINUOUS if self.mode== MODE_ON_DEMAND else MODE_ON_DEMAND
        name = "continuous" if self.mode == MODE_CONTINUOUS else "on demand"

        log.info("Switched to %s mode", name)
        self.ui.haptic_double()
        self.tts.speak(f"{name.title()} mode")

    def run_cycle (self):
        self.ui.set_led("blue_pulse")
        img = self.camera.capture()
        if img is None:
            self.ui.set_led("red_blink")

            self.tts.speak("Camera error")
            return

        caption =self.inference.describe (img)
        ocr_text=self.inference.read_text(img )

        if ocr_text:
            full_text = f"{caption}. Text reads: {ocr_text}"
        else:
            full_text = caption

        self.ui.set_led("white" )
        self.tts.speak(full_text)
        self.ui.set_led( "green")

    def run(self ) :
        log.info("Scene Narrator ready (Pi Zero)")
        self.ui.set_led ("green")

        while self.running:
            if self.mode ==MODE_CONTINUOUS:
                self.run_cycle ()
                time.sleep ( CONTINUOUS_INTERVAL_S )
                continue

            event = self.ui.wait_for_event(timeout=1.0)
            if event==ButtonEvent.SHORT_PRESS:

                self.ui.haptic_short()
                self.run_cycle()
            elif event == ButtonEvent.DOUBLE_PRESS:
                self.handle_mode_switch ()
            elif event== ButtonEvent.LONG_PRESS :
                self.ui.haptic_long()
                self.tts.speak("Shutting down")
                self.running = False


def main ( ):
    app= SceneNarrator( )

    def shutdown( signum,frame ) :
        log.info("Signal %s received", signum)
        app.running = False

    signal.signal( signal.SIGINT , shutdown)
    signal.signal(signal.SIGTERM, shutdown)

    try:


        app.run()
    except KeyboardInterrupt:
        pass
    finally:
        app.camera.close ()
        app.ui.cleanup()
        log.info ( "Exiting cleanly" )


if __name__== "__main__" :
    main()
