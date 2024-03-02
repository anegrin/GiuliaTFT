#ifdef __cplusplus
extern "C"
{
#endif
#include <lvgl.h>
#define BOOST_ANIM_TIME_MS 200
#define SCREEN_MAIN_ID 1
#define SCREEN_VALUES_ID 2
    typedef void (*gt_onStateChange)(int screenId, bool active);
    typedef void (*gt_onResetDevice)(bool hard);
    lv_obj_t *gt_mainScreen(gt_onStateChange onStateChange, gt_onResetDevice onResetDevice);
    lv_obj_t *gt_valuesScreen(gt_onStateChange onStateChange, gt_onResetDevice onResetDevice);
    void gt_setGear(int gear);
    void gt_setBoost(int mbar);
    void gt_setCoolant(int celsius);
    void gt_setAirTemp(int celsius);
    void gt_setDpfClogging(int percent);
    void gt_setDpfTemperature(int celsius);
    void gt_setDpfRegeneration(int percent);
    void gt_setDpfDistance(int distance);
    void gt_setDpfCount(int count);
    void gt_setBatteryVoltage(float volt);
    void gt_setBatteryIBS(int percent);
    void gt_setEngineOilDegradation(int percent);
    void gt_setTireTemperatureFL(int celsius);
    void gt_setTireTemperatureFR(int celsius);
    void gt_setTireTemperatureRL(int celsius);
    void gt_setTireTemperatureRR(int celsius);
    void gt_setGearboxOilTemp(int celsius);
#ifdef __cplusplus
}
#endif