#include <stdio.h>
#include <stdlib.h>
#include "gt_main.h"
#include "consts.h"

const int MBAR_MAX = 3600;

static gt_onResetDevice _onResetDevice;
static gt_onStateChange _onStateChange;

static lv_obj_t *_boost_arc;
static lv_obj_t *_boost_label;
static lv_obj_t *_gear_label;
static lv_obj_t *_coolant_label;
static lv_obj_t *_air_temp_label;
static lv_obj_t *_dpf_icon;
static lv_obj_t *_dpf_clog_label;
static lv_obj_t *_dpf_temp_label;
static lv_obj_t *_dpf_reg_label;
static lv_obj_t *_dpf_dist_label;

static int _boostMbar = -1;
static int _currentGear = -1;
static int _currentDecibar = -1;
static int _currentCoolant = -1;
static int _currentAirTemp = -1;
static int _currentDpfClogging = -1;
static int _currentDpfTemp = -1;
static int _currentDpfRegen = -1;
static int _currentDpfDist = -1;

void gt_setGear(int gear)
{
  if (_currentGear == -1 || gear != _currentGear)
  {
    _currentGear = gear;
    if (_currentGear != -1 && lv_obj_is_valid(_gear_label))
    {
      switch (gear)
      {
      case 0:
        lv_label_set_text(_gear_label, "N");
        break;
      case 16: // seems like both 0x10 and 10 can be R
      case 10:
        lv_label_set_text(_gear_label, "R");
        break;
      case 1:
        lv_label_set_text(_gear_label, "1");
        break;
      case 2:
        lv_label_set_text(_gear_label, "2");
        break;
      case 3:
        lv_label_set_text(_gear_label, "3");
        break;
      case 4:
        lv_label_set_text(_gear_label, "4");
        break;
      case 5:
        lv_label_set_text(_gear_label, "5");
        break;
      case 6:
        lv_label_set_text(_gear_label, "6");
        break;
      case 7:
        lv_label_set_text(_gear_label, "7");
        break;
      case 8:
        lv_label_set_text(_gear_label, "8");
        break;
      default:
        lv_label_set_text(_gear_label, "-");
      }
    }
  }
}

static void _set_angle(void *obj, int32_t v)
{
  lv_arc_set_value(obj, v);
}

static inline int _min(int const x, int const y)
{
  return y < x ? y : x;
}

static inline int _max(int const x, int const y)
{
  return y > x ? y : x;
}

void gt_setBoost(int mbar)
{
  int currentDecibar = mbar / 100;

  if (_currentDecibar == -1 || currentDecibar != _currentDecibar)
  {
    _currentDecibar = currentDecibar;
    if (mbar >= 0 && mbar <= MBAR_MAX)
    {
      if (lv_obj_is_valid(_boost_label))
      {
        char str[5];
        sprintf(str, "%.1f", ((float)mbar) / 1000.0f);
        lv_label_set_text(_boost_label, str);
      }
      if (lv_obj_is_valid(_boost_arc))
      {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, _boost_arc);
        lv_anim_set_exec_cb(&a, _set_angle);
        int animSpeed = _min(_max(BOOST_ANIM_TIME_MS / 3, BOOST_ANIM_TIME_MS * abs(_boostMbar - mbar) / (MBAR_MAX / 2)), BOOST_ANIM_TIME_MS);
        lv_anim_set_time(&a, animSpeed);
        lv_anim_set_values(&a, _boostMbar, mbar);
        lv_anim_start(&a);
      }
    }
    _boostMbar = mbar;
  }
}

void gt_setCoolant(int celsius)
{
  if (_currentCoolant == -1 || _currentCoolant != celsius)
  {
    _currentCoolant = celsius;
    if (_currentCoolant != -1 && lv_obj_is_valid(_coolant_label))
    {
      char str[8];
      sprintf(str, "%d °C", celsius % 1000); // module is for overflow protection
      lv_label_set_text(_coolant_label, str);
    }
  }
}

