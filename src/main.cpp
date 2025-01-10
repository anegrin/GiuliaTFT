#ifndef USE_SDL
#include <FS.h>
#include <lvgl.h>

#include <TFT_eSPI.h>
#include <TFT_eWidget.h>
#include <BluetoothSerial.h>
#include <ELMduino.h>
#include <log4arduino.h>

#ifdef DPF_AUDIO_NOTIFICATION
#include <MusicDefinitions.h>
#include <XT_DAC_Audio.h>
#endif

#include "Free_Fonts.h"
#include "consts.h"
#include "screens/gt_main.h"
#include "screens/gt_devices.h"
#include "screens/gt_splash.h"
#include "main.h"

#ifndef ELM_DEBUG
#define ELM_DEBUG false // elmduino debugging
#endif
#ifndef GT_DEMO
#define GT_DEMO false // show demo random data
#endif

#ifdef DPF_AUDIO_NOTIFICATION
int8_t PROGMEM NOTE[] = { NOTE_A4, SCORE_END};
XT_DAC_Audio_Class DacAudio(26, 0);                                         
XT_MusicScore_Class Beep(NOTE, TEMPO_PRESTO, INSTRUMENT_PIANO);
#endif

#define BT_DISCOVER_TIME 3000
#define LOOP_DELAY 3
#define ELM_LOOP_DELAY 3
#define RT_SENSORS_DELAY 50
#define SWITCHED_STATE_DELAY 1000
#define ELM_QUERY_TIMEOUT 2000
#define RECONNECTION_DELAY 5000
#define NON_RT_SENSORS_DELAY 1000
#define VALUES_SENSORS_DELAY 1000
#define QUERY_OFFSET 300 // to time-distribute data loading on main screen
#define NO_DATA_TIMEOUT 60000
#define DEEPSLEEP_DURATION_US 15000000L      // 15s
#define MAX_DEEPSLEEP_DURATION_US 120000000L // 20m

#define CALIBRATION_FILE "/calib.data"
#define DEVICE_FILE "/device.data"

#define HEADER_DA10F1 "DA10F1"
#define HEADER_DA10F1_ID 1
#define HEADER_DB33F1 "DB33F1"
#define HEADER_DB33F1_ID 2
#define HEADER_DAC7F1 "DAC7F1"
#define HEADER_DAC7F1_ID 3
#define HEADER_DA18F1 "DA18F1"
#define HEADER_DA18F1_ID 4

BluetoothSerial SerialBT;
ELM327 btELM327;

static lv_disp_draw_buf_t _draw_buf;
static lv_color_t _screen_buffer[GT_SCREEN_WIDTH * GT_SCREEN_HEIGTH / 10];
static uint16_t _touchX, _touchY;
RTC_DATA_ATTR int _rtcBootCount = 0; // keep state when deepsleeping
RTC_DATA_ATTR bool _rtcGotDataOnce;  // keep state when deepsleeping
static bool _gotDataOnce;            // same as _rtcGotDataOnce to avoid access to slow mem

static bool _elmConnected = false;
static int _currentHeaderId = -1;
static char *_mac;
static bool _btScanning = false;
static int _btScanDurationMs = 0;
static unsigned long _mainGearLastQueringMs = 0L;
static unsigned long _mainBoostLastQueringMs = RT_SENSORS_DELAY / 2;
static unsigned long _mainCoolantLastQueringMs = QUERY_OFFSET * 1;
static unsigned long _mainAirTempLastQueringMs = QUERY_OFFSET * 2;
static unsigned long _mainDPFClogLastQueringMs = QUERY_OFFSET * 3;
static unsigned long _mainDPFTempLastQueringMs = QUERY_OFFSET * 4;
static unsigned long _mainDPFRegLastQueringMs = QUERY_OFFSET * 5;
static unsigned long _mainDPFDistLastQueringMs = QUERY_OFFSET * 6;
static unsigned long _valuesNonRtLastQueringMs = 0L;
static bool _mainScreenActive = false;
static bool _valuesScreenActive = false;

