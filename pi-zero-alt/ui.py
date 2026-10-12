import enum
import logging
import time

log = logging.getLogger("ui")

try:
    import RPi.GPIO as GPIO

    HAS_GPIO = True
except ImportError:
    HAS_GPIO= False

PIN_BUTTON = 17


PIN_LED = 27
PIN_VIBRATION = 22

DEBOUNCE_S= 0.05
SHORT_PRESS_S= 0.5

DOUBLE_PRESS_S = 1.0
LONG_PRESS_S =3.0


class ButtonEvent(enum.Enum ):
    NONE = 0
    SHORT_PRESS= 1


    DOUBLE_PRESS= 2
    LONG_PRESS = 3


class UserInterface:
    def __init__(self):
        self._last_press = 0.0
        self._press_count =0


        if HAS_GPIO:
            GPIO.setmode(GPIO.BCM)
            GPIO.setup(PIN_BUTTON,GPIO.IN,pull_up_down=GPIO.PUD_UP)
            GPIO.setup (PIN_LED ,GPIO.OUT)
            GPIO.setup(PIN_VIBRATION,GPIO.OUT )
            GPIO.output(PIN_VIBRATION, GPIO.LOW)
            log.info ("GPIO UI initialized")
        else:
            log.warning("RPi.GPIO not available: auto-trigger simulation")


    def wait_for_event( self , timeout:float =None) ->ButtonEvent :
        if not HAS_GPIO:
            time.sleep ( 1.0)
            return ButtonEvent.SHORT_PRESS

        start = time.time()
        while True:
            if GPIO.input(PIN_BUTTON) == GPIO.LOW:
                press_start = time.time()
                while GPIO.input (PIN_BUTTON) == GPIO.LOW :
                    time.sleep(DEBOUNCE_S)
                duration = time.time ()- press_start

                if duration>= LONG_PRESS_S:
                    self._press_count =0
                    return ButtonEvent.LONG_PRESS

                if duration > SHORT_PRESS_S:
                    continue

                now= time.time ( )
                if self._last_press >0 and ( now -self._last_press)< DOUBLE_PRESS_S:
                    self._press_count += 1
                else:
                    self._press_count= 1

                self._last_press = now

                if self._press_count >= 2:
                    self._press_count = 0
                    return ButtonEvent.DOUBLE_PRESS

                deadline= now +DOUBLE_PRESS_S
                while time.time( )< deadline:
                    if GPIO.input ( PIN_BUTTON)== GPIO.LOW :
                        while GPIO.input ( PIN_BUTTON) ==GPIO.LOW:
                            time.sleep(DEBOUNCE_S)
                        self._press_count=0
                        return ButtonEvent.DOUBLE_PRESS
                    if timeout and ( time.time ()-start)>=timeout:
                        return ButtonEvent.NONE
                    time.sleep(DEBOUNCE_S)



                self._press_count= 0
                return ButtonEvent.SHORT_PRESS

            if timeout and (time.time() - start) >= timeout:
                return ButtonEvent.NONE
            time.sleep (DEBOUNCE_S)

    def set_led(self, state: str):
        log.debug ( "LED -> %s",state )
        if HAS_GPIO :
            on= state in( "green","white" ,"blue_pulse")
            GPIO.output(PIN_LED , GPIO.HIGH if on else GPIO.LOW )

    def haptic_short( self ) :
        self._buzz (0.1)

    def haptic_double ( self):
        self._buzz( 0.1 )
        time.sleep ( 0.08)
        self._buzz (0.1)

    def haptic_long (self) :
        self._buzz ( 0.5)

    def _buzz (self ,duration_s: float):

        if HAS_GPIO:
            GPIO.output(PIN_VIBRATION, GPIO.HIGH)
            time.sleep(duration_s)
            GPIO.output(PIN_VIBRATION, GPIO.LOW)

    def cleanup(self):
        if HAS_GPIO:
            GPIO.cleanup ( )
