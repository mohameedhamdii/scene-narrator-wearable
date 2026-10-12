#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int x, y, w, h;
    float score;
} tflite_text_box_t;

typedef struct tflite_runner tflite_runner_t;

tflite_runner_t *tflite_runner_create(const uint8_t *model_data, size_t model_size);
void tflite_runner_destroy(tflite_runner_t *runner);

bool tflite_runner_describe_scene(tflite_runner_t *runner,
                                  const uint8_t *jpeg, size_t jpeg_len,
                                  char *caption, size_t caption_len);

bool tflite_runner_detect_text(tflite_runner_t *runner,
                               const uint8_t *jpeg, size_t jpeg_len,
                               tflite_text_box_t *boxes, int max_boxes, int *count);

bool tflite_runner_recognise_text(tflite_runner_t *runner,
                                  const uint8_t *jpeg, size_t jpeg_len,
                                  const tflite_text_box_t *box,
                                  char *text, size_t text_len);
