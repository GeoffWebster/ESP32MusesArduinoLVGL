/* ESP32 +TFT Pre-amp Controller with WiFi
*********************

Author Geoff Webster

ESP32/TFT + Wifi Current Ver 2.0  15 February 2024
- Addition of Web OTA function (uses ElegantOTA library)
- Correction of setIO and sourceUpdate routines to ensure Web interface indicates current selected source
- Addition of WiFi remote control for volume, source and mute
- Addition of clock display
- Amended setVolume routine to display atten/gain (-112dB min to +15.5dB). Previously displayed 0 -255.

ESP32/TFT + Wifi version 1.0 11 February 2024
- Migrated from Arduino + 4x20 LCD version to ESP32 microcontroller and 320x240 TFT colour display.
- Addition of WiFi remote control
- Amended RC coding for backlight so only toggles backlight (leaving source, volume and mute unchanged)
- Replaced original rotary encoder library with ESP32RotaryEncoder library
- Addition of KnobCallback and ButtonCallback routines to interface with new encoder library and
- Amended VolumeUpdate and sourceUpdate procedures to use above new rotary routines
- Deleted balance function

Arduino + 4x20 LCD version 3.0 Date	14 July 2023
- Changed mas6116::mas6116 construct in mas6116.cpp so that MUTE pin is initialized LOW
  Ensures MUTE remains LOW for two seconds after power startup

Arduino + 4x20 LCD version 2.0 Date	27 April 2022
- Changed mute pin to match new Controller board v2.0 (mutePin = 9). Mute pin used previously was A2 (on Ver 1.0 board)
- Added code to setup() routine which displays the SW version for two seconds at startup

*/

#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <RC5.h>
#include <mas6116.h> // Hardware-specific library
#include <ESP32RotaryEncoder.h>
#include <MCP23S08.h> // Hardware-specific library
#include <LittleFS.h>
#include <FS.h>
#include <WiFi.h>
#include "time.h"
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "Free_Fonts.h" // Include the Free fonts header file

// Current software
#define softTitle1 "ESP32/TFT"
#define softTitle2 "Pre-amp Controller"
//version number
#define VERSION_NUM "2.0"

/******* MACHINE STATES *******/
#define STATE_BALANCE 1 // when user adjusts balance
#define STATE_RUN 0     // normal run state
#define STATE_IO 1      // when user selects input/output
#define STATE_OFF 4     // when power down
#define ON LOW
#define OFF HIGH
#define STANDBY 0 // Standby
#define ACTIVE 1  // Active

// Preference modes
#define RW_MODE false
#define RO_MODE true

#define TIME_EXITSELECT 5 //** Time in seconds to exit I/O select mode when no activity

Preferences preferences;

// 23S08 Construct
MCP23S08 MCP(10); //  HW SPI address 0x00, CS GPIO10

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// define IR input
unsigned int IR_PIN = 27;
// RC5 construct
RC5 rc5(IR_PIN);

// define preAmp control pins
const int mutePin = 17;
const int mas_CS = 16;

// preAmp construct
mas6116 preamp(mutePin, mas_CS);

// define encoder pins
const uint8_t DI_ENCODER_A = 33;
const uint8_t DI_ENCODER_B = 32;
const int8_t DI_ENCODER_SW = 12;

// Network credentials
const char *ssid = "PLUSNET-9FC9NQ";
const char *password = "M93ucVcxRGCKeR";

// Web Server / WebSocket constructs

#define HTTP_PORT 80

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

// Rotary construct
RotaryEncoder rotaryEncoder(DI_ENCODER_A, DI_ENCODER_B, DI_ENCODER_SW);

/******* TIMING *******/
unsigned long milOnButton;  // Stores last time for switch press
unsigned long milOnAction;  // Stores last time of user input
unsigned long milOnFadeIn;  // LCD fade timing
unsigned long milOnFadeOut; // LCD fade timing

/********* Global Variables *******************/
uint8_t volume;   // current volume, between 0 and VOL_STEPS
uint8_t vol2;
uint8_t leftVol;  // current left volume
uint8_t rightVol; // current right volume
bool backlight;   // current backlight state
uint16_t counter = 0;
uint8_t source;        // current input channel
uint8_t oldsource = 1; // previous input channel
bool isMuted;          // current mute status
uint8_t state = 0;     // current machine state
uint8_t balanceState;  // current balance state
bool btnstate = 0;
bool oldbtnstate = 0;
int lastSeconds = 0;    // last seconds
int currentSeconds = 0; // current seconds

/*System addresses and codes used here match RC-5 infra-red codes for amplifiers (and CDs)*/
uint16_t oldtoggle;
u_char oldaddress;
u_char oldcommand;
u_char toggle;
u_char address;
u_char command;

