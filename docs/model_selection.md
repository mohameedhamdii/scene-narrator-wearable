# Model Selection Guide

## ESP32-P4 (Primary Target)

| Task | Model | Size | Notes |
|---|---|---|---|
| Scene caption | MobileNetV2 224 INT8 (`vlm_model.tflite`) | ~3 MB | Real TFLite inference; top classes → scene sentence |
| Text detection | EAST (TFLite INT8) | ~5 MB | Lightweight gate before OCR |
| Text recognition | CRNN (TFLite INT8) | ~3 MB | English character set |
| TTS | Piper ONNX (optional) | ~15 MB | Phoneme synth used until ONNX runtime linked |

### Flash Layout

Store models on SPIFFS:

```
/spiffs/vlm_model.tflite
/spiffs/text_detector.tflite
/spiffs/text_recogniser.tflite
/spiffs/tts_model.onnx
```

Flash with `./scripts/flash_models.sh`.

## Raspberry Pi Zero 2 W (Fallback)

| Task | Model | Size | Runtime |
|---|---|---|---|
| Scene caption | Moondream2 (pip) | ~1.6 GB download | `moondream` package (recommended) |
| Scene caption (alt) | MobileVLM-v2-1.7B Q4 + mmproj | ~400 MB | llama.cpp multimodal |
| OCR | Tesseract 4 + LSTM | system pkg | pytesseract |
| TTS | Piper en_US-lessac-low | ~15 MB | piper CLI |

```bash
export VLM_MODEL_PATH=models/mobilevlm-q4.gguf
export VLM_MMPROJ_PATH=models/mobilevlm-mmproj.gguf
export PIPER_MODEL=models/en_US-lessac-low.onnx
```

Or run `./scripts/setup_pi.sh` for automated setup.

## Download Instructions

See `models/README.md` for download links and placement.
