#include "gt_splash.h"
#include "consts.h"

lv_obj_t * gt_splashScreen(bool booting)
{
  lv_obj_t *splashScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(splashScreen, lv_color_black(), LV_STATE_DEFAULT);

  lv_obj_t * logoLabel = lv_label_create(splashScreen);
  lv_label_set_text(logoLabel, GT_SPLASH_MESSAGE);
  lv_obj_center(logoLabel);

  static lv_style_t style_label;
  lv_style_init(&style_label);
  lv_style_set_text_align(&style_label, LV_TEXT_ALIGN_RIGHT);
  lv_style_set_text_color(&style_label, lv_palette_main(LV_PALETTE_GREY));

  lv_obj_t * bootingLabel = lv_label_create(splashScreen);
  lv_obj_add_style(bootingLabel, &style_label, LV_STATE_DEFAULT);
  lv_label_set_text(bootingLabel, booting ? "booting..." : "resuming...");
  lv_obj_set_width(bootingLabel, GT_SCREEN_WIDTH/2);
  lv_obj_set_pos(bootingLabel, GT_SCREEN_WIDTH/2 - GT_THEME_PADDING,  GT_SCREEN_HEIGTH - GT_FONT_SIZE - GT_THEME_PADDING);

  return splashScreen;
}