static int _gear = -1;
static int _boost = -1;
static int _coolant = -1;
static int _airTemp = -1;
static int _dpfClogging = -1;
static int _dpfTemperature = -1;
static int _dpfRegeneration = -1;
static bool _shouldPlayDPFSound = true;
static int _dpfDistance = -1;
static int _dpfCount = -1;
static float _batteryVoltage = -1;
static int _batteryIBS = -1;
static int _engineOilDegradation = -1;
static int _tireTempFL = -1;
static int _tireTempFR = -1;
static int _tireTempRL = -1;
static int _tireTempRR = -1;
static int _gearboxOilTemp = -1;

static gt_onDeviceDiscovered _onDeviceDiscovered;

TFT_eSPI tft = TFT_eSPI();

void _onStateChange(int screenId, bool active)
{
  if (screenId == SCREEN_MAIN_ID)
  {
    VLOG("Switching main screen state to %s", active ? "active" : "inactive");

    if (active)
    {
      _gear = -1;
      _boost = -1;
      _coolant = -1;
      _airTemp = -1;
      _dpfClogging = -1;
      _dpfTemperature = -1;
      _dpfRegeneration = -1;
      _shouldPlayDPFSound = true;
      _dpfDistance = -1;

      unsigned long now = millis();
      _mainGearLastQueringMs = now;
      _mainBoostLastQueringMs = now + RT_SENSORS_DELAY / 2;
      _mainCoolantLastQueringMs = now + QUERY_OFFSET;
      _mainAirTempLastQueringMs = now + QUERY_OFFSET * 2;
      _mainDPFClogLastQueringMs = now + QUERY_OFFSET * 3;
      _mainDPFTempLastQueringMs = now + QUERY_OFFSET * 4;
      _mainDPFRegLastQueringMs = now + QUERY_OFFSET * 5;
      _mainDPFDistLastQueringMs = now + QUERY_OFFSET * 6;
    }

    _mainScreenActive = active;
  }
  else if (screenId == SCREEN_VALUES_ID)
  {
    VLOG("Switching values screen state to %s", active ? "active" : "inactive");
    if (active)
    {
      _dpfCount = -1;
      _batteryVoltage = -1;
      _batteryIBS = -1;
      _engineOilDegradation = -1;
      _tireTempFL = -1;
      _tireTempFR = -1;
      _tireTempRL = -1;
      _tireTempRR = -1;
      _gearboxOilTemp = -1;
      _valuesNonRtLastQueringMs = 0L;
    }

    _valuesScreenActive = active;
  }
}

// _demo values
static int _get_demo_v(int pid)
{
  switch (pid)
  {
  case 0x192D:
    return (int)(((float)rand() / (float)(RAND_MAX)) * 11);
  case 0x195A:
    return 2000 + (int)(((float)rand() / (float)(RAND_MAX)) * 1000) - 500;
  case 0x1003:
    return (int)(((float)rand() / (float)(RAND_MAX)) * 130);
  case 0x1935:
    return (int)(((float)rand() / (float)(RAND_MAX)) * 50) - 10;
  case 0x18E4:
    return 75 + (int)(((float)rand() / (float)(RAND_MAX)) * 10);
  case 0x18DE:
    return 500 + (int)(((float)rand() / (float)(RAND_MAX)) * 100);
  case 0x380B:
    return (int)(((float)rand() / (float)(RAND_MAX)) * 2);
  case 0x3807:
    return 200 + (int)(((float)rand() / (float)(RAND_MAX)) * 50);
  case 0x18A4:
    return 10 + (int)(((float)rand() / (float)(RAND_MAX)) * 10);
  case 0x42:
    return 12.0f + (((float)rand() / (float)(RAND_MAX)) * 2);
  case 0x19BD:
    return 80 + (int)(((float)rand() / (float)(RAND_MAX)) * 15);
  case 0x3813:
    return 80 + (int)(((float)rand() / (float)(RAND_MAX)) * 15);
  case 0x40B1:
  case 0x40B2:
  case 0x40B3:
  case 0x30B4:
  case 0x04FE:
    return 70 + (int)(((float)rand() / (float)(RAND_MAX)) * 10) - 10;
  default:
    return -1;
  }
}
// /_demo values

void _dispFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

void _touchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{

  bool touched = tft.getTouch(&_touchX, &_touchY, 600);
  if (!touched)
  {
    data->state = LV_INDEV_STATE_REL;
  }
  else
  {
    data->state = LV_INDEV_STATE_PR;

    data->point.x = _touchX;
    data->point.y = _touchY;
  }
}

/**
 * handle callbacks for closed or congested connection
 */
