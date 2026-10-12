# Scene Narrator Wearable

A battery-powered wearable that captures images, describes the scene with on-device AI, reads visible text, and speaks the narration through bone-conduction audio — built for accessibility with aggressive power management.

> **No cloud required.** All inference and TTS runs locally on ESP32-P4 or Raspberry Pi Zero 2 W.

## Features

- **Real-time scene narration** — vision-language model describes surroundings in 1–2 sentences
- **OCR integration** — reads signs, labels, and menus in the scene
- **Offline TTS** — Piper or phoneme synthesis converts text to speech
- **Power-optimised** — camera/amp power gating, light/deep sleep, ~76 mA average draw
- **Dual platform** — ESP32-P4 firmware (primary) + Pi Zero Python (fallback)
- **Accessible UI** — button triggers, WS2812 RGB LED status, haptic feedback

## Architecture

```
Button → Capture → VLM Caption → OCR → Merge → TTS → Bone-conduction audio
              ↑                                    ↓
         Power Manager ← ← ← ← ← ← ← ← ← Light Sleep
```

## Quick Start (ESP32-P4)

### Prerequisites

- [ESP-IDF 5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32p4/get-started/) with ESP32-P4 support
- OV2640 camera module wired per `docs/assembly_guide.md`

### Build & Flash

```bash
idf.py set-target esp32p4
idf.py build
idf.py -p COM3 flash monitor
```

Simulation mode is **off** by default for real hardware. Enable in `menuconfig → Scene Narrator` for desk testing without hardware.

### Load Models

1. Download models per `models/README.md` into `models/`
2. Flash SPIFFS: `./scripts/flash_models.sh`
3. Enable battery monitoring in menuconfig only if a 2:1 voltage divider is wired to GPIO 35

## Quick Start (Pi Zero 2 W)

```bash
./scripts/setup_pi.sh
cd pi-zero-alt
source .venv/bin/activate
python main.py
```

Set environment variables (or use defaults):

```bash
export VLM_MODEL_PATH=../models/mobilevlm-q4.gguf
export VLM_MMPROJ_PATH=../models/mobilevlm-mmproj.gguf
export PIPER_MODEL=../models/en_US-lessac-low.onnx
```

Moondream is auto-detected if installed — fastest path to a real vision demo on Pi.

## Project Structure

```
├── main/                  # ESP-IDF firmware modules
├── components/            # TFLite runner, Piper/phoneme TTS
├── pi-zero-alt/           # Python implementation for Pi Zero
├── models/                # Quantized model files (not in git)
├── scripts/               # setup_pi.sh, flash_models.sh
├── hardware/              # Schematic, PCB, enclosure STL
├── tests/                 # Unit tests (Unity)
└── docs/                  # Assembly, power budget, model guides
```

## User Interface

| Input | Action |
|---|---|
| Short press | Trigger one narration cycle |
| Double press | Toggle on-demand ↔ continuous mode |
| Long press (3 s) | Enter deep sleep |

| LED | Meaning |
|---|---|
| Green solid | Ready |
| Blue pulse | Processing |
| White solid | Speaking |
| Red blink | Error / low battery |

## Hardware

See `docs/assembly_guide.md` for BOM and wiring. Prototype uses chest-mount badge form factor (Option B).

## Power Budget

~76 mA average → **4–8 hours** practical runtime on 2000 mAh LiPo. Details in `docs/power_budget.md`.

Battery ADC monitoring is **disabled by default** to prevent false shutdowns without a voltage divider.

## Tests

```bash
idf.py -D CONFIG_SCENE_NARRATOR_SIMULATE=y build
```

## License

MIT

## Acknowledgements

Built from [Plan 03 — Real-Time Scene-Narration Wearable](plan-03-scene-narrator.md).
