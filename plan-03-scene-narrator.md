# Plan 03 — Real-Time Scene-Narration Wearable

> **One-liner:** A wearable device that narrates the scene in front of the user in real time — built for accessibility with brutal power-budget constraints.

---

## 1. Project Overview

### 1.1 Goal
Build a compact, battery-powered wearable that captures images from a small camera, runs on-device vision-language inference to generate scene descriptions, and delivers audio narration through bone-conduction speakers — all without cloud connectivity. The system must operate for 4+ hours on a single charge by aggressively managing power states.

### 1.2 Domain
Wearables & Accessibility

### 1.3 Portfolio Value
Combines assistive-technology impact with ultra-low-power embedded design. The power-budget engineering alone (sleep states, duty cycling, model optimisation) is a standout interview story. Running a vision-language model on constrained hardware demonstrates advanced edge AI skills.

---

## 2. Hardware Bill of Materials

| Component | Specific Part / Model | Qty | Purpose |
|---|---|---|---|
| Microcontroller / SBC | ESP32-P4 (primary) or Pi Zero 2 W (fallback) | 1 | Main processor — runs inference, controls peripherals |
| Camera module | OV2640 or OV5640 (SPI/DVP interface) | 1 | Captures scene images |
| Audio output | AfterShokz-style bone-conduction transducer + MAX98357A I²S amp | 1 | Non-occluding audio narration |
| Battery | 3.7 V 2000 mAh LiPo | 1 | Runtime target: 4+ hours |
| Power management | BQ25895 or MCP73871 charge controller + 3.3 V LDO | 1 | Charge management, voltage regulation |
| Button | Tactile pushbutton | 1 | Trigger narration on-demand |
| Vibration motor | 3 V coin vibration motor | 1 | Haptic feedback for confirmation |
| LED | Single RGB LED (WS2812B) | 1 | Status indicator |
| Enclosure | 3D-printed glasses-clip or chest-mount case | 1 | Wearable form factor |
| MicroSD card (if Pi Zero) | 32 GB A1 | 1 | OS + model storage |
| USB-C breakout | USB-C female connector | 1 | Charging port |

### 2.1 Form Factor Design
- **Option A — Glasses clip:** Camera on the bridge, processor + battery in a temple-arm housing, bone-conduction transducer presses against the skull behind the ear.
- **Option B — Chest-mount badge:** Camera faces forward from a lanyard badge, processor and battery inside the badge, wired bone-conduction earpiece.
- Choose Option B for prototyping (easier wiring, more battery space); migrate to Option A for a polished demo.

### 2.2 Power Budget Analysis

| Component | Active Current | Duty Cycle | Avg Current |
|---|---|---|---|
| ESP32-P4 (inference) | 350 mA | 15% (2 s every 13 s) | 52.5 mA |
| ESP32-P4 (light sleep) | 5 mA | 80% | 4.0 mA |
| ESP32-P4 (deep sleep) | 10 µA | 5% | ~0 mA |
| Camera (active) | 60 mA | 15% | 9.0 mA |
| Camera (power-gated off) | 0 mA | 85% | 0 mA |
| Audio amp (speaking) | 100 mA | 10% | 10.0 mA |
| Audio amp (shutdown pin) | 1 µA | 90% | ~0 mA |
| **Total average** | — | — | **~76 mA** |
| **Battery life (2000 mAh)** | — | — | **~26 hours** |

*Note: Real-world will be lower due to WiFi/BLE for config, voltage regulator losses, and model loading overhead. Target 4–8 hours is achievable.*

---

## 3. Software Stack & Dependencies

| Layer | Technology | Notes |
|---|---|---|
| Firmware platform (ESP32-P4) | ESP-IDF 5.x | FreeRTOS-based, C primary language |
| Firmware platform (Pi Zero alt.) | Raspberry Pi OS Lite + Python | Fallback option |
| Vision-language model | MobileVLM-v2-1.7B (quantized INT4) or nanoLLaVA | Compact VLM that can describe scenes |
| Image captioning fallback | BLIP-2 ViT-B (quantized) or custom CNN+LSTM captioner | If full VLM too large for ESP32-P4 |
| OCR engine | PaddleOCR-Lite or Tesseract-Lite | Read text in scene (signs, labels) |
| Text-to-speech | Piper TTS (VITS-based, offline) | Converts captions to audio |
| Audio codec | WAV (16-bit, 16 kHz mono) | Simple, no decoding overhead |
| Power management | ESP-IDF power management APIs | Light sleep, deep sleep, peripheral power gating |
| Programming language | C (ESP-IDF) or Python (Pi Zero) | — |