void _gt_btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if (event == ESP_SPP_CLOSE_EVT)
  {
    LOGS("Bluetooth disconnected");
    SerialBT.register_callback(NULL);
    _elmConnected = false;
  }
  else if (event == ESP_SPP_CONG_EVT)
  {
    if (param->cong.cong && btELM327.nb_rx_state == ELM_TIMEOUT)
    {
      LOGS("Bluetooth connection congested, let's reconnect");
      SerialBT.register_callback(NULL);
      _elmConnected = false;
    }
  }
}

/**
 * do connect to ELM327 device
 */
static bool _gt_elmConnect()
{
  LOG("Connecting to ELM327 device %s", _mac);
  if (GT_DEMO)
  {
    return true;
  }
  bool success = false;
  SerialBT.register_callback(NULL);
  SerialBT.end();
  SerialBT.register_callback(_gt_btCallback);
  SerialBT.begin("ESP32", true);

  if (SerialBT.connect(BTAddress(_mac)))
  {
    if (!btELM327.begin(SerialBT, ELM_DEBUG, ELM_QUERY_TIMEOUT))
    {
      LOG("Can't init connection to ELM327 device %s", _mac);
    }
    else
    {
      success = true;
    }
  }
  else
  {
    LOG("Can't connect to ELM327 device %s", _mac);
  }

  if (!success)
  {
    SerialBT.end();
    LOG("ELM327 connection failed, waiting %d ms", RECONNECTION_DELAY);
  }

  return success;
}

/**
 * set header, returns headerId if set or -1 if it fails
 */
static int _gt_setHeader(int headerId, const char *header)
{
  if (GT_DEMO)
  {
    return headerId;
  }

  VLOG("nb_rx_state is %d", btELM327.nb_rx_state);

  char command[20] = {'\0'};
  sprintf(command, SET_HEADER, header);
  VLOG("Setting header to %s", header);
  if (btELM327.sendCommand_Blocking(command) == ELM_SUCCESS)
  {
    if (strstr(btELM327.payload, "OK") != NULL)
    {
      VLOG("Header set to %s", header);
      return headerId;
    }
  }

  return -1;
}

/**
 * process PID in blocking mode
 */
static float _gt_processPID(const uint8_t &service, const uint16_t &pid, const uint8_t &numResponses, const uint8_t &numExpectedBytes, const double &scaleFactor, const float &bias)
{
  unsigned long timeoutAt = millis() + (ELM_QUERY_TIMEOUT * 2);
  while (millis() < timeoutAt)
  {
    delay(1);
    if (GT_DEMO) // wanna test some delay :)
    {
      delay(ELM_LOOP_DELAY);
      return _get_demo_v(pid);
    }
    float value = btELM327.processPID(service, pid, numResponses, numExpectedBytes, scaleFactor, bias);
    if (btELM327.nb_rx_state == ELM_SUCCESS)
    {
      VLOG("Value for %x is %.2f", pid, value);
      return value;
    }
    if (btELM327.nb_rx_state != ELM_GETTING_MSG)
    {
      btELM327.printError();
      return -1;
    }
  }

  return -1;
}

static void _gt_resetFunc(lv_timer_t *timer)
{
  esp_restart();
}

/**
 * on reset callback, if hard then it will also delete stored settings (device mac address and calibration data)
 */
void _gt_onResetDevice(bool hard)
{
  LOG("Performing %s reset", hard ? "hard" : "soft");

  if (hard)
  {
    if (SPIFFS.exists(DEVICE_FILE))
      SPIFFS.remove(DEVICE_FILE);
    if (SPIFFS.exists(CALIBRATION_FILE))
      SPIFFS.remove(CALIBRATION_FILE);
  }

  if (_elmConnected)
  {
    LOGS("Disconnecting Bluetooth");
    _currentHeaderId = -1;
    SerialBT.end();
  }

  lv_timer_create(_gt_resetFunc, SWITCHED_STATE_DELAY, NULL);
}

/**
 * ELM loop; handles:
 * - reset if no connection was successful for NO_DATA_TIMEOUT
 * - deepsleep if conected once but no data retrieved for NO_DATA_TIMEOUT
 * - do connect if not connected
 * - get data for main screen if active
 * - get data for values screen if active
 */
