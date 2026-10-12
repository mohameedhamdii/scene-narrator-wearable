# Model Files

Large model binaries are **not committed** to git. Download and place them here before flashing.

## ESP32-P4 (flash to SPIFFS)

| File | Purpose | Source |
|---|---|---|
| `vlm_model.tflite` | MobileNetV2 224 INT8 scene classifier | [TF Hub MobileNet V2](https://www.tensorflow.org/lite/examples/image_classification/overview) |
| `text_detector.tflite` | EAST text detection | [EAST TFLite](https://github.com/oylyttam/EAST) |
| `text_recogniser.tflite` | CRNN text recognition | Export from PaddleOCR-Lite |
| `tts_model.onnx` | Piper voice (optional) | `piper download --model en_US-lessac-low` |

Rename the MobileNet download to `vlm_model.tflite`. The firmware maps top ImageNet classes into scene descriptions.

## Pi Zero 2 W

| File | Purpose | Source |
|---|---|---|
| `mobilevlm-q4.gguf` | MobileVLM-v2 1.7B Q4 | [Hugging Face](https://huggingface.co/models?search=mobilevlm) |
| `mobilevlm-mmproj.gguf` | Vision projector for MobileVLM | Same model repo |
| `en_US-lessac-low.onnx` | Piper TTS voice | `piper download --model en_US-lessac-low` |

Alternatively install `moondream` (see `scripts/setup_pi.sh`) — no GGUF files required.

## Flashing Models to ESP32

```bash
# From project root, with ESP-IDF sourced:
./scripts/flash_models.sh models/

# Or manually:
python $IDF_PATH/components/spiffs/spiffsgen.py 0x300000 models/ build/models.bin
esptool.py write_flash 0x210000 build/models.bin
idf.py flash
```

SPIFFS partition offset is **0x210000** (see `partitions.csv`).

## Fallback Mode

Without model files:

- **ESP32**: hash-based demo captions, OCR skipped, phoneme TTS (robotic speech)
- **Pi**: hash captions if no VLM backend, OCR skipped if no Tesseract, log-only TTS if no Piper/espeak
