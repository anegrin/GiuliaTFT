#include <stdio.h>
#include <stdlib.h>
#include "gt_devices.h"
#include "consts.h"

typedef struct _btdList_s
{
  char *name;
  char *mac;
  struct _btdList_s *next;
} _btdList;

static _btdList *_results;
static lv_obj_t *_devices_list;
static BTDevice _selectedDevice;
static gt_onDeviceSelected _onDeviceSelected;
static gt_discoverDevices _discoverDevices;

static void _closeMsgBox(lv_timer_t *timer)
{
  lv_obj_t *mbox = timer->user_data;
  if (lv_obj_is_valid(mbox))
  {
    lv_msgbox_close(mbox);
  }
}

static void _clearResults()
{
  _btdList *current = _results;
  if (_results != NULL)
  {
    while (current->next != NULL)
    {
      _btdList *next = current->next;
      free(current);
      current = next;
    }
  }

  _results = NULL;
}

static void _onDeviceBoxEvent(lv_event_t *e)
{
  lv_obj_t *obj = lv_event_get_current_target(e);
  char *mac = e->user_data;

  const char *what = lv_msgbox_get_active_btn_text(obj);

  if (lv_obj_is_valid(obj))
  {
    lv_msgbox_close(obj);
  }

  if (strcmp(what, "No") == 0)
  {
    return;
  }
  bool always = strcmp(what, "Always") == 0;
  if (always || strcmp(what, "Yes") == 0)
  {
    {

      _btdList *current = _results;
      while (strcmp(mac, current->mac) != 0 && current->next != NULL)
      {
        current = current->next;
      }
      _onDeviceSelected(current->name, current->mac, always);
    }
  }
}

static void _onDeviceButtonReleased(lv_event_t *e)
{
  char *mac = lv_obj_get_user_data(lv_event_get_current_target(e));

  _btdList *current = _results;
  while (strcmp(mac, current->mac) != 0 && current->next != NULL)
  {
    current = current->next;
  }

  static const char *btns[] = {"Yes", "Always", "No", "\0"};
  lv_obj_t *device_mbox = lv_msgbox_create(NULL, current->name, "Do you want to connect to this device?", btns, true);
  lv_obj_set_width(device_mbox, LV_PCT(75));
  lv_obj_center(device_mbox);

  lv_obj_add_event_cb(device_mbox, _onDeviceBoxEvent, LV_EVENT_VALUE_CHANGED, current->mac);
}

static void _onDeviceDiscovered(BTDevice device)
{
  lv_obj_t * device_btn = lv_btn_create(_devices_list);
  lv_obj_set_width(device_btn, LV_PCT(100));
  lv_obj_set_style_border_color(device_btn, lv_obj_get_style_bg_color(_devices_list, 0), LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(device_btn, 2, LV_STATE_DEFAULT);
  lv_obj_t * label = lv_label_create(device_btn);
  lv_label_set_text_fmt(label, "%s", device.name != NULL && strcmp(device.name, "") != 0 ? device.name : device.mac);

  lv_obj_add_event_cb(device_btn, _onDeviceButtonReleased, LV_EVENT_RELEASED, NULL);

  lv_obj_set_user_data(device_btn, device.mac);
  if (_results == NULL)
  {
    _btdList *head = NULL;
    head = (_btdList *)malloc(sizeof(_btdList));
    head->name = device.name;
    head->mac = device.mac;
    head->next = NULL;
    _results = head;
  }
  else
  {
    _btdList *current = _results;
    while (current->next != NULL)
    {
      current = current->next;
    }

    current->next = (_btdList *)malloc(sizeof(_btdList));
    current->next->name = device.name;
    current->next->mac = device.mac;
    current->next->next = NULL;
  }
}

static void _onScanButtonReleased(lv_event_t *e)
{

  lv_obj_t *scan_mbox = lv_msgbox_create(NULL, LV_SYMBOL_BLUETOOTH "Scanning...", "Please wait while searching for BT devices", NULL, true);
  lv_obj_center(scan_mbox);
  lv_obj_set_width(scan_mbox, LV_PCT(75));

  lv_timer_t *timer = lv_timer_create(_closeMsgBox, GT_SCANNING_TIMEOUT + 1000, scan_mbox);
  lv_timer_set_repeat_count(timer, 1);

  if (lv_obj_is_valid(_devices_list))
  {
    lv_obj_clean(_devices_list);
  }

  _clearResults();

  _discoverDevices(_onDeviceDiscovered);
}

lv_obj_t *gt_devicesScreen(gt_discoverDevices discoverDevices, gt_onDeviceSelected onDeviceSelected)
{
  _discoverDevices = discoverDevices;
  _onDeviceSelected = onDeviceSelected;

  lv_obj_t *devices_screen = lv_obj_create(NULL);
  lv_obj_set_style_pad_all(devices_screen, GT_THEME_PADDING, LV_STATE_DEFAULT);

  lv_obj_t *title = lv_label_create(devices_screen);
  lv_label_set_text(title, "Bluetooth devices");
  lv_obj_set_align(title, LV_ALIGN_TOP_MID);

  _devices_list = lv_list_create(devices_screen);
  int topOffset = GT_FONT_SIZE + GT_THEME_PADDING;
  lv_obj_set_size(_devices_list, LV_PCT(100), GT_SCREEN_HEIGTH - topOffset - GT_THEME_PADDING * 5 - GT_FONT_SIZE);
  lv_obj_align(_devices_list, LV_ALIGN_TOP_MID, 0, topOffset);

  lv_obj_t *scan_btn = lv_btn_create(devices_screen);
  lv_obj_t *scan_btn_label = lv_label_create(scan_btn);
  lv_label_set_text(scan_btn_label, LV_SYMBOL_BLUETOOTH "Start scanning...");
  lv_obj_set_align(scan_btn, LV_ALIGN_BOTTOM_MID);

  lv_obj_add_event_cb(scan_btn, _onScanButtonReleased, LV_EVENT_RELEASED, NULL);

  return devices_screen;
}