void _gt_elmLoop(void *pvParameters)
{

  LOGS("Starting ELM loop");

  while (true)
  {
    delay(ELM_LOOP_DELAY);

    if (millis() > _mainGearLastQueringMs + NO_DATA_TIMEOUT && millis() > _valuesNonRtLastQueringMs + NO_DATA_TIMEOUT)
    {
      if (_rtcGotDataOnce)
      {
        unsigned long durationUs = min((long)(pow(2, _rtcBootCount - 1) * DEEPSLEEP_DURATION_US), MAX_DEEPSLEEP_DURATION_US);
        LOG("Deepsleeping for %dus", durationUs);
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        esp_sleep_enable_timer_wakeup(durationUs);
        esp_deep_sleep_start();
      }
      else if (_elmConnected)
      {
        _gt_onResetDevice(false);
      }
    }

    if (!_elmConnected && (_valuesScreenActive || _mainScreenActive))
    {
      delay(SWITCHED_STATE_DELAY);
      bool success = _gt_elmConnect();
      if (!success)
      {
        delay(RECONNECTION_DELAY);
        continue;
      }
      else
      {
        _currentHeaderId = -1;
        _elmConnected = true;
        if (!GT_DEMO && btELM327.sendCommand_Blocking(DISP_CURRENT_PROTOCOL) == ELM_SUCCESS)
        {
          LOG("Current protocol is %s", btELM327.payload);
        }
        delay(ELM_QUERY_TIMEOUT);
      }
    }

    if (_elmConnected && !_valuesScreenActive && _mainScreenActive)
    {

      if (_currentHeaderId != HEADER_DA10F1_ID)
      {
        int headerSet = _gt_setHeader(HEADER_DA10F1_ID, HEADER_DA10F1);
        if (headerSet != HEADER_DA10F1_ID)
        {
          continue;
        }
        else
        {
          _currentHeaderId = HEADER_DA10F1_ID;
        }
      }

      if (_currentHeaderId == HEADER_DA10F1_ID)
      {
        if (millis() > _mainGearLastQueringMs)
        {
          _gear = (int)_gt_processPID(0x22, 0x192D, 1, 1, 1.0, 0);
          if (_gear != -1)
          {
            if (!_gotDataOnce)
            {
              _gotDataOnce = true;
              _rtcGotDataOnce = true;
            }
            _mainGearLastQueringMs = millis() + RT_SENSORS_DELAY;
          }
        }

        if (millis() > _mainBoostLastQueringMs)
        {
          _boost = (int)_gt_processPID(0x22, 0x195A, 1, 2, 1.0, -32768.0f);
          if (_boost != -1)
          {
            _mainBoostLastQueringMs = millis() + RT_SENSORS_DELAY;
          }
        }

        if (millis() > _mainCoolantLastQueringMs)
        {
          _coolant = (int)_gt_processPID(0x22, 0x1003, 1, 2, 0.02, -40.0f);
          if (_coolant != -1)
          {
            _mainCoolantLastQueringMs = millis() + NON_RT_SENSORS_DELAY;
          }
        }

        if (millis() > _mainAirTempLastQueringMs)
        {
          _airTemp = (int)_gt_processPID(0x22, 0x1935, 1, 2, 0.02, -40.0f);
          if (_airTemp != -1)
          {
            _mainAirTempLastQueringMs = millis() + NON_RT_SENSORS_DELAY;
          }
        }

        if (millis() > _mainDPFClogLastQueringMs)
        {
          _dpfClogging = (int)_gt_processPID(0x22, 0x18E4, 1, 2, 0.015259022, 0);
          if (_dpfClogging != -1)
          {
            _mainDPFClogLastQueringMs = millis() + NON_RT_SENSORS_DELAY;
          }
        }
        if (millis() > _mainDPFTempLastQueringMs)
        {
          _dpfTemperature = (int)_gt_processPID(0x22, 0x18DE, 1, 2, 0.02, -40.0f);
          if (_dpfTemperature != -1)
          {
            _mainDPFTempLastQueringMs = millis() + NON_RT_SENSORS_DELAY;
          }
        }
        if (millis() > _mainDPFRegLastQueringMs)
        {
          _dpfRegeneration = (int)_gt_processPID(0x22, 0x380B, 1, 2, 0.001525902, 0);
          if (_dpfRegeneration != -1)
          {
            _mainDPFRegLastQueringMs = millis() + NON_RT_SENSORS_DELAY;
          }
        }
        if (millis() > _mainDPFDistLastQueringMs)
        {
          _dpfDistance = (int)_gt_processPID(0x22, 0x3807, 1, 3, 0.1, 0);
          if (_dpfDistance != -1)
          {
            _mainDPFDistLastQueringMs = millis() + NON_RT_SENSORS_DELAY;
          }
        }
      }
    }

    else if (_elmConnected && !_mainScreenActive && _valuesScreenActive)
    {
      if (millis() > _valuesNonRtLastQueringMs)
      {

        if (_currentHeaderId != HEADER_DA10F1_ID)
        {
          int headerSet = _gt_setHeader(HEADER_DA10F1_ID, HEADER_DA10F1);
          if (headerSet != HEADER_DA10F1_ID)
          {
            continue;
          }
          else
          {
            _currentHeaderId = HEADER_DA10F1_ID;
          }
        }

        if (_currentHeaderId == HEADER_DA10F1_ID)
        {
          _dpfCount = (int)_gt_processPID(0x22, 0x18A4, 1, 2, 1.0, 0);
          _batteryIBS = (int)_gt_processPID(0x22, 0x19BD, 1, 1, 1.0, 0);
          _engineOilDegradation = (int)_gt_processPID(0x22, 0x3813, 1, 2, 0.001525902, 0);
        }

        if (_currentHeaderId != HEADER_DAC7F1_ID)
        {
          int headerSet = _gt_setHeader(HEADER_DAC7F1_ID, HEADER_DAC7F1);
          if (headerSet != HEADER_DAC7F1_ID)
          {
            continue;
          }
          else
          {
            _currentHeaderId = HEADER_DAC7F1_ID;
          }
        }

        if (_currentHeaderId == HEADER_DAC7F1_ID)
        {
          _tireTempFL = _gt_processPID(0x22, 0x40B1, 1, 3, 0.0000305, 0.0f);
          _tireTempFR = _gt_processPID(0x22, 0x40B2, 1, 3, 0.0000305, 0.0f);
          _tireTempRL = _gt_processPID(0x22, 0x40B3, 1, 3, 0.0000305, 0.0f);
          _tireTempRR = _gt_processPID(0x22, 0x40B4, 1, 3, 0.0000305, 0.0f);
        }

        if (_currentHeaderId != HEADER_DA18F1_ID)
        {
          int headerSet = _gt_setHeader(HEADER_DA18F1_ID, HEADER_DA18F1);
          if (headerSet != HEADER_DA18F1_ID)
          {
            continue;
          }
          else
          {
            _currentHeaderId = HEADER_DA18F1_ID;
          }
        }

        if (_currentHeaderId == HEADER_DA18F1_ID)
        {
          _gearboxOilTemp = _gt_processPID(0x22, 0x04FE, 1, 1, 1.0, -40.0f);
        }

        if (_currentHeaderId != HEADER_DB33F1_ID)
        {
          int headerSet = _gt_setHeader(HEADER_DB33F1_ID, HEADER_DB33F1);
          if (headerSet != HEADER_DB33F1_ID)
          {
            continue;
          }
          else
          {
            _currentHeaderId = HEADER_DB33F1_ID;
          }
        }

        if (_currentHeaderId == HEADER_DB33F1_ID)
        {
          _batteryVoltage = _gt_processPID(0x01, 0x42, 1, 2, 0.001, 0);
        }

        if (_batteryIBS != -1 &&
            _engineOilDegradation != -1 &&
            _tireTempFL != -1 &&
            _tireTempFR != -1 &&
            _tireTempRL != -1 &&
            _tireTempRR != -1 &&
            _gearboxOilTemp != -1 &&
            _batteryVoltage != -1)
        {
          _valuesNonRtLastQueringMs = millis() + NON_RT_SENSORS_DELAY;
        }
      }
    }
  }
}

