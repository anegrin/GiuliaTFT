#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

#ifndef SD_MOSI
#define SD_MOSI 23
#endif
#ifndef SD_MISO
#define SD_MISO 19
#endif
#ifndef SD_SCK
#define SD_SCK 18
#endif
#ifndef SD_CS
#define SD_CS 5
#endif

#ifdef GT_VERBOSE_LOG
#define VLOGS(s) LOGS(s)
#define VLOG(fmt, ...) LOG(fmt, ##__VA_ARGS__)
#else
#define VLOG(...)
#define VLOGS(s)
#endif