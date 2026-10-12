# Assembly Guide — Scene Narrator Wearable

## Form Factor

This prototype uses **Option B — chest-mount badge** for easier wiring and battery space. See `hardware/enclosure_stl/` for 3D print files.

## Bill of Materials

| Component | Part | Qty |
|---|---|---|
| MCU | ESP32-P4 dev board | 1 |
| Camera | OV2640 module (DVP) | 1 |
| Audio | MAX98357A I²S amp + bone-conduction transducer | 1 |
| Battery | 3.7 V 2000 mAh LiPo | 1 |
| Charger | BQ25895 or MCP73871 breakout | 1 |
| Button | Tactile pushbutton | 1 |
| Haptic | 3 V coin vibration motor + N-ch MOSFET | 1 |
| LED | WS2812B RGB | 1 |
| Enclosure | 3D-printed chest badge | 1 |

## Wiring (ESP32-P4)

Refer to the pin map in `plan-03-scene-narrator.md` section 8, or the schematic in `hardware/schematic.pdf`.

Key connections:

- **Camera power gating:** N-channel MOSFET on camera VCC, gate → GPIO 2
- **I²S audio:** BCLK → GPIO 26, LRCLK → GPIO 27, DOUT → GPIO 32
- **Amp shutdown:** GPIO 33 → MAX98357A SD pin
- **Button:** GPIO 34 with internal pull-up
- **Battery ADC:** Voltage divider (100k/100k) → GPIO 35

## Assembly Steps

1. **Print enclosure** — `hardware/enclosure_stl/chest_badge.stl`
2. **Solder power circuit** — LiPo → charge controller → 3.3 V LDO → ESP32-P4
3. **Mount camera** on front face, route DVP ribbon to MCU
4. **Wire I²S amp** and attach bone-conduction pad behind ear
5. **Install button, LED, vibration motor** on GPIO pins per schematic
6. **Flash firmware** (see README)
7. **Load models** to SPIFFS partition (see `models/README.md`)

## Safety

- Use a LiPo bag when charging for the first time
- Verify polarity before connecting battery
- Add a 100 µF cap near the camera VCC for inrush current