/**
 * start the ELM loop in the second core (not used by LVGL)
 */
void _gt_startElmLoop(char *mac)
{
  _mac = mac;

  int elmCore = (xPortGetCoreID() + 1) % 2;

  VLOG("Using core %d ELM loop", elmCore);

  xTaskCreatePinnedToCore(_gt_elmLoop, "_gt_elmLoop", 4096, NULL, 1, NULL, elmCore);
}

/**
 * callback when an ELM327 device if picked from the list, can be persisted to autmatically connected on boot
 */
void _gt_onDeviceSelected(char *name, char *mac, bool persist)
{
  lv_obj_t *mainScreen = gt_mainScreen(_onStateChange, _gt_onResetDevice);
  lv_scr_load_anim(mainScreen, LV_SCR_LOAD_ANIM_FADE_ON, 250, 500, true);

  if (persist)
  {
    LOG("Persisting device %s address %s", name, mac);
    File f = SPIFFS.open(DEVICE_FILE, "w");
    if (f)
    {
      f.print(mac);
      f.close();
    }
  }

  _gt_startElmLoop(mac);
}

/**
 * on bluetooth device found callback
 */
void _gt_btAdvertisedDeviceFound(BTAdvertisedDevice *pDevice)
{
  const char *cStrName = pDevice->getName().c_str();
  char *arrName = new char[pDevice->getName().length() + 1];
  strcpy(arrName, cStrName);

  BTAddress address = pDevice->getAddress();
  String mac = address.toString(true);
  char *arrMAC = new char[mac.length() + 1];
  strcpy(arrMAC, mac.c_str());

  BTDevice found = {.name = arrName, .mac = arrMAC, .cod = (int)pDevice->getCOD()};

  VLOG("Bluetooth device discovered: %s (%s)", arrName, arrMAC);

  _onDeviceDiscovered(found);
}

