#ifdef __cplusplus
extern "C"
{
#endif
#include <lvgl.h>
#include "models.h"
#define GT_SCANNING_TIMEOUT 5000
    typedef void (*gt_onDeviceSelected)(char * name, char * mac,bool persist);
    typedef void (*gt_onDeviceDiscovered)(BTDevice device);
    typedef void (*gt_discoverDevices)(gt_onDeviceDiscovered onDeviceDiscovered);
    lv_obj_t *gt_devicesScreen(gt_discoverDevices discoverDevices, gt_onDeviceSelected onDeviceSelected);
#ifdef __cplusplus
}
#endif