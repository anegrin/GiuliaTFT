#include <stdio.h>
#include <stdlib.h>
#include "gt_main.h"
#include "consts.h"

static gt_onResetDevice _onResetDevice;
static gt_onStateChange _onStateChange;

static lv_obj_t *_dpf_tot_label;
static lv_obj_t *_battery_volt_label;
static lv_obj_t *_battery_ibs_label;
static lv_obj_t *_oil_deg_label;
static lv_obj_t *_tire_temp_fl_label;
static lv_obj_t *_tire_temp_fr_label;
static lv_obj_t *_tire_temp_rl_label;
static lv_obj_t *_tire_temp_rr_label;
static lv_obj_t *_gearbox_oil_temp_label;

static int _currentDpfTot = -1;
static float _currentBatteryVolt = -1;
static int _currentBatteryIBS = -1;
static int _currentOilDeg = -1;
static int _currentTireTempFL = -1;
static int _currentTireTempFR = -1;
static int _currentTireTempRL = -1;
static int _currentTireTempRR = -1;
static int _currentGearboxOilTemp = -1;

void gt_setDpfCount(int count)
{
  if (_currentDpfTot == -1 || _currentDpfTot != count)
  {
    _currentDpfTot = count;
    if (_currentDpfTot >= 0 && lv_obj_is_valid(_dpf_tot_label))
    {
      char str[6];
      sprintf(str, "%d", count % 10000); // module is for overflow protection
      lv_label_set_text(_dpf_tot_label, str);
    }
  }
}

void gt_setBatteryVoltage(float volt)
{
  if (_currentBatteryVolt == -1 || _currentBatteryVolt != volt)
  {
    _currentBatteryVolt = volt;
    if (_currentBatteryVolt >= 0 && volt < 100 && lv_obj_is_valid(_battery_volt_label))
    {
      char str[7];
      sprintf(str, "%.1f V", ((float)volt));
      lv_label_set_text(_battery_volt_label, str);
    }
  }
}

void gt_setBatteryIBS(int percent)
{
  if (_currentBatteryIBS == -1 || _currentBatteryIBS != percent)
  {
    _currentBatteryIBS = percent;
    if (_currentBatteryIBS >= 0 && lv_obj_is_valid(_battery_ibs_label))
    {
      char str[5];
      sprintf(str, "%d%%", percent % 1000); // module is for overflow protection
      lv_label_set_text(_battery_ibs_label, str);
    }
  }
}

void gt_setEngineOilDegradation(int percent)
{
  if (_currentOilDeg == -1 || _currentOilDeg != percent)
  {
    _currentOilDeg = percent;
    if (_currentOilDeg >= 0 && lv_obj_is_valid(_oil_deg_label))
    {
      char str[5];
      sprintf(str, "%d%%", percent % 1000); // module is for overflow protection
      lv_label_set_text(_oil_deg_label, str);
    }
  }
}

void gt_setTireTemperatureFL(int celsius)
{
  if (_currentTireTempFL == -1 || _currentTireTempFL != celsius)
  {
    _currentTireTempFL = celsius;
    if (_currentTireTempFL != -1 && lv_obj_is_valid(_tire_temp_fl_label))
    {
      char str[8];
      sprintf(str, "%d °C", celsius % 1000); // module is for overflow protection
      lv_label_set_text(_tire_temp_fl_label, str);
    }
  }
}

void gt_setTireTemperatureFR(int celsius)
{
  if (_currentTireTempFR == -1 || _currentTireTempFR != celsius)
  {
    _currentTireTempFR = celsius;
    if (_currentTireTempFR != -1 && lv_obj_is_valid(_tire_temp_fr_label))
    {
      char str[8];
      sprintf(str, "%d °C", celsius % 1000); // module is for overflow protection
      lv_label_set_text(_tire_temp_fr_label, str);
    }
  }
}

void gt_setTireTemperatureRL(int celsius)
{
  if (_currentTireTempRL == -1 || _currentTireTempRL != celsius)
  {
    _currentTireTempRL = celsius;
    if (_currentTireTempRL != -1 && lv_obj_is_valid(_tire_temp_rl_label))
    {
      char str[8];
      sprintf(str, "%d °C", celsius % 1000); // module is for overflow protection
      lv_label_set_text(_tire_temp_rl_label, str);
    }
  }
}

void gt_setTireTemperatureRR(int celsius)
{
  if (_currentTireTempRR == -1 || _currentTireTempRR != celsius)
  {
    _currentTireTempRR = celsius;
    if (_currentTireTempRR != -1 && lv_obj_is_valid(_tire_temp_rr_label))
    {
      char str[8];
      sprintf(str, "%d °C", celsius % 1000); // module is for overflow protection
      lv_label_set_text(_tire_temp_rr_label, str);
    }
  }
}