/**
 * start discovery of bluetooth device; onDeviceDiscovered be used to notify new device to the UI
 */
void _gt_discoverDevices(gt_onDeviceDiscovered onDeviceDiscovered)
{

  if (_btScanning)
  {
    return;
  }

  _onDeviceDiscovered = onDeviceDiscovered;

  SerialBT.begin(GT_SPLASH_MESSAGE, true);

  if (SerialBT.discoverAsync(_gt_btAdvertisedDeviceFound))
  {
    LOGS("Discovering bluetooth devices");
    _btScanning = true;
  }
  else
  {
    LOGS("Error starting bluetooth devices discovery");
  }
}

/**
 * LVGL UI init; if an ELM327 device address has been persisted then it will load main screen and start the loop
 */
void _lv_main()
{
  VLOGS("Starting UI...")
  lv_scr_load(gt_splashScreen(_rtcBootCount == 1));

  if (SPIFFS.exists(DEVICE_FILE))
  {

    VLOG("Using connection address from file %s", DEVICE_FILE);
    File f = SPIFFS.open(DEVICE_FILE, "r");
    if (f)
    {
      char *mac = strdup(f.readString().c_str());
      f.close();

      LOG("Connection address is %s", mac);
      lv_obj_t *mainScreen = gt_mainScreen(_onStateChange, _gt_onResetDevice);

      lv_scr_load_anim(mainScreen, LV_SCR_LOAD_ANIM_FADE_ON, 250, 500, true);
      _gt_startElmLoop(mac);
    }
    else
    {
      LOG("Can't read connection address from file %s", DEVICE_FILE);
      lv_obj_t *devicesScreen = gt_devicesScreen(_gt_discoverDevices, _gt_onDeviceSelected);

      lv_scr_load_anim(devicesScreen, LV_SCR_LOAD_ANIM_FADE_ON, 250, 500, true);
    }
  }
  else
  {

    lv_obj_t *devicesScreen = gt_devicesScreen(_gt_discoverDevices, _gt_onDeviceSelected);

    lv_scr_load_anim(devicesScreen, LV_SCR_LOAD_ANIM_FADE_ON, 250, 500, true);
  }
}

/**
 * touch screen calibarion (first boot)
 */
void _touch_calibrate()
{
  VLOGS("Calibrating TFT");
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin())
  {
    LOGS("Init SPIFFS");
    SPIFFS.format();
    SPIFFS.begin();
  }

  if (SPIFFS.exists(CALIBRATION_FILE))
  {
    VLOG("Using calibration data from file %s", CALIBRATION_FILE);
    File f = SPIFFS.open(CALIBRATION_FILE, "r");
    if (f)
    {
      if (f.readBytes((char *)calData, 14) == 14)
        calDataOK = 1;
      f.close();
    }
  }

  if (calDataOK)
  {
    tft.setTouch(calData);
    tft.fillScreen(TFT_BLACK);
    _lv_main();
  }
  else
  {
    LOGS("Invalid or missing calibration data");
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f)
    {
      VLOG("Saving calibration data to file %s", CALIBRATION_FILE);
      f.write((const unsigned char *)calData, 14);
      f.close();
    }

    tft.fillScreen(TFT_BLACK);
    _lv_main();
  }
}