// Used to know when to fire an event when the knob is turned
volatile bool turnedRightFlag = false;
volatile bool turnedLeftFlag = false;

char buffer1[20] = "";

// Global Constants
//------------------
const char *inputName[] = {"  Phono ", "   Media  ", "     CD    ", "   Tuner  "}; // Elektor i/p board
const int source_size = 6;
const int volume_size = 6;
const int source_x = 100;
const int source_y = 10;
const int volume_x = 100;
const int volume_y = 100;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

// Function prototypes
void RC5Update(void);
void setIO();
void knobCallback(long value);
void buttonCallback(unsigned long duration);
void RotaryUpdate();
void volumeUpdate();
void setVolume();
void sourceUpdate();
void mute();
void unMute();
void toggleMute();
void initLittleFS(void);
void initWiFi(void);
String processor(const String &var);
void onRootRequest(AsyncWebServerRequest *request);
void initWebServer(void);
void notifyClients(void);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server,
             AsyncWebSocketClient *client,
             AwsEventType type,
             void *arg,
             uint8_t *data,
             size_t len);
void initWebSocket(void);
void printLocalTime(void);
void setTimezone(String timezone);
void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst);
void initTime(String timezone);
void onOTAStart();
void onOTAProgress(size_t current, size_t final);
void onOTAEnd(bool success);

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
}

void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void initTime(String timezone){
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println("  Failed to obtain time");
    return;
  }
  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}

void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tm;

  tm.tm_year = yr - 1900;   // Set date
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0
  time_t t = mktime(&tm);
  Serial.printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  currentSeconds = timeinfo.tm_sec;
  if (lastSeconds != currentSeconds)
  {
    lastSeconds = currentSeconds;
    strftime (buffer1, 20, "  %H:%M:%S  ", &timeinfo);
    //tft.setFreeFont(FSS18);
    tft.drawString(buffer1, 160, 40, 1);
    //tft.setFreeFont(FSS24);
  }
}

// ----------------------------------------------------------------------------
// LittleFS initialization
// ----------------------------------------------------------------------------

void initLittleFS()
{
  if (!LittleFS.begin())
  {
    Serial.println("Cannot mount LittleFS volume...");
    while (1)
    {
      // onboard_led.on = millis() % 200 < 50;
      // onboard_led.update();
    }
  }
}

// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------
String processor(const String &var)
{
  if (var == "VOLUME")
  {
    return String(volume);
  }
  if (var == "SOURCE")
  {
    return String(inputName[source - 1]);
  }
  if (var == "STATE1")
  {
    return String(String(var == "STATE1" && isMuted ? "on" : "off"));
  }
  if (var == "STATE2")
  {
    return String(String(var == "STATE2" && isMuted ? "off" : "on"));
  }
  return String();
}

void onRootRequest(AsyncWebServerRequest *request)
{
  request->send(LittleFS, "/index.html", "text/html", false, processor);
}

void initWebServer()
{
  server.on("/", onRootRequest);
  server.serveStatic("/", LittleFS, "/");
  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  server.begin();
}

// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------

void notifyClients()
{
  JsonDocument json;
  json["source"] = inputName[source - 1];
  json["volume"] = volume;
  json["mute"] = isMuted ? "on" : "off";
  // serializeJson(json, Serial);
  // Serial.println();
  char buffer[50];
  size_t len = serializeJson(json, buffer);
  ws.textAll(buffer, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, data);
    if (err)
    {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
      return;
    }
    if (json["Select"])
    {
      const char *sel = json["Select"];
      if (strcmp(sel, "toggle") == 0)
      {
        oldsource = source;
        if (oldsource < 4)
        {
          source++;
        }
        else
        {
          source = 1;
        }
        setIO();
      }
    }
    else if (json["Volup"])
    {
      const char *vol = json["Volup"];
      if (strcmp(vol, "toggle") == 0)
      {
        if (isMuted)
        {
          unMute();
        }
        if (volume < 255)
        {
          volume = volume + 1;
          setVolume();
        }
      }
    }
    else if (json["Voldown"])
    {
      const char *vol = json["Voldown"];
      if (strcmp(vol, "toggle") == 0)
      {
        if (isMuted)
        {
          unMute();
        }
        if (volume > 0)
        {
          volume = volume - 1;
          setVolume();
        }
      }
    }
    else if (json["Mute"])
    {
      const char *mut = json["Mute"];
      if (strcmp(mut, "toggle") == 0)
      {
        toggleMute();
      }
    }
  }
}

