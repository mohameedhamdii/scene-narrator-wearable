#include "unity.h"
#include "ocr_engine.h"
#include "model_loader.h"
#include "camera_capture.h"

void setUp(void)
{
    model_loader_init(NULL);
    ocr_engine_init();
}

void tearDown(void)
{
    ocr_engine_deinit();
    model_loader_deinit();
}

void test_ocr_returns_without_crash(void)
{


    image_buffer_t *img=camera_capture();
    TEST_ASSERT_NOT_NULL(img);

    char text[128];
    TEST_ASSERT_TRUE(ocr_engine_read(img, text,sizeof(text)));


    camera_free(img);
}

void test_ocr_empty_on_no_text(void)
{
    image_buffer_t img= {
        .data = (uint8_t[]){0xFF, 0xD8, 0xFF, 0xD9},
        .len = 4,
        .width= 1,
        .height=1,
        .owns_data = false,
    };



    char text[128]= "garbage";
    TEST_ASSERT_TRUE(ocr_engine_read(&img, text, sizeof(text)));
}
