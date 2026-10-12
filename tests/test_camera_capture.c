#include "unity.h"
#include "camera_capture.h"
#include "power_manager.h"

void setUp(void)
{
    power_manager_init();
}

void tearDown(void)
{
}

void test_camera_capture_returns_valid_jpeg(void)
{
    TEST_ASSERT_TRUE(camera_is_available());

    image_buffer_t *img = camera_capture();
    TEST_ASSERT_NOT_NULL(img);


    TEST_ASSERT_NOT_NULL(img->data);
    TEST_ASSERT_GREATER_THAN(2, img->len);
    TEST_ASSERT_EQUAL(0xFF, img->data[0]);
    TEST_ASSERT_EQUAL(0xD8, img->data[1]);


    camera_free(img);
}

void test_camera_free_null_safe(void)
{
    camera_free(NULL);
    TEST_ASSERT_TRUE(true);


}
