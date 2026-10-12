#include "unity.h"
#include "config.h"
#include "power_manager.h"

void setUp(void)
{
    power_manager_init();
}

void tearDown(void){}

void test_default_mode_is_on_demand(void)
{
    TEST_ASSERT_EQUAL(MODE_ON_DEMAND, power_manager_get_mode());
}

void test_mode_switch(void)
{
    power_manager_set_mode(MODE_CONTINUOUS);


    TEST_ASSERT_EQUAL(MODE_CONTINUOUS, power_manager_get_mode());

    power_manager_set_mode(MODE_ON_DEMAND);
    TEST_ASSERT_EQUAL(MODE_ON_DEMAND,power_manager_get_mode());
}


void test_battery_reading_in_valid_range(void)
{
    uint32_t mv= power_manager_read_battery_mv();
    TEST_ASSERT_GREATER_OR_EQUAL(3000,mv);
    TEST_ASSERT_LESS_OR_EQUAL(4200, mv);
}



void test_peripheral_gating_no_crash(void)
{
    power_manager_periph_enable(PERIPH_CAMERA, true);
    power_manager_periph_enable(PERIPH_CAMERA, false);
    power_manager_periph_enable(PERIPH_AUDIO_AMP, true);
    power_manager_periph_enable(PERIPH_AUDIO_AMP,false);
    TEST_ASSERT_TRUE(true);


}
