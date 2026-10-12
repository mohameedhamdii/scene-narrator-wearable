#include "unity.h"
#include "vlm_inference.h"
#include "model_loader.h"
#include "camera_capture.h"

void setUp(void)
{
    model_loader_init(NULL);
    vlm_inference_init();
}

void tearDown(void)
{
    vlm_inference_deinit();
    model_loader_deinit();
}

void test_vlm_produces_non_empty_caption(void)
{


    image_buffer_t *img = camera_capture();
    TEST_ASSERT_NOT_NULL(img);

    char caption[256];
    TEST_ASSERT_TRUE(vlm_inference_describe(img, caption, sizeof(caption)));

    TEST_ASSERT_GREATER_THAN(10, strlen(caption));

    camera_free(img);
}

void test_vlm_rejects_null_input(void)
{
    char caption[256];


    TEST_ASSERT_FALSE(vlm_inference_describe(NULL, caption, sizeof(caption)));
}