---

## 4. System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     WEARABLE DEVICE                         │
│                                                             │
│  ┌──────────┐   Button press or timer wakeup                │
│  │  BUTTON  │──────────────────┐                            │
│  └──────────┘                  │                            │
│                                ▼                            │
│  ┌──────────────────────────────────────────┐               │
│  │         MAIN CONTROL LOOP (MCU)          │               │
│  │                                          │               │
│  │  1. Wake from sleep                      │               │
│  │  2. Power ON camera                      │               │
│  │  3. Capture single frame (JPEG)          │               │
│  │  4. Power OFF camera                     │               │
│  │  5. Run VLM / captioning inference       │               │
│  │  6. Run OCR on detected text regions     │               │
│  │  7. Merge caption + OCR text             │               │
│  │  8. Run TTS on merged text               │               │
│  │  9. Play audio via bone-conduction       │               │
│  │ 10. Enter light/deep sleep               │               │
│  └──────────────────────────────────────────┘               │
│       │              │              │                        │
│       ▼              ▼              ▼                        │
│  ┌────────┐   ┌───────────┐   ┌──────────────┐             │
│  │ Camera │   │ VLM/OCR   │   │ Bone-conduct │             │
│  │ OV2640 │   │ Inference │   │   Speaker    │             │
│  └────────┘   └───────────┘   └──────────────┘             │
└─────────────────────────────────────────────────────────────┘
```

---

## 5. Detailed Module Breakdown

### 5.1 Module A — Power Manager (`power_manager.c`)

**Responsibility:** Manage all power states and peripheral power gating to maximise battery life.

**Implementation details:**
1. **Power states:**
   - **ACTIVE:** All systems on, inference running. Max 2–3 seconds per cycle.
   - **LIGHT_SLEEP:** CPU halted, RAM retained, peripherals off. GPIO interrupt (button) or timer wakes.
   - **DEEP_SLEEP:** Only RTC running. Used during extended idle (> 60 s without button press).
2. **Peripheral power gating:**
   - Camera: controlled via a GPIO-switched MOSFET on the camera's VCC line. Power ON only during capture (< 500 ms).
   - Audio amp: use the MAX98357A's `SD` (shutdown) pin — pull LOW to disable, HIGH to enable.
   - Vibration motor: GPIO-controlled via N-channel MOSFET.
3. **Duty cycle manager:**
   - **On-demand mode (default):** Device stays in LIGHT_SLEEP. Button press triggers one capture-infer-speak cycle, then returns to LIGHT_SLEEP.
   - **Continuous mode:** Timer wakes the device every N seconds (configurable, default 15 s), captures and narrates, returns to LIGHT_SLEEP.
   - **Deep sleep timeout:** If no button press for 5 minutes, enter DEEP_SLEEP. Long-press button (3 s) to wake.
4. **Battery monitoring:**
   - Read battery voltage via ADC (voltage divider on LiPo output).
   - If voltage < 3.3 V, announce "battery low" via TTS and enter DEEP_SLEEP.

**Configuration (`config.h` defines):**
```c
#define MODE_ON_DEMAND      0
#define MODE_CONTINUOUS     1
#define CONTINUOUS_INTERVAL_S  15
#define DEEP_SLEEP_TIMEOUT_S   300
#define BATTERY_LOW_MV         3300
```

**Files to create:**
- `main/power_manager.h`
- `main/power_manager.c`

---

### 5.2 Module B — Camera Capture (`camera_capture.c`)

**Responsibility:** Capture a single JPEG frame from the camera module.

**Implementation details:**
1. Power ON camera via GPIO (MOSFET gate HIGH).
2. Wait 100 ms for camera startup stabilisation.
3. Initialise camera over SPI/DVP using ESP-IDF camera driver (or `esp32-camera` component).
4. Capture a single frame at 640 × 480 JPEG quality 80.
5. Store the JPEG buffer in PSRAM (ESP32-P4 has external PSRAM).
6. Deinitialise camera driver, power OFF camera via GPIO.
7. Total capture time target: < 500 ms.

**Edge cases:**
- If camera init fails, retry once. If still failing, play error beep and return to sleep.
- If PSRAM allocation fails (frame too large), reduce JPEG quality to 60 and retry.

**Files to create:**
- `main/camera_capture.h`
- `main/camera_capture.c`

---

### 5.3 Module C — Vision-Language Inference (`vlm_inference.c`)

**Responsibility:** Generate a natural-language description of the captured image.

**Implementation details:**
1. **Model selection decision tree:**
   - **ESP32-P4 (PSRAM ≥ 8 MB):** Use a heavily quantized compact VLM. Options:
     - nanoLLaVA (INT4, ~500 MB) — if PSRAM is sufficient.
     - Custom CNN (MobileNetV3) + LSTM captioner trained on COCO Captions — fallback, ~20 MB.
   - **Pi Zero 2 W (512 MB RAM):** Use MobileVLM-v2-1.7B (INT4 GGUF) via llama.cpp.
2. **Inference pipeline on ESP32-P4:**
   - Load quantized model weights from SPI flash or SD card into PSRAM.
   - Preprocess JPEG: decode to RGB, resize to model input size (e.g., 224 × 224).
   - Run vision encoder: extract image features (CNN forward pass).
   - Run language decoder: generate caption tokens autoregressively (max 30 tokens).
   - Decode tokens to text string.
3. **Inference pipeline on Pi Zero 2 W:**
   - Use `llama-cpp-python` with multimodal support.
   - Load model once at boot, keep in RAM.
   - Send image + prompt "Describe this scene briefly for a visually impaired person."
   - Capture generated text (max 50 tokens).
4. **Prompt engineering:**
   - System prompt: "You are a scene narrator for a visually impaired person. Describe the scene in 1–2 sentences. Focus on: people, obstacles, text/signs, key objects, and spatial layout. Be concise."
5. **Target inference time:** < 2 seconds on ESP32-P4, < 5 seconds on Pi Zero.

**Files to create:**
- `main/vlm_inference.h`
- `main/vlm_inference.c`
- `main/model_loader.c` (loads weights from flash/SD)
- `models/` directory (store quantized model files)

---

### 5.4 Module D — OCR Engine (`ocr_engine.c`)

**Responsibility:** Detect and read text in the captured image (signs, labels, menus).

**Implementation details:**
1. **Text detection:**
   - Use EAST text detector (quantized, ~5 MB) or a simple edge-based text region proposal.
   - Identify rectangular text regions in the image.
2. **Text recognition:**
   - Run a lightweight CRNN (CNN + RNN) character recognition model on each detected text region.
   - Use a pre-trained English character set model.
   - Alternative: on Pi Zero, use Tesseract OCR (heavier but more accurate).
3. **Integration with VLM output:**
   - If OCR detects text, append it to the VLM caption: "The scene shows a street corner with a café. Sign reads: 'OPEN 9AM–5PM'."
4. **Skip OCR if no text detected** (saves power — text detection is a lightweight gate).

**Files to create:**
- `main/ocr_engine.h`
- `main/ocr_engine.c`
- `models/text_detector.tflite`
- `models/text_recogniser.tflite`

---

### 5.5 Module E — Text-to-Speech Engine (`tts_engine.c`)

**Responsibility:** Convert the generated text caption into audible speech.

**Implementation details:**
1. **TTS model:** Piper TTS (VITS-based neural TTS, runs offline).
   - Use the "en_US-lessac-low" voice model (~15 MB).
   - Generates 16 kHz 16-bit mono PCM audio.
2. **On ESP32-P4:**
   - Port Piper's inference to run on ESP-IDF (may require a C reimplementation of the VITS decoder).
   - Alternative: use eSpeak-NG (formant-based, very lightweight, ~2 MB, lower quality but guaranteed to run on MCU).
3. **On Pi Zero:**
   - Run Piper via command line: `echo "text" | piper --model en_US-lessac-low --output_raw | aplay -r 16000 -f S16_LE`.
4. **Audio playback:**
   - Stream PCM samples to the MAX98357A I²S amplifier.
   - ESP-IDF I²S driver configured for 16 kHz, 16-bit, mono.
   - Buffer: double-buffered DMA, 512-sample buffers for glitch-free playback.
5. **Speech rate:** Normal speed (1.0×). Configurable via a compile-time or runtime setting.
6. **Volume control:** Adjust I²S output amplitude in software (multiply samples by a gain factor 0.0–1.0).

**Files to create:**
- `main/tts_engine.h`
- `main/tts_engine.c`
- `main/audio_player.h`
- `main/audio_player.c` (I²S driver wrapper)
- `models/tts_model.bin` (Piper voice model)

---

### 5.6 Module F — User Interface (`ui.c`)

**Responsibility:** Handle button input, LED feedback, and haptic feedback.

**Implementation details:**
1. **Button handling:**
   - Short press (< 500 ms): trigger one narration cycle.
   - Double press (two presses within 1 s): switch between on-demand and continuous mode.
   - Long press (3 s): power on/off (enter/exit deep sleep).
   - Debounce: 50 ms software debounce.
2. **LED feedback (RGB LED):**
   - Solid green: ready (light sleep, waiting for button).
   - Pulsing blue: processing (capturing / inferring).
   - Solid white: speaking.
   - Red blink: error or low battery.
   - Off: deep sleep.
3. **Haptic feedback:**
   - Single short buzz (100 ms): button press acknowledged.
   - Double buzz: mode switched.
   - Long buzz (500 ms): entering deep sleep.

**Files to create:**
- `main/ui.h`
- `main/ui.c`

---

## 6. Directory Structure

```
scene-narrator-wearable/
├── README.md
├── CMakeLists.txt                     # ESP-IDF project root
├── sdkconfig.defaults                 # ESP-IDF configuration
├── main/
│   ├── CMakeLists.txt
│   ├── main.c                         # Entry point — FreeRTOS task setup
│   ├── power_manager.h / .c
│   ├── camera_capture.h / .c
│   ├── vlm_inference.h / .c
│   ├── model_loader.h / .c
│   ├── ocr_engine.h / .c
│   ├── tts_engine.h / .c
│   ├── audio_player.h / .c
│   ├── ui.h / .c
│   └── config.h                       # All compile-time config defines
├── components/
│   ├── esp32-camera/                  # Camera driver component
│   ├── tflite-micro/                  # TensorFlow Lite Micro for CNN models
│   └── piper-lite/                    # Lightweight TTS engine
├── models/
│   ├── vlm_model.bin                  # Quantized VLM weights
│   ├── text_detector.tflite
│   ├── text_recogniser.tflite
│   └── tts_model.bin
├── hardware/
│   ├── schematic.pdf                  # KiCad schematic
│   ├── pcb_layout.pdf
│   └── enclosure_stl/                 # 3D print files
│       ├── chest_badge.stl
│       └── glasses_clip.stl
├── pi-zero-alt/                       # Alternative Pi Zero implementation
│   ├── main.py
│   ├── camera.py
│   ├── inference.py
│   ├── tts.py
│   └── requirements.txt
├── tests/
│   ├── test_camera_capture.c
│   ├── test_vlm_inference.c
│   ├── test_ocr_engine.c
│   └── test_power_states.c
└── docs/
    ├── assembly_guide.md
    ├── power_budget.md
    └── model_selection.md
