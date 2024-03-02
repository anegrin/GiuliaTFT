#define GT_ROTATION 3

#define GT_THEME_PADDING 12

#ifndef GT_FONT_SIZE
#define GT_FONT_SIZE 14
#endif

#ifdef TFT_WIDTH
#define GT_SCREEN_WIDTH TFT_HEIGHT//because of rotation 1 or 3
#elif SDL_HOR_RES
#define GT_SCREEN_WIDTH SDL_HOR_RES
#else
#define GT_SCREEN_WIDTH 480
#endif

#ifdef TFT_HEIGHT
#define GT_SCREEN_HEIGTH TFT_WIDTH//because of rotation 1 or 3
#elif SDL_VER_RES
#define GT_SCREEN_HEIGTH SDL_VER_RES
#else
#define GT_SCREEN_HEIGTH 320
#endif