#!/usr/bin/env bash
# Build SPIFFS image from models/ and flash to ESP32
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODELS_DIR="${1:-$ROOT/models}"
BUILD_DIR="$ROOT/build"
PART_SIZE=0x300000
PART_OFFSET=0x210000

if [ -z "${IDF_PATH:-}" ]; then
  echo "Source ESP-IDF first: . \$IDF_PATH/export.sh"
  exit 1
fi

mkdir -p "$MODELS_DIR" "$BUILD_DIR"

echo "==> Expected files in $MODELS_DIR:"
echo "  vlm_model.tflite       (MobileNetV2 224 INT8 for scene labels)"
echo "  text_detector.tflite   (EAST text detector)"
echo "  text_recogniser.tflite (CRNN recogniser)"
echo "  tts_model.onnx         (Piper voice, optional)"

python "$IDF_PATH/components/spiffs/spiffsgen.py" \
  "$PART_SIZE" "$MODELS_DIR" "$BUILD_DIR/models.bin"

echo "==> Flashing SPIFFS to offset $PART_OFFSET"
esptool.py write_flash "$PART_OFFSET" "$BUILD_DIR/models.bin"

echo "Done."