```

---

## 7. Main Control Loop (`main.c`) — Pseudocode

```c
void app_main(void) {
    // Initialise all drivers
    power_manager_init();
    ui_init();            // button, LED, haptic
    audio_player_init();  // I²S
    
    // Load models into PSRAM (one-time)
    model_loader_init("models/vlm_model.bin");
    
    // Set LED to green (ready)
    ui_set_led(LED_GREEN_SOLID);
    
    while (1) {
        // Wait for trigger (button press or timer)
        trigger_t trigger = power_manager_wait_for_trigger();
        
        if (trigger == TRIGGER_SHUTDOWN) {
            ui_haptic(HAPTIC_LONG);
            power_manager_deep_sleep();
            // execution stops here until hardware reset
        }
        
        // Acknowledge button press
        ui_haptic(HAPTIC_SHORT);
        ui_set_led(LED_BLUE_PULSE);
        
        // Step 1: Capture image
        image_buffer_t* img = camera_capture();
        if (img == NULL) {
            ui_set_led(LED_RED_BLINK);
            tts_speak("Camera error");
            continue;
        }
        
        // Step 2: Run VLM inference
        char caption[256];
        vlm_inference_describe(img, caption, sizeof(caption));
        
        // Step 3: Run OCR
        char ocr_text[128];
        ocr_engine_read(img, ocr_text, sizeof(ocr_text));
        
        // Step 4: Merge caption and OCR
        char full_text[384];
        if (strlen(ocr_text) > 0) {
            snprintf(full_text, sizeof(full_text), "%s. Text reads: %s", caption, ocr_text);
        } else {
            strncpy(full_text, caption, sizeof(full_text));
        }
        
        // Step 5: Speak
        ui_set_led(LED_WHITE_SOLID);
        tts_speak(full_text);
        
        // Step 6: Return to ready state
        camera_free(img);
        ui_set_led(LED_GREEN_SOLID);
        
        // Enter light sleep until next trigger
        power_manager_light_sleep();
    }
}
```

---

## 8. Hardware Schematic — Key Connections

```
ESP32-P4 Pin Assignments:
─────────────────────────
GPIO  2  → Camera PWDN (via MOSFET gate — power gating)
GPIO  4  → Camera D0 (DVP data bus)
GPIO  5  → Camera D1
GPIO 12  → Camera D2
GPIO 13  → Camera D3
GPIO 14  → Camera D4
GPIO 15  → Camera D5
GPIO 18  → Camera XCLK
GPIO 19  → Camera PCLK
GPIO 21  → Camera SDA (I²C config)
GPIO 22  → Camera SCL (I²C config)
GPIO 23  → Camera VSYNC
GPIO 25  → Camera HREF