/**
 * verbose logging of boot/wakup cause
 */
void _print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    VLOGS("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    VLOGS("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    VLOGS("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    VLOGS("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    VLOGS("Wakeup caused by ULP program");
    break;
  default:
    VLOG("Wakeup was not caused by deep sleep: %d", wakeup_reason);
    break;
  }
}

/**
 * do setup the app:
 * - init serial logging port
 * - init LVGL
 * - init TFT display
 */
void setup()
{
  _rtcBootCount++;
  Serial.begin(115200);
  LOG_INIT(&Serial);

  _print_wakeup_reason();

  VLOGS("Init LVGL")
  lv_init();

  VLOGS("Setup TFT")
  tft.begin();
  tft.setRotation(GT_ROTATION);

  lv_disp_draw_buf_init(&_draw_buf, _screen_buffer, NULL, GT_SCREEN_WIDTH * GT_SCREEN_HEIGTH / 13);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = GT_SCREEN_WIDTH;
  disp_drv.ver_res = GT_SCREEN_HEIGTH;
  disp_drv.flush_cb = _dispFlush;
  disp_drv.draw_buf = &_draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = _touchpadRead;
  lv_indev_drv_register(&indev_drv);

  delay(100);
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);
  tft.fillScreen(TFT_BLACK);

#if LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

  tft.setFreeFont(FMB18);

  _touch_calibrate();
}

/**
 * main app loop
 */
void loop()
{
  lv_task_handler();

  delay(LOOP_DELAY);

  if (_btScanning)
  {
    _btScanDurationMs += LOOP_DELAY;

    // dummy impl, we do close the message box once discovered is stopped - no smart callback handling :)
    if (_btScanDurationMs > BT_DISCOVER_TIME)
    {
      LOGS("Stopping BT discovery")
      SerialBT.discoverAsyncStop();
      SerialBT.end();
      _btScanning = false;
      _btScanDurationMs = 0;
    }
  }

  if (_elmConnected)
  {
    // do set values; gt_* functions do handle the logic about "if is value changed then redraw"
    if (_mainScreenActive)
    {
      gt_setGear(_gear);
      gt_setBoost(_boost);
      gt_setCoolant(_coolant);
      gt_setAirTemp(_airTemp);
      gt_setDpfClogging(_dpfClogging);
      gt_setDpfTemperature(_dpfTemperature);

#ifdef DPF_AUDIO_NOTIFICATION
      DacAudio.FillBuffer();
      if (_dpfRegeneration > 0 && !DacAudio.AlreadyPlaying(&Beep) && _shouldPlayDPFSound) {
        _shouldPlayDPFSound = false;
        DacAudio.DacVolume = 100;
        DacAudio.Play(&Beep, false);
      } else if (_dpfRegeneration == 0) {
        _shouldPlayDPFSound = true;
      }
#endif

      gt_setDpfRegeneration(_dpfRegeneration);
      gt_setDpfDistance(_dpfDistance);
    }
    else if (_valuesScreenActive)
    {
      gt_setDpfCount(_dpfCount);
      gt_setBatteryVoltage(_batteryVoltage);
      gt_setBatteryIBS(_batteryIBS);
      gt_setEngineOilDegradation(_engineOilDegradation);
      gt_setTireTemperatureFL(_tireTempFL);
      gt_setTireTemperatureFR(_tireTempFR);
      gt_setTireTemperatureRL(_tireTempRL);
      gt_setTireTemperatureRR(_tireTempRR);
      gt_setGearboxOilTemp(_gearboxOilTemp);
    }
  }
}
#else
#include <stdio.h>
#include <stdlib.h>
#include <lvgl.h>
#include "app_hal.h"
#include "screens/gt_splash.h"
#include "screens/gt_devices.h"
#include "screens/gt_main.h"

static bool _mainScreenActive = false;
static bool _valuesScreenActive = false;