void onEvent(AsyncWebSocket *server,
             AsyncWebSocketClient *client,
             AwsEventType type,
             void *arg,
             uint8_t *data,
             size_t len)
{

  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void knobCallback(long value)
{
  // See the note in the `loop()` function for
  // an explanation as to why we're setting
  // boolean values here instead of running
  // functions directly.

  // Don't do anything if either flag is set;
  // it means we haven't taken action yet
  if (turnedRightFlag || turnedLeftFlag)
    return;

  // Set a flag that we can look for in `loop()`
  // so that we know we have something to do
  switch (value)
  {
  case 1:
    turnedRightFlag = true;
    break;

  case -1:
    turnedLeftFlag = true;
    break;
  }

  // Override the tracked value back to 0 so that
  // we can continue tracking right/left events
  rotaryEncoder.setEncoderValue(0);
}

void buttonCallback(unsigned long duration)
{
  int _duration = duration;
  if (_duration > 50)
  {
    switch (state)
    {
    case STATE_RUN:
      state = STATE_IO;
      milOnButton = millis();
      break;
    default:
      break;
    }
  }
}

void volumeUpdate()
{
  if (turnedRightFlag)
  {
    if (isMuted)
    {
      unMute();
    }
    if (volume < 255)
    {
      volume = volume + 1;
      setVolume();
    }
    // Set flag back to false so we can watch for the next move
    turnedRightFlag = false;
  }
  else if (turnedLeftFlag)
  {
    if (isMuted)
    {
      unMute();
    }
    if (volume != 0)
    {
      volume = volume - 1;
      setVolume();
    }
    // Set flag back to false so we can watch for the next move
    turnedLeftFlag = false;
  }
}

void setVolume()
{
  // set new volume setting
  preamp.mas6116Write(mas6116RegBoth, volume);
  preferences.putUInt("VOLUME", volume);
  // display volume setting
  if (!backlight)
  {
    backlight = ACTIVE;
    digitalWrite(TFT_BL, HIGH); // Turn on backlight
  }
  float atten = ((float)volume / 2)-112;
  sprintf(buffer1, "    %.1f dB    ", atten);
  tft.setTextSize(2);
  tft.setFreeFont(FSS18);
  tft.drawString(buffer1, 150, 120, 1);
  tft.setTextSize(1);
  tft.setFreeFont(FSS24);
  notifyClients();
}

void sourceUpdate()
{
  if (turnedRightFlag)
  {
    oldsource = source;
    milOnButton = millis();
    if (oldsource < 4)
    {
      source++;
    }
    else
    {
      source = 1;
    }
    setIO();
    // Set flag back to false so we can watch for the next move
    turnedRightFlag = false;
  }
  else if (turnedLeftFlag)
  {
    oldsource = source;
    milOnButton = millis();
    if (source > 1)
    {
      source--;
    }
    else
    {
      source = 4;
    }
    if (!backlight)
    {
      backlight = ACTIVE;
      digitalWrite(TFT_BL, HIGH); // Turn on backlight
    }
    setIO();
    // Set flag back to false so we can watch for the next move
    turnedLeftFlag = false;
  }
}

void RC5Update()
{
  /*
  System addresses and codes used here match RC-5 infra-red codes for amplifiers (and CDs)
  */
  u_char toggle;
  u_char address;
  u_char command;
  // Poll for new RC5 command
  if (rc5.read(&toggle, &address, &command))
  {
    if (address == 0x10) // standard system address for preamplifier
    {
      switch (command)
      {
      case 1:
        // Phono
        if ((oldtoggle != toggle))
        {
          if (!backlight)
          {
            unMute(); // unmute output
          }
          oldsource = source;
          source = 1;
          setIO();
        }
        break;
      case 3:
        // Tuner
        if ((oldtoggle != toggle))
        {
          if (!backlight)
          {
            unMute(); // unmute output
          }
          oldsource = source;
          source = 4;
          setIO();
        }
        break;
      case 7:
        // CD
        if ((oldtoggle != toggle))
        {
          if (!backlight)
          {
            unMute(); // unmute output
          }
          oldsource = source;
          source = 3;
          setIO();
        }
        break;
      case 8:
        // Media
        if ((oldtoggle != toggle))
        {
          if (!backlight)
          {
            unMute(); // unmute output
          }
          oldsource = source;
          source = 2;
          setIO();
        }
        break;
      case 13:
        // Mute
        if ((oldtoggle != toggle))
        {
          toggleMute();
        }
        break;
      case 16:
        // Increase Vol
        if (isMuted)
        {
          unMute();
        }
        if (volume < 255)
        {
          volume = volume + 1;
          setVolume();
        }
        break;
      case 17:
        // Reduce Vol
        if (isMuted)
        {
          unMute();
        }
        if (volume != 0)
        {
          volume = volume - 1;
          setVolume();
        }
        break;
      case 59:
        // Display Toggle
        if ((oldtoggle != toggle))
        {
          if (backlight)
          {
            backlight = STANDBY;
            digitalWrite(TFT_BL, LOW); // Turn off backlight
            // mute();                    // mute output
          }
          else
          {
            backlight = ACTIVE;
            digitalWrite(TFT_BL, HIGH); // Turn on backlight
            // unMute(); // unmute output
          }
        }
        break;
      default:
        break;
      }
    }
    else if (address == 0x14) // system address for CD
    {
      if ((oldtoggle != toggle))
      {
        if (command == 53) // Play
        {
          if (!backlight)
          {
            unMute(); // unmute output
          }
          oldsource = source;
          source = 3;
          setIO();
        }
      }
    }
    oldtoggle = toggle;
  }
}

void unMute()
{
  if (!backlight)
  {
    backlight = ACTIVE;
    digitalWrite(TFT_BL, HIGH);
  }
  isMuted = 0;
  preamp.mas6116Mute(HIGH);
  //tft.fillScreen(TFT_WHITE);
  // set volume
  setVolume();
  // set source
  setIO();
  notifyClients();
}

void mute()
{
  isMuted = 1;
  preamp.mas6116Mute(LOW);
  tft.setTextSize(2);
  tft.setFreeFont(FSS18);
  tft.drawString("    Muted    ", 160, 120, 1);
  tft.setTextSize(1);
  tft.setFreeFont(FSS24);
  //tft.drawString("    Muted    ", 160, 120, 1);
  notifyClients();
}

void toggleMute()
{
  if (isMuted)
  {
    unMute();
  }
  else
  {
    mute();
  }
}

void RotaryUpdate()
{
  switch (state)
  {
  case STATE_RUN:
    volumeUpdate();
    break;
  case STATE_IO:
    sourceUpdate();
    if ((millis() - milOnButton) > TIME_EXITSELECT * 1000)
    {
      state = STATE_RUN;
    }
    break;
  default:
    break;
  }
}

void setIO()
{
  MCP.write1((oldsource - 1), LOW); // Reset source select to NONE
  MCP.write1((source - 1), HIGH);   // Set new source
  preferences.putUInt("SOURCE", source);
  if (isMuted)
  {
    if (!backlight)
    {
      backlight = ACTIVE;
      digitalWrite(TFT_BL, HIGH);
    }
    isMuted = 0;
    preamp.mas6116Mute(HIGH);
    tft.fillScreen(TFT_WHITE);
    // set volume
    setVolume();
  }
  notifyClients();
  tft.drawString(inputName[source - 1], 150, 200, 1);
}

// This section of code runs only once at start-up.
void setup()
{
  Serial.begin(115200);

  // This tells the library that the encoder has no pull-up resistors and to use ESP32 internal ones
  rotaryEncoder.setEncoderType(EncoderType::FLOATING);

  // The encoder will only return -1, 0, or 1, and will not wrap around.
  rotaryEncoder.setBoundaries(-1, 1, false);

  // The function specified here will be called every time the knob is turned
  // and the current value will be passed to it
  rotaryEncoder.onTurned(&knobCallback);

  // The function specified here will be called every time the button is pushed and
  // the duration (in milliseconds) that the button was down will be passed to it
  rotaryEncoder.onPressed(&buttonCallback);

  // This is where the rotary inputs are configured and the interrupts get attached
  rotaryEncoder.begin();
  
  initLittleFS();
  initWiFi();
  initWebSocket();
  initWebServer();

  // Init and get the time
  initTime("GMT0BST,M3.5.0/1,M10.4.0");   // Set for Europe / London
  
  // Initialise the TFT screen
  tft.init();
  tft.setRotation(1);

  // Set text datum to middle centre
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(FSS18);

  // Clear the screen
  tft.fillScreen(TFT_WHITE);
  // show software version briefly in display

  tft.setTextColor(TFT_BLUE, TFT_WHITE);
  tft.drawString(softTitle1, 160, 80, 1);
  tft.drawString(softTitle2, 160, 120, 1);
  tft.drawString("SW ver " VERSION_NUM, 160, 160, 1);
  delay(4000);
  tft.setFreeFont(FSS24);
  tft.fillScreen(TFT_WHITE);

  // This initialises the Source select pins as outputs, all deselected (i.e. o/p=low)
  MCP.begin();
  MCP.pinMode8(0x00); //  0 = output , 1 = input

  // Load saved settings (volume, balance, source)
  preferences.begin("settings", RW_MODE);
  source = preferences.getUInt("SOURCE", 1);
  volume = preferences.getUInt("VOLUME", 15);
  printLocalTime();
  // set startup volume
  setVolume();
  // set source
  setIO();
  // unmute
  isMuted = 0;
  preamp.mas6116Mute(HIGH);
}

void loop()
{
  ElegantOTA.loop();
  RC5Update();
  RotaryUpdate();
  printLocalTime();
}
