idf_component_register(
    SRCS
        "tflite_runner.cpp"
        "jpeg_decode.c"
        "imagenet_labels.c"
    INCLUDE_DIRS "include"
    REQUIRES esp-tflite-micro esp_jpeg
)