void _onStateChange(int screenId, bool active)
{
  printf("_onStateChange %d %s\n\r", screenId, active ? "active" : "paused");
  if (screenId == SCREEN_MAIN_ID)
  {
    _mainScreenActive = active;
  }
  else if (screenId == SCREEN_VALUES_ID)
  {
    _valuesScreenActive = active;
  }
}

void _onResetDevice(bool hard)
{
  printf("_onResetDevice %d\n\r", hard);
}

void _gt_onDeviceSelected(char *name, char *mac, bool persist)
{
  printf("%s - %s", name, mac);
  lv_obj_t *mainScreen = gt_mainScreen(_onStateChange, _onResetDevice);
  lv_scr_load_anim(mainScreen, LV_SCR_LOAD_ANIM_FADE_ON, 125, 500, true);
}

void _gt_discoverDevices(gt_onDeviceDiscovered onDeviceDiscovered)
{
  char *s0 = strdup("BT device 0");
  char *m0 = strdup("00:00:00:00:00");
  BTDevice first = {.name = s0, .mac = m0, .cod = 1234};
  char *s1 = strdup("BT device 1");
  char *m1 = strdup("00:00:00:00:01");
  BTDevice second = {.name = s1, .mac = m1, .cod = 1234};

  onDeviceDiscovered(first);
  onDeviceDiscovered(second);
}

// fake
static void _demo_m(lv_timer_t *timer)
{
  if (!_mainScreenActive)
  {
    return;
  }

  int boost = (int)(((float)rand() / (float)(RAND_MAX)) * 1000) - 500;
  gt_setBoost(2000 + boost);

  int gear = (int)(((float)rand() / (float)(RAND_MAX)) * 11);
  gt_setGear(gear);

  int celsius = (int)(((float)rand() / (float)(RAND_MAX)) * 130);
  gt_setCoolant(celsius);

  celsius = (int)(((float)rand() / (float)(RAND_MAX)) * 50) - 10;
  gt_setAirTemp(celsius);

  int percent = 75 + (int)(((float)rand() / (float)(RAND_MAX)) * 10);
  gt_setDpfClogging(percent);

  celsius = 500 + (int)(((float)rand() / (float)(RAND_MAX)) * 100);
  gt_setDpfTemperature(celsius);

  percent = 45 + (int)(((float)rand() / (float)(RAND_MAX)) * 10);
  gt_setDpfRegeneration(percent > 50 ? percent : 0);

  int distance = 300 + (int)(((float)rand() / (float)(RAND_MAX)) * 50);
  gt_setDpfDistance(distance);
}

static void _demo_v(lv_timer_t *timer)
{
  if (!_valuesScreenActive)
  {
    return;
  }

  int count = 10 + (int)(((float)rand() / (float)(RAND_MAX)) * 10);
  gt_setDpfCount(count);

  float volt = 12.0f + (((float)rand() / (float)(RAND_MAX)) * 2);
  gt_setBatteryVoltage(volt);

  int percent = 80 + (int)(((float)rand() / (float)(RAND_MAX)) * 15);
  gt_setBatteryIBS(percent);

  percent = 80 + (int)(((float)rand() / (float)(RAND_MAX)) * 15);
  gt_setEngineOilDegradation(percent);

  int celsius = 60 + (int)(((float)rand() / (float)(RAND_MAX)) * 30);
  gt_setTireTemperatureFL(celsius);
  gt_setTireTemperatureFR(celsius + 1);
  gt_setTireTemperatureRL(celsius + 2);
  gt_setTireTemperatureRR(celsius + 3);

  celsius = 60 + (int)(((float)rand() / (float)(RAND_MAX)) * 20);
  gt_setGearboxOilTemp(celsius);
}
// fake

int main(void)
{
  lv_init();

  hal_setup();

  bool demo = true;
  bool quick = false;

  lv_scr_load(gt_splashScreen(quick));

  lv_obj_t *screen = quick ? gt_mainScreen(_onStateChange, _onResetDevice) : gt_devicesScreen(_gt_discoverDevices, _gt_onDeviceSelected);

  lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_ON, 125, 500, true);

  if (demo)
  {
    gt_setBoost(2000);
    lv_timer_t *timerA = lv_timer_create(_demo_m, 1500, NULL);
    lv_timer_t *timerB = lv_timer_create(_demo_v, 1500, NULL);
  }

  hal_loop();
}
#endif