void gt_setAirTemp(int celsius)
{
  if (_currentAirTemp == -1 || _currentAirTemp != celsius)
  {
    _currentAirTemp = celsius;
    if (_currentAirTemp != -1 && lv_obj_is_valid(_air_temp_label))
    {
      char str[8];
      sprintf(str, "%d °C", celsius % 1000); // module is for overflow protection
      lv_label_set_text(_air_temp_label, str);
    }
  }
}

void gt_setDpfClogging(int percent)
{
  if (_currentDpfClogging == -1 || _currentDpfClogging != percent)
  {
    _currentDpfClogging = percent;
    if (_currentDpfClogging >= 0 && lv_obj_is_valid(_dpf_clog_label))
    {
      char str[5];
      sprintf(str, "%d%%", percent % 1000); // module is for overflow protection
      lv_label_set_text(_dpf_clog_label, str);
    }
  }
}

void gt_setDpfTemperature(int celsius)
{
  if (_currentDpfTemp == -1 || _currentDpfTemp != celsius)
  {
    _currentDpfTemp = celsius;
    if (_currentDpfTemp >= 0 && lv_obj_is_valid(_dpf_temp_label))
    {
      char str[9];
      sprintf(str, "%d °C", celsius % 10000); // module is for overflow protection
      lv_label_set_text(_dpf_temp_label, str);
    }
  }
}

void gt_setDpfRegeneration(int percent)
{
  if (_currentDpfRegen == -1 || _currentDpfRegen != percent)
  {
    _currentDpfRegen = percent;
    if (_currentDpfRegen >= 0)
    {
      if (lv_obj_is_valid(_dpf_reg_label))
      {
        char str[5];
        sprintf(str, "%d%%", percent % 1000); // module is for overflow protection
        lv_label_set_text(_dpf_reg_label, str);
        if (percent > 0)
        {
          lv_obj_add_state(_dpf_reg_label, LV_STATE_USER_1);
        }
        else
        {
          lv_obj_clear_state(_dpf_reg_label, LV_STATE_USER_1);
        }
      }

      if (lv_obj_is_valid(_dpf_icon))
      {
        if (percent > 0)
        {
          lv_obj_add_state(_dpf_icon, LV_STATE_USER_1);
        }
        else
        {
          lv_obj_clear_state(_dpf_icon, LV_STATE_USER_1);
        }
      }
    }
  }
}

