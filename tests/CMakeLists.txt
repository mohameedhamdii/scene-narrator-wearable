idf_component_register(
    SRCS
        "test_camera_capture.c"
        "test_vlm_inference.c"
        "test_ocr_engine.c"
        "test_power_states.c"
    INCLUDE_DIRS "."
    REQUIRES unity main
)