void gt_setGearboxOilTemp(int celsius)
{
  if (_currentGearboxOilTemp == -1 || _currentGearboxOilTemp != celsius)
  {
    _currentGearboxOilTemp = celsius;
    if (_currentGearboxOilTemp != -1 && lv_obj_is_valid(_gearbox_oil_temp_label))
    {
      char str[8];
      sprintf(str, "%d °C", celsius % 1000); // module is for overflow protection
      lv_label_set_text(_gearbox_oil_temp_label, str);
    }
  }
}

static void _onPrevButtonReleased(lv_event_t *e)
{
  lv_scr_load_anim(gt_mainScreen(_onStateChange, _onResetDevice), LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 125, 500, true);
}

static void _onResetBoxEvent(lv_event_t *e)
{
  lv_obj_t *obj = lv_event_get_current_target(e);

  const char *what = lv_msgbox_get_active_btn_text(obj);

  if (lv_obj_is_valid(obj))
  {
    lv_msgbox_close(obj);
  }

  _onResetDevice(strcmp(what, "Hard") == 0);
}

static void _onResetButtonReleased(lv_event_t *e)
{

  static const char *btns[] = {"Soft", "Hard", "\0"};
  lv_obj_t *reset_mbox = lv_msgbox_create(NULL, "Reset", "How do you want to reset the device?", btns, true);
  lv_obj_set_width(reset_mbox, LV_PCT(75));
  lv_obj_center(reset_mbox);

  lv_obj_add_event_cb(reset_mbox, _onResetBoxEvent, LV_EVENT_VALUE_CHANGED, NULL);
}

static void _onScreenEvent(lv_event_t *e)
{
  if (e->code == LV_EVENT_SCREEN_LOADED)
  {
    _onStateChange(SCREEN_VALUES_ID, true);
  }
  else if (e->code == LV_EVENT_SCREEN_UNLOADED)
  {
    _onStateChange(SCREEN_VALUES_ID, false);
  }
}

static void _create_prefix_label(lv_obj_t *values_screen, lv_style_t *style_prefix_label, const char *text, int ordinal)
{
  lv_obj_t *label_prefix = lv_label_create(values_screen);
  lv_obj_add_style(label_prefix, style_prefix_label, LV_STATE_DEFAULT);
  lv_label_set_text(label_prefix, text);
  lv_obj_set_pos(label_prefix, 0, ordinal * GT_FONT_SIZE);
}