void gt_setDpfDistance(int distance)
{
  if (_currentDpfDist == -1 || _currentDpfDist != distance)
  {
    _currentDpfDist = distance;
    if (_currentDpfDist >= 0 && lv_obj_is_valid(_dpf_dist_label))
    {
      char str[9];
      sprintf(str, "%d Km", distance % 10000); // module is for overflow protection
      lv_label_set_text(_dpf_dist_label, str);
    }
  }
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

static void _onNextButtonReleased(lv_event_t *e)
{
  lv_scr_load_anim(gt_valuesScreen(_onStateChange, _onResetDevice), LV_SCR_LOAD_ANIM_MOVE_TOP, 125, 500, true);
}

static void _onScreenEvent(lv_event_t *e)
{
  if (e->code == LV_EVENT_SCREEN_LOADED)
  {
    _onStateChange(SCREEN_MAIN_ID, true);
  }
  else if (e->code == LV_EVENT_SCREEN_UNLOADED)
  {
    _onStateChange(SCREEN_MAIN_ID, false);
  }
}

static void _create_prefix_label(lv_obj_t *main_screen, lv_style_t *style_dpf_label, lv_style_t *style_prefix_label, const char *text, int baWidth, int dpfOffset, int ordinal)
{
  lv_obj_t *dpf_label_prefix = lv_label_create(main_screen);
  lv_obj_set_width(dpf_label_prefix, baWidth / 2);
  lv_obj_add_style(dpf_label_prefix, style_dpf_label, LV_STATE_DEFAULT);
  lv_obj_add_style(dpf_label_prefix, style_prefix_label, LV_STATE_DEFAULT);
  lv_label_set_text(dpf_label_prefix, text);
  lv_obj_set_pos(dpf_label_prefix, baWidth, dpfOffset - GT_FONT_SIZE * (4 - ordinal));
}

lv_obj_t *gt_mainScreen(gt_onStateChange onStateChange, gt_onResetDevice onResetDevice)
{

  _onResetDevice = onResetDevice;
  _onStateChange = onStateChange;

  lv_obj_t *main_screen = lv_obj_create(NULL);
  lv_obj_add_event_cb(main_screen, _onScreenEvent, LV_EVENT_SCREEN_LOADED, NULL);
  lv_obj_add_event_cb(main_screen, _onScreenEvent, LV_EVENT_SCREEN_UNLOADED, NULL);
  lv_obj_set_style_pad_all(main_screen, GT_THEME_PADDING, LV_STATE_DEFAULT);
  lv_obj_clear_flag(main_screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_color_t bgColor = lv_obj_get_style_bg_color(main_screen, LV_PART_MAIN);

  int baWidth = GT_SCREEN_WIDTH / 2 - GT_THEME_PADDING;

  LV_FONT_DECLARE(gt_font_segments_128);
  LV_FONT_DECLARE(gt_font_segments_96);

  static lv_style_t style_icons_label;
  lv_style_init(&style_icons_label);
  /*lv_style_set_border_color(&style_icons_label, lv_palette_main(LV_PALETTE_GREEN));
  lv_style_set_border_width(&style_icons_label, 1);*/
  lv_style_set_text_align(&style_icons_label, LV_TEXT_ALIGN_CENTER);

  static lv_style_t style_transparent_label;
  lv_style_init(&style_transparent_label);
  lv_style_set_text_color(&style_transparent_label, lv_palette_lighten(LV_PALETTE_GREY, 1));

  _gear_label = lv_label_create(main_screen);
  static lv_style_t style_gear_label;
  lv_style_init(&style_gear_label);
  lv_style_set_text_font(&style_gear_label, GT_FONT_SEGMENTS);
  lv_style_set_text_align(&style_gear_label, LV_TEXT_ALIGN_CENTER);
  lv_obj_add_style(_gear_label, &style_gear_label, LV_STATE_DEFAULT);
  lv_label_set_text(_gear_label, "-");
  lv_obj_set_pos(_gear_label, baWidth / 2 - GT_FONT_SEGMENTS_SIZE / 4, baWidth / 2 - GT_FONT_SEGMENTS_SIZE / 2);

  lv_obj_t *gear_label_suffix = lv_label_create(main_screen);
  lv_obj_add_style(gear_label_suffix, &style_icons_label, LV_STATE_DEFAULT);
  lv_obj_add_style(gear_label_suffix, &style_transparent_label, LV_STATE_DEFAULT);
  lv_obj_set_width(gear_label_suffix, baWidth / 2);
  lv_label_set_text(gear_label_suffix, "G");
  lv_obj_set_pos(gear_label_suffix, 0, (baWidth - GT_FONT_SIZE) / 2);

  lv_obj_t *boost_green_arc_segment = lv_arc_create(main_screen);
  lv_obj_remove_style(boost_green_arc_segment, NULL, LV_PART_MAIN);
  lv_obj_remove_style(boost_green_arc_segment, NULL, LV_PART_KNOB);
  lv_obj_set_size(boost_green_arc_segment, baWidth, baWidth);
  lv_arc_set_value(boost_green_arc_segment, 0);
  lv_arc_set_rotation(boost_green_arc_segment, 90);
  lv_arc_set_bg_angles(boost_green_arc_segment, 0, 180);
  lv_obj_clear_flag(boost_green_arc_segment, LV_OBJ_FLAG_CLICKABLE);
  static lv_style_t style_boost_green_arc_segment;
  lv_style_init(&style_boost_green_arc_segment);
  lv_style_set_arc_width(&style_boost_green_arc_segment, GT_THEME_PADDING);
  lv_style_set_arc_rounded(&style_boost_green_arc_segment, false);
  lv_style_set_arc_color(&style_boost_green_arc_segment, lv_palette_darken(LV_PALETTE_GREEN, 1));
  lv_obj_add_style(boost_green_arc_segment, &style_boost_green_arc_segment, LV_PART_MAIN);
  lv_obj_add_style(boost_green_arc_segment, &style_boost_green_arc_segment, LV_PART_INDICATOR);

  lv_obj_t *boost_orange_arc_segment = lv_arc_create(main_screen);
  lv_obj_remove_style(boost_orange_arc_segment, NULL, LV_PART_MAIN);
  lv_obj_remove_style(boost_orange_arc_segment, NULL, LV_PART_KNOB);
  lv_obj_set_size(boost_orange_arc_segment, baWidth, baWidth);
  lv_arc_set_value(boost_orange_arc_segment, 0);
  lv_arc_set_rotation(boost_orange_arc_segment, 90);
  lv_arc_set_bg_angles(boost_orange_arc_segment, 180, 225);
  lv_obj_clear_flag(boost_orange_arc_segment, LV_OBJ_FLAG_CLICKABLE);
  static lv_style_t style_boost_orange_arc_segment;
  lv_style_init(&style_boost_orange_arc_segment);
  lv_style_set_arc_width(&style_boost_orange_arc_segment, GT_THEME_PADDING);

  lv_style_set_arc_rounded(&style_boost_orange_arc_segment, false);
  lv_style_set_arc_color(&style_boost_orange_arc_segment, lv_palette_darken(LV_PALETTE_ORANGE, 1));
  lv_obj_add_style(boost_orange_arc_segment, &style_boost_orange_arc_segment, LV_PART_MAIN);
  lv_obj_add_style(boost_orange_arc_segment, &style_boost_orange_arc_segment, LV_PART_INDICATOR);

  lv_obj_t *boost_red_arc_segment = lv_arc_create(main_screen);
  lv_obj_remove_style(boost_red_arc_segment, NULL, LV_PART_MAIN);
  lv_obj_remove_style(boost_red_arc_segment, NULL, LV_PART_KNOB);
  lv_obj_set_size(boost_red_arc_segment, baWidth, baWidth);
  lv_arc_set_value(boost_red_arc_segment, 0);
  lv_arc_set_rotation(boost_red_arc_segment, 90);
  lv_arc_set_bg_angles(boost_red_arc_segment, 225, 270);
  lv_obj_clear_flag(boost_red_arc_segment, LV_OBJ_FLAG_CLICKABLE);
  static lv_style_t style_boost_red_arc_segment;
  lv_style_init(&style_boost_red_arc_segment);
  lv_style_set_arc_width(&style_boost_red_arc_segment, GT_THEME_PADDING);

  lv_style_set_arc_rounded(&style_boost_red_arc_segment, false);
  lv_style_set_arc_color(&style_boost_red_arc_segment, lv_palette_darken(LV_PALETTE_RED, 1));
  lv_obj_add_style(boost_red_arc_segment, &style_boost_red_arc_segment, LV_PART_MAIN);
  lv_obj_add_style(boost_red_arc_segment, &style_boost_red_arc_segment, LV_PART_INDICATOR);

  _boost_arc = lv_arc_create(main_screen);
  lv_obj_remove_style(_boost_arc, NULL, LV_PART_MAIN);
  lv_obj_set_style_opa(_boost_arc, LV_OPA_0, LV_PART_INDICATOR);
  lv_obj_set_size(_boost_arc, baWidth, baWidth);
  lv_arc_set_range(_boost_arc, 0, MBAR_MAX);
  lv_arc_set_rotation(_boost_arc, 90);
  lv_arc_set_bg_angles(_boost_arc, 0, 270);
  lv_obj_clear_flag(_boost_arc, LV_OBJ_FLAG_CLICKABLE);

  static lv_style_t style_boost_arc;
  lv_style_init(&style_boost_arc);
  lv_style_set_bg_color(&style_boost_arc, lv_palette_lighten(LV_PALETTE_GREY, 3));
  lv_obj_add_style(_boost_arc, &style_boost_arc, LV_PART_KNOB);

  static lv_style_t style_boost_label;
  lv_style_init(&style_boost_label);
  lv_style_set_text_align(&style_boost_label, LV_TEXT_ALIGN_RIGHT);

  lv_obj_t *boost_label_prefix = lv_label_create(main_screen);
  lv_obj_add_style(boost_label_prefix, &style_boost_label, LV_STATE_DEFAULT);
  lv_obj_add_style(boost_label_prefix, &style_transparent_label, LV_STATE_DEFAULT);
  lv_label_set_text(boost_label_prefix, "boost");
  lv_obj_set_width(boost_label_prefix, baWidth / 2);
  lv_obj_set_pos(boost_label_prefix, baWidth / 2, baWidth - GT_FONT_SIZE * 2);

  _boost_label = lv_label_create(main_screen);
  lv_obj_add_style(_boost_label, &style_boost_label, LV_STATE_DEFAULT);
  lv_label_set_text(_boost_label, "0.0");
  lv_obj_set_width(_boost_label, baWidth / 4);
  lv_obj_set_pos(_boost_label, baWidth / 2, baWidth - GT_FONT_SIZE);

  lv_obj_t *boost_label_suffix = lv_label_create(main_screen);
  lv_obj_add_style(boost_label_suffix, &style_boost_label, LV_STATE_DEFAULT);
  lv_label_set_text(boost_label_suffix, "Bar");
  lv_obj_set_width(boost_label_suffix, baWidth / 4);
  lv_obj_set_pos(boost_label_suffix, baWidth / 2 + baWidth / 4, baWidth - GT_FONT_SIZE);

  LV_FONT_DECLARE(gt_font_icons_64);
  LV_FONT_DECLARE(gt_font_icons_48);

  static lv_style_t style_icons;
  lv_style_init(&style_icons);
  /*lv_style_set_border_color(&style_icons, lv_palette_main(LV_PALETTE_YELLOW));
  lv_style_set_border_width(&style_icons, 1);*/
  lv_style_set_text_font(&style_icons, GT_FONT_ICONS);
  lv_style_set_text_align(&style_icons, LV_TEXT_ALIGN_CENTER);

  lv_obj_t *coolant_icon = lv_label_create(main_screen);
  lv_obj_set_width(coolant_icon, baWidth / 2);
  lv_obj_add_style(coolant_icon, &style_icons, LV_STATE_DEFAULT);
  lv_label_set_text(coolant_icon, "C");
  lv_obj_set_pos(coolant_icon, baWidth, baWidth / 8);

  _coolant_label = lv_label_create(main_screen);
  lv_obj_set_width(_coolant_label, baWidth / 2);
  lv_obj_add_style(_coolant_label, &style_icons_label, LV_STATE_DEFAULT);
  lv_label_set_text(_coolant_label, "-");
  lv_obj_set_pos(_coolant_label, baWidth, baWidth / 2 - GT_FONT_SIZE);

  lv_obj_t *air_temp_icon = lv_label_create(main_screen);
  lv_obj_set_width(air_temp_icon, baWidth / 2);
  lv_obj_add_style(air_temp_icon, &style_icons, LV_STATE_DEFAULT);
  lv_label_set_text(air_temp_icon, "A");
  lv_obj_set_pos(air_temp_icon, baWidth + baWidth / 2, baWidth / 8);

  _air_temp_label = lv_label_create(main_screen);
  lv_obj_set_width(_air_temp_label, baWidth / 2);
  lv_obj_add_style(_air_temp_label, &style_icons_label, LV_STATE_DEFAULT);
  lv_label_set_text(_air_temp_label, "-");
  lv_obj_set_pos(_air_temp_label, baWidth + baWidth / 2, baWidth / 2 - GT_FONT_SIZE);

  static lv_style_t style_warn;
  lv_style_init(&style_warn);
  lv_style_set_text_color(&style_warn, lv_palette_main(LV_PALETTE_DEEP_ORANGE));

  int dpfOffset = baWidth + GT_FONT_SIZE / 2;

  _dpf_icon = lv_label_create(main_screen);
  lv_obj_set_width(_dpf_icon, baWidth);
  lv_obj_add_style(_dpf_icon, &style_icons, LV_STATE_DEFAULT);
  lv_obj_add_style(_dpf_icon, &style_warn, LV_STATE_USER_1);
  lv_label_set_text(_dpf_icon, "D");
  lv_obj_set_pos(_dpf_icon, baWidth, (dpfOffset - GT_THEME_PADDING) / 2);

  static lv_style_t style_dpf_label;
  lv_style_init(&style_dpf_label);
  lv_style_set_text_align(&style_dpf_label, LV_TEXT_ALIGN_RIGHT);

  _create_prefix_label(main_screen, &style_dpf_label, &style_transparent_label, "clog.", baWidth, dpfOffset, 1);

  _dpf_clog_label = lv_label_create(main_screen);
  lv_obj_set_width(_dpf_clog_label, baWidth / 2);
  lv_obj_add_style(_dpf_clog_label, &style_dpf_label, LV_STATE_DEFAULT);
  lv_label_set_text(_dpf_clog_label, "-");
  lv_obj_set_pos(_dpf_clog_label, baWidth + baWidth / 2, dpfOffset - GT_FONT_SIZE * 3);

  _create_prefix_label(main_screen, &style_dpf_label, &style_transparent_label, "temp.", baWidth, dpfOffset, 2);

  _dpf_temp_label = lv_label_create(main_screen);
  lv_obj_set_width(_dpf_temp_label, baWidth / 2);
  lv_obj_add_style(_dpf_temp_label, &style_dpf_label, LV_STATE_DEFAULT);
  lv_label_set_text(_dpf_temp_label, "-");
  lv_obj_set_pos(_dpf_temp_label, baWidth + baWidth / 2, dpfOffset - GT_FONT_SIZE * 2);

  _create_prefix_label(main_screen, &style_dpf_label, &style_transparent_label, "reg.", baWidth, dpfOffset, 3);

  _dpf_reg_label = lv_label_create(main_screen);
  lv_obj_set_width(_dpf_reg_label, baWidth / 2);
  lv_obj_add_style(_dpf_reg_label, &style_dpf_label, LV_STATE_DEFAULT);
  lv_obj_add_style(_dpf_reg_label, &style_warn, LV_STATE_USER_1);
  lv_label_set_text(_dpf_reg_label, "-");
  lv_obj_set_pos(_dpf_reg_label, baWidth + baWidth / 2, dpfOffset - GT_FONT_SIZE);

  _create_prefix_label(main_screen, &style_dpf_label, &style_transparent_label, "dist.", baWidth, dpfOffset, 4);

  _dpf_dist_label = lv_label_create(main_screen);
  lv_obj_set_width(_dpf_dist_label, baWidth / 2);
  lv_obj_add_style(_dpf_dist_label, &style_dpf_label, LV_STATE_DEFAULT);
  lv_obj_add_style(_dpf_dist_label, &style_warn, LV_STATE_USER_1);
  lv_label_set_text(_dpf_dist_label, "-");
  lv_obj_set_pos(_dpf_dist_label, baWidth + baWidth / 2, dpfOffset);

  lv_obj_t *reset_button = lv_btn_create(main_screen);
  lv_obj_remove_style(reset_button, NULL, LV_PART_MAIN);
  lv_obj_set_width(reset_button, baWidth - GT_FONT_SIZE * 2);
  lv_obj_set_pos(reset_button, 0, GT_SCREEN_HEIGTH - GT_FONT_SIZE - GT_THEME_PADDING * 2);

  lv_obj_t *reset_label = lv_label_create(reset_button);
  lv_obj_add_style(reset_label, &style_transparent_label, LV_STATE_DEFAULT);
  lv_label_set_text(reset_label, LV_SYMBOL_POWER " Reset");

  lv_obj_add_event_cb(reset_button, _onResetButtonReleased, LV_EVENT_RELEASED, NULL);

  lv_obj_t *next_button = lv_btn_create(main_screen);
  lv_obj_set_width(next_button, GT_FONT_SIZE * 2);
  lv_obj_set_pos(next_button, GT_SCREEN_WIDTH / 2 - GT_FONT_SIZE, GT_SCREEN_HEIGTH - GT_FONT_SIZE - GT_THEME_PADDING * 2);

  lv_obj_t *next_label = lv_label_create(next_button);
  lv_label_set_text(next_label, LV_SYMBOL_UP);
  lv_obj_center(next_label);

  lv_obj_add_event_cb(next_button, _onNextButtonReleased, LV_EVENT_RELEASED, NULL);

  return main_screen;
}