GPIO 26  → I²S BCLK (to MAX98357A)
GPIO 27  → I²S LRCLK (to MAX98357A)
GPIO 32  → I²S DOUT (to MAX98357A)
GPIO 33  → MAX98357A SD (shutdown pin)

GPIO 34  → Tactile button (input, pull-up)
GPIO 35  → Battery voltage ADC (via voltage divider)
GPIO 16  → WS2812B data (RGB LED)
GPIO 17  → Vibration motor (via N-ch MOSFET)

3V3      → Camera VCC (through MOSFET), LED VCC
GND      → Common ground
VSPI     → SD card (if used for model storage)
```

---

## 9. Model Optimisation Strategy

### 9.1 For ESP32-P4 (limited RAM/flash)
1. **Quantization:** INT4 or INT8 quantization of all models using TensorFlow Lite post-training quantization.
2. **Model pruning:** Remove 30–50% of weights with minimal accuracy loss.
3. **Knowledge distillation:** Train a tiny student model from a larger teacher VLM.
4. **Progressive loading:** If model doesn't fit in PSRAM entirely, load encoder and decoder in stages, processing sequentially.
5. **Token limit:** Cap generated caption at 20 tokens to reduce decoder runtime.

### 9.2 For Pi Zero 2 W (512 MB RAM)
1. Use GGUF Q4_K_M quantization for the VLM (via llama.cpp).
2. Model size target: < 400 MB to leave headroom for OS and inference buffers.
3. Use `mmap` for model loading to reduce memory pressure.

---

## 10. Testing Strategy

| Test | Type | What it validates |
|---|---|---|
| `test_camera_capture.c` | Unit | Camera powers on, captures valid JPEG, powers off correctly |
| `test_vlm_inference.c` | Unit | Given test images, produces reasonable captions |
| `test_ocr_engine.c` | Unit | Given images with text, OCR output matches expected strings |
| `test_power_states.c` | Integration | Power state transitions correct; current draw measured with multimeter matches budget |
| Battery life test | System | Run continuous mode for 4+ hours, log battery voltage over time |
| Field test | System | Wear device outdoors, verify narrations are useful and timely |

---

## 11. Stretch Goals

- **Scene change detection:** Only narrate when the scene changes significantly (frame differencing / perceptual hash comparison) — saves even more power.
- **Object distance estimation:** Use monocular depth estimation to warn "obstacle 2 metres ahead."
- **Multi-language TTS:** Switch narration language via button combo.
- **BLE companion app:** Configure settings (mode, interval, volume, language) from a phone app.
- **Hazard alerts:** Priority-narrate dangerous situations (vehicles, stairs, obstacles) even in on-demand mode.

---

## 12. Risks & Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| VLM too large for ESP32-P4 PSRAM | Cannot run on target MCU | Use CNN+LSTM captioner as fallback; pivot to Pi Zero |
| TTS quality too low with eSpeak | Poor user experience | Use Piper TTS; accept larger model size |
| Camera motion blur (user walking) | Bad captures | Use high shutter speed; add "hold still" haptic feedback |
| Audio inaudible outdoors | Narration missed | Use bone-conduction at higher volume; add volume control |
| Battery life below 4 hours | Impractical for real use | Reduce duty cycle; use larger battery (3000 mAh) |

---

## 13. Estimated Timeline

| Phase | Duration | Deliverable |
|---|---|---|
| Hardware assembly & power circuit | 3 days | Wearable prototype with camera, audio, battery |
| Camera capture & power gating | 2 days | Camera captures JPEG, powers off correctly |
| VLM model selection & quantization | 4 days | Model running on target hardware |
| OCR integration | 2 days | Text detection + recognition working |
| TTS integration & audio playback | 2 days | Spoken narration from text |
| Power management & sleep states | 3 days | Full duty cycling, measured battery life |
| UI (button, LED, haptic) | 1 day | All user interactions working |
| End-to-end integration & field testing | 3 days | Reliable wearable demo |
| **Total** | **~20 days** | — |
