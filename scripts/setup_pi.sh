#!/usr/bin/env bash
# Setup Scene Narrator on Raspberry Pi Zero 2 W
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT/pi-zero-alt"

echo "==> System packages"
sudo apt-get update
sudo apt-get install -y python3-venv python3-pip tesseract-ocr espeak-ng alsa-utils

echo "==> Piper TTS (optional, recommended)"
if ! command -v piper >/dev/null 2>&1; then
  echo "Install Piper from https://github.com/rhasspy/piper/releases"
  echo "Then: piper download --model en_US-lessac-low"
fi

echo "==> Python venv"
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

echo "==> Models directory"
mkdir -p "$ROOT/models"
echo "Download models per models/README.md into $ROOT/models"

echo "==> Environment (add to ~/.bashrc)"
cat <<EOF

export VLM_MODEL_PATH=$ROOT/models/mobilevlm-q4.gguf
export VLM_MMPROJ_PATH=$ROOT/models/mobilevlm-mmproj.gguf
export PIPER_MODEL=$ROOT/models/en_US-lessac-low.onnx
EOF

echo "Done. Run: cd pi-zero-alt && source .venv/bin/activate && python main.py"
