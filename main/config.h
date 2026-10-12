#pragma once

/* ── Operating modes ─────────────────────────────────────────────── */
#define MODE_ON_DEMAND           0
#define MODE_CONTINUOUS          1
#define CONTINUOUS_INTERVAL_S    15
#define DEEP_SLEEP_TIMEOUT_S     300
#define BATTERY_LOW_MV           3300

/* ── GPIO pin assignments (ESP32-P4) ─────────────────────────────── */
#define PIN_CAMERA_PWR           2
#define PIN_I2S_BCLK             26
#define PIN_I2S_LRCLK            27
#define PIN_I2S_DOUT             32
#define PIN_AMP_SD               33
#define PIN_BUTTON               34
#define PIN_BATTERY_ADC          35
#define PIN_LED_DATA             16
#define PIN_VIBRATION            17

/* ── Camera settings ─────────────────────────────────────────────── */
#define CAMERA_FRAME_WIDTH       640
#define CAMERA_FRAME_HEIGHT      480
#define CAMERA_JPEG_QUALITY      80
#define CAMERA_JPEG_QUALITY_FALLBACK 60
#define CAMERA_STARTUP_MS        100

/* ── Audio settings ──────────────────────────────────────────────── */
#define AUDIO_SAMPLE_RATE        16000
#define AUDIO_BITS_PER_SAMPLE    16
#define AUDIO_DMA_BUF_SAMPLES    512
#define AUDIO_DEFAULT_GAIN       0.85f

/* ── VLM / inference ─────────────────────────────────────────────── */
#define VLM_MAX_TOKENS           30
#define VLM_INPUT_SIZE           224
#define VLM_CAPTION_MAX_LEN      256
#define VLM_MODEL_PATH           "/spiffs/vlm_model.tflite"

/* ── OCR ─────────────────────────────────────────────────────────── */
#define OCR_TEXT_MAX_LEN         128
#define OCR_MAX_REGIONS          4

/* ── TTS ─────────────────────────────────────────────────────────── */
#define TTS_MODEL_PATH           "/spiffs/tts_model.onnx"

/* ── UI timing ───────────────────────────────────────────────────── */
#define UI_DEBOUNCE_MS           50
#define UI_SHORT_PRESS_MS        500
#define UI_DOUBLE_PRESS_MS       1000
#define UI_LONG_PRESS_MS         3000
#define UI_HAPTIC_SHORT_MS       100
#define UI_HAPTIC_LONG_MS        500

/* ── Development / simulation (see Kconfig) ──────────────────────── */
#include "sdkconfig.h"