lv_obj_t *gt_valuesScreen(gt_onStateChange onStateChange, gt_onResetDevice onResetDevice)
{
  _onResetDevice = onResetDevice;
  _onStateChange = onStateChange;

  lv_obj_t *values_screen = lv_obj_create(NULL);
  lv_obj_add_event_cb(values_screen, _onScreenEvent, LV_EVENT_SCREEN_LOADED, NULL);
  lv_obj_add_event_cb(values_screen, _onScreenEvent, LV_EVENT_SCREEN_UNLOADED, NULL);
  lv_obj_set_style_pad_all(values_screen, GT_THEME_PADDING, LV_STATE_DEFAULT);
  lv_obj_clear_flag(values_screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_color_t bgColor = lv_obj_get_style_bg_color(values_screen, LV_PART_MAIN);

  int baWidth = GT_SCREEN_WIDTH / 2 - GT_THEME_PADDING;

  static lv_style_t style_transparent_label;
  lv_style_init(&style_transparent_label);
  lv_style_set_text_color(&style_transparent_label, lv_palette_lighten(LV_PALETTE_GREY, 1));

  static lv_style_t style_label;
  lv_style_init(&style_label);
  lv_style_set_text_align(&style_label, LV_TEXT_ALIGN_RIGHT);

  _create_prefix_label(values_screen, &style_transparent_label, "DPF regenerations", 0);

  _dpf_tot_label = lv_label_create(values_screen);
  lv_obj_add_style(_dpf_tot_label, &style_label, LV_STATE_DEFAULT);
  lv_obj_set_width(_dpf_tot_label, baWidth);
  lv_label_set_text(_dpf_tot_label, "-");
  lv_obj_set_pos(_dpf_tot_label, baWidth, 0);

  _create_prefix_label(values_screen, &style_transparent_label, "Battery voltage", 1);

  _battery_volt_label = lv_label_create(values_screen);
  lv_obj_add_style(_battery_volt_label, &style_label, LV_STATE_DEFAULT);
  lv_obj_set_width(_battery_volt_label, baWidth);
  lv_label_set_text(_battery_volt_label, "-");
  lv_obj_set_pos(_battery_volt_label, baWidth, GT_FONT_SIZE);

  _create_prefix_label(values_screen, &style_transparent_label, "Battery IBS", 2);

  _battery_ibs_label = lv_label_create(values_screen);
  lv_obj_add_style(_battery_ibs_label, &style_label, LV_STATE_DEFAULT);
  lv_obj_set_width(_battery_ibs_label, baWidth);
  lv_label_set_text(_battery_ibs_label, "-");
  lv_obj_set_pos(_battery_ibs_label, baWidth, GT_FONT_SIZE * 2);

  _create_prefix_label(values_screen, &style_transparent_label, "Engine oil degradation", 3);

  _oil_deg_label = lv_label_create(values_screen);
  lv_obj_add_style(_oil_deg_label, &style_label, LV_STATE_DEFAULT);
  lv_obj_set_width(_oil_deg_label, baWidth);
  lv_label_set_text(_oil_deg_label, "-");
  lv_obj_set_pos(_oil_deg_label, baWidth, GT_FONT_SIZE * 3);

  _create_prefix_label(values_screen, &style_transparent_label, "Tire temperature F/L", 4);

  _tire_temp_fl_label = lv_label_create(values_screen);
  lv_obj_add_style(_tire_temp_fl_label, &style_label, LV_STATE_DEFAULT);
  lv_obj_set_width(_tire_temp_fl_label, baWidth);
  lv_label_set_text(_tire_temp_fl_label, "-");
  lv_obj_set_pos(_tire_temp_fl_label, baWidth, GT_FONT_SIZE * 4);

  _create_prefix_label(values_screen, &style_transparent_label, "Tire temperature F/R", 5);

  _tire_temp_fr_label = lv_label_create(values_screen);
  lv_obj_add_style(_tire_temp_fr_label, &style_label, LV_STATE_DEFAULT);
  lv_obj_set_width(_tire_temp_fr_label, baWidth);
  lv_label_set_text(_tire_temp_fr_label, "-");
  lv_obj_set_pos(_tire_temp_fr_label, baWidth, GT_FONT_SIZE * 5);

  _create_prefix_label(values_screen, &style_transparent_label, "Tire temperature R/L", 6);

  _tire_temp_rl_label = lv_label_create(values_screen);
  lv_obj_add_style(_tire_temp_rl_label, &style_label, LV_STATE_DEFAULT);
  lv_obj_set_width(_tire_temp_rl_label, baWidth);
  lv_label_set_text(_tire_temp_rl_label, "-");
  lv_obj_set_pos(_tire_temp_rl_label, baWidth, GT_FONT_SIZE * 6);

  _create_prefix_label(values_screen, &style_transparent_label, "Tire temperature R/R", 7);

  _tire_temp_rr_label = lv_label_create(values_screen);
  lv_obj_add_style(_tire_temp_rr_label, &style_label, LV_STATE_DEFAULT);
  lv_obj_set_width(_tire_temp_rr_label, baWidth);
  lv_label_set_text(_tire_temp_rr_label, "-");
  lv_obj_set_pos(_tire_temp_rr_label, baWidth, GT_FONT_SIZE * 7);

  _create_prefix_label(values_screen, &style_transparent_label, "Gearbox oil temperature", 8);

  _gearbox_oil_temp_label = lv_label_create(values_screen);
  lv_obj_add_style(_gearbox_oil_temp_label, &style_label, LV_STATE_DEFAULT);
  lv_obj_set_width(_gearbox_oil_temp_label, baWidth);
  lv_label_set_text(_gearbox_oil_temp_label, "-");
  lv_obj_set_pos(_gearbox_oil_temp_label, baWidth, GT_FONT_SIZE * 8);

  lv_obj_t *reset_button = lv_btn_create(values_screen);
  lv_obj_remove_style(reset_button, NULL, LV_PART_MAIN);
  lv_obj_set_width(reset_button, baWidth - GT_FONT_SIZE * 2);
  lv_obj_set_pos(reset_button, 0, GT_SCREEN_HEIGTH - GT_FONT_SIZE - GT_THEME_PADDING * 2);

  lv_obj_t *reset_label = lv_label_create(reset_button);
  lv_obj_add_style(reset_label, &style_transparent_label, LV_STATE_DEFAULT);
  lv_label_set_text(reset_label, LV_SYMBOL_POWER " Reset");

  lv_obj_add_event_cb(reset_button, _onResetButtonReleased, LV_EVENT_RELEASED, NULL);

  lv_obj_t *prev_button = lv_btn_create(values_screen);
  lv_obj_set_width(prev_button, GT_FONT_SIZE * 2);
  lv_obj_set_pos(prev_button, GT_SCREEN_WIDTH / 2 - GT_FONT_SIZE, GT_SCREEN_HEIGTH - GT_FONT_SIZE - GT_THEME_PADDING * 2);

  lv_obj_t *prev_label = lv_label_create(prev_button);
  lv_label_set_text(prev_label, LV_SYMBOL_DOWN);
  lv_obj_center(prev_label);

  lv_obj_add_event_cb(prev_button, _onPrevButtonReleased, LV_EVENT_RELEASED, NULL);

  return values_screen;
}
