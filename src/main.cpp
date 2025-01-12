#include <Arduino.h>
#include <lvgl.h>
#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <RC5.h>
#include <Muses72323.h> // Hardware-specific library
#include <ESP32RotaryEncoder.h>
#include <MCP23S08.h>   // Hardware-specific library
<<<<<<< HEAD
=======
#include "Free_Fonts.h" // Include the Free fonts header file

// Current software
#define softTitle1 "ESP32/TFT/lvgl"
#define softTitle2 "Muses72323 Controller"
// version number
#define VERSION_NUM "1.0"
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e

/******* MACHINE STATES *******/
#define STATE_RUN 0 // normal run state
#define STATE_IO 1  // when user selects input/output
#define STATE_OFF 4 // when power down
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

// define IR input
unsigned int IR_PIN = 27;
// RC5 construct
RC5 rc5(IR_PIN);

// define preAmp control pins
const int s_select_72323 = 16;
//  The address wired into the muses chip (usually 0).
static const byte MUSES_ADDRESS = 0;

// preAmp construct
static Muses72323 Muses(MUSES_ADDRESS, s_select_72323); // muses chip address (usually 0), slave select pin0;

// define encoder pins
const uint8_t DI_ENCODER_A = 33;
const uint8_t DI_ENCODER_B = 32;
<<<<<<< HEAD
const int8_t DI_ENCODER_SW = 12;

/********* Global Variables *******************/

float atten;           // current attenuation, between 0 and -111.75
int16_t volume = -447; // current volume, between 0 and -447
bool backlight;        // current backlight state
uint16_t counter = 0;
uint8_t source;        // current input channel
=======
const int8_t DI_ENCODER_SW = 14;

/********* Global Variables *******************/

float atten;              // current attenuation, between 0 and -111.75
int16_t oldvolume = -447; // current volume, between 0 and -447
int16_t volume = -447;    // current volume, between 0 and -447
bool backlight;           // current backlight state
int16_t counter = 0;
uint8_t source = 1;    // current input channel
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e
uint8_t oldsource = 1; // previous input channel
bool isMuted;          // current mute status
uint8_t state = 0;     // current machine state
bool btnstate = 0;
bool oldbtnstate = 0;
<<<<<<< HEAD
=======
int splashcount;
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e

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
const char *inputName[] = {"Phono", "Media", "CD", "Tuner"}; // Elektor i/p board

// Rotary construct
RotaryEncoder rotaryEncoder(DI_ENCODER_A, DI_ENCODER_B, DI_ENCODER_SW);

/******* TIMING *******/
unsigned long milOnButton; // Stores last time for switch press
unsigned long milOnAction; // Stores last time of user input

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Function prototypes
void log_print(lv_log_level_t level, const char *buf);
<<<<<<< HEAD
void RC5Update(void);
void setIO();
=======
void setIO(void *text_label_source, int32_t v);
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e
void knobCallback(long value);
void buttonCallback(unsigned long duration);
void volumeUpdate();
static void set_volume(void *text_label_vol_value, int32_t v);
void sourceUpdate();
void RC5Update(void);
void unMute();
void mute();
void toggleMute();
<<<<<<< HEAD
=======
void RotaryUpdate();
void setIO(void *text_label_source, int32_t v);
void lv_create_splash_screen(void);
void lv_create_main_gui(void);
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char *buf)
{
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

void knobCallback(long value)
{
  // Don't do anything if either flag is set;
  // it means we haven't taken action yet
  if (turnedRightFlag || turnedLeftFlag)
    return;
  // Set a flag that we can look for in `loop()`
  // so that we know we have something to do
  counter = value;
  switch (value)
  {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
  case 10:
    turnedRightFlag = true;
    break;

  case -1:
  case -2:
  case -3:
  case -4:
  case -5:
  case -6:
  case -7:
  case -8:
  case -9:
  case -10:
    turnedLeftFlag = true;
    break;
  default:
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
    if (volume < 0)
    {
<<<<<<< HEAD
      volume = volume + 1;
=======
      volume = volume + counter;
    }
    if (volume > 0)
    {
      volume = 0;
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e
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
    if (volume > -447)
    {
<<<<<<< HEAD
      volume = volume - 1;
=======
      volume = volume + counter;
    }
    if (volume < -447)
    {
      volume - -447;
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e
    }
    // Set flag back to false so we can watch for the next move
    turnedLeftFlag = false;
  }
}

#define TEMP_ARC_MIN -112
#define TEMP_ARC_MAX 0
lv_obj_t *arc;

// Set the volume value in the bar and text label
static void set_volume(void *text_label_vol_value, int32_t v)
{
<<<<<<< HEAD
  Muses.setVolume(volume, volume);
  preferences.putInt("VOLUME", volume);
  // display volume setting
  if (!backlight)
=======
  // Don't send MUSES72323 data if no change to volume
  if (oldvolume != volume)
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e
  {
    Muses.setVolume(volume, volume);
    preferences.putInt("VOLUME", volume);
    oldvolume = volume;
    // Turn on display if volume changed
    if (!backlight)
    {
      backlight = ACTIVE;
      digitalWrite(TFT_BL, HIGH); // Turn on backlight
    }
  }
  float amp_vol = ((float)volume / 4);
<<<<<<< HEAD
  lv_obj_set_style_text_color((lv_obj_t *)text_label_vol_value, lv_palette_main(LV_PALETTE_INDIGO), 0);
  const char dB_symbol[] = "dB";
  lv_arc_set_value(arc, amp_vol);
  String amp_vol_text = String(amp_vol) + dB_symbol;
=======
  int amp_vol_int = volume / 4;
  const char dB_symbol[] = "dB";
  lv_arc_set_value(arc, float(amp_vol));
  String amp_vol_text = String(amp_vol_int) + dB_symbol;
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e
  lv_label_set_text((lv_obj_t *)text_label_vol_value, isMuted ? "Muted" : amp_vol_text.c_str());
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
    if (isMuted)
    {
      if (!backlight)
      {
        backlight = ACTIVE;
        digitalWrite(TFT_BL, HIGH);
      }
      isMuted = 0;
      Muses.setVolume(volume, volume);
    }
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
    if (isMuted)
    {
      if (!backlight)
      {
        backlight = ACTIVE;
        digitalWrite(TFT_BL, HIGH);
      }
      isMuted = 0;
      Muses.setVolume(volume, volume);
    }
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
    /* For Debug
    Serial.print("a:");
    Serial.print(address);
    Serial.print(" c:");
    Serial.print(command);
    Serial.print(" t:");
    Serial.println(toggle);*/

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
        // Increase Vol / reduce attenuation
        if (isMuted)
        {
          unMute();
        }
        if (volume < 0)
        {
          volume = volume + 1;
        }
        break;
      case 17:
        // Reduce Vol / increase attenuation
        if (isMuted)
        {
          unMute();
        }
        if (volume > -447)
        {
          volume = volume - 1;
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
  // set volume
  Muses.setVolume(volume, volume);
}

void mute()
{
  isMuted = 1;
  Muses.mute();
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
    /*
    if ((millis() - milOnButton) > TIME_EXITSELECT * 1000)
    {
      state = STATE_RUN;
    }*/
    break;
  default:
    break;
  }
}

void setIO(void *text_label_source, int32_t v)
{
<<<<<<< HEAD
  MCP.write1((oldsource - 1), LOW); // Reset source select to NONE
  MCP.write1((source - 1), HIGH);   // Set new source
  preferences.putUInt("SOURCE", source);
  lv_label_set_text((lv_obj_t *)text_label_source, inputName[source - 1]);
}

void lv_create_main_gui(void)
{
=======
  if (oldsource != source)
  {
    MCP.write1((oldsource - 1), LOW); // Reset source select to NONE
    MCP.write1((source - 1), HIGH);   // Set new source
    preferences.putUInt("SOURCE", source);
    //oldsource = source;
  }
  lv_label_set_text((lv_obj_t *)text_label_source, inputName[source - 1]);
}

static void splash_screen_cb(void *lv_screen_active, int32_t v)
{
  if (millis() > (splashcount + 3000))
  lv_create_main_gui();
  MCP.write1((source - 1), HIGH);  // Set source
  Muses.setVolume(volume, volume); // Set volume
  isMuted = 0;
}

void lv_create_splash_screen(void)
{
  lv_obj_t *header1 = lv_label_create(lv_screen_active());
  lv_label_set_text(header1, softTitle1);
  lv_obj_align(header1, LV_ALIGN_CENTER, 0, -30);
  lv_obj_t *header2 = lv_label_create(lv_screen_active());
  lv_label_set_text(header2, softTitle2);
  lv_obj_align(header2, LV_ALIGN_CENTER, 0, 0);
  lv_obj_t *header3 = lv_label_create(lv_screen_active());
  lv_label_set_text(header3, "version " VERSION_NUM);
  lv_obj_align(header3, LV_ALIGN_CENTER, 0, 30);
  lv_obj_set_style_text_font(header1, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_font(header2, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_font(header3, &lv_font_montserrat_24, 0);
  splashcount = millis();

  //Create an animation to close splash screen and create main GUI
  lv_anim_t splash;
  lv_anim_init(&splash);
  lv_anim_set_exec_cb(&splash, splash_screen_cb );
  lv_anim_set_duration(&splash, 4000);
  lv_anim_set_var(&splash, header1);
  lv_anim_set_values(&splash, 0, 100);
  lv_anim_start(&splash);
}

void lv_create_main_gui(void)
{
  lv_obj_clean(lv_scr_act());
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e
  // Create a text label to display the current Source
  lv_obj_t *text_label_source = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_source, "Phono");
  lv_obj_set_style_text_align(text_label_source, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(text_label_source, LV_ALIGN_CENTER, 0, -80);
  static lv_style_t style_source;
  lv_style_init(&style_source);
<<<<<<< HEAD
  lv_style_set_text_font(&style_source, &lv_font_montserrat_42);
  lv_obj_add_style(text_label_source, &style_source, 0);
  // Create an animation to update the text label with the latest source value every 10 seconds
=======
  lv_style_set_text_font(&style_source, &lv_font_montserrat_48);
  lv_obj_add_style(text_label_source, &style_source, 0);
  // Create an animation to update the text label with the latest source value every 10 milli-seconds
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e
  lv_anim_t a_source;
  lv_anim_init(&a_source);
  lv_anim_set_exec_cb(&a_source, setIO);
  lv_anim_set_duration(&a_source, 10);
  lv_anim_set_playback_duration(&a_source, 10);
  lv_anim_set_var(&a_source, text_label_source);
  lv_anim_set_values(&a_source, 0, 100);
  lv_anim_set_repeat_count(&a_source, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a_source);

  // Create an Arc
  arc = lv_arc_create(lv_screen_active());
  lv_obj_set_size(arc, 210, 210);
<<<<<<< HEAD
  lv_arc_set_rotation(arc, 180);
  lv_arc_set_bg_angles(arc, -30, 210);
  lv_arc_set_range(arc, TEMP_ARC_MIN, TEMP_ARC_MAX);
  lv_obj_set_style_arc_color(arc, lv_color_hex(0x666666), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(arc, lv_color_hex(0x333333), LV_PART_KNOB);
  lv_obj_align(arc, LV_ALIGN_BOTTOM_MID, 0, 40);

  // Create a text label in font size 36 to display the current volume
  lv_obj_t *text_label_vol_value = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_vol_value, "--.--");
  lv_obj_set_style_text_align(text_label_vol_value, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(text_label_vol_value, LV_ALIGN_CENTER, 0, 40);

  static lv_style_t style_vol;
  lv_style_init(&style_vol);
  lv_style_set_text_font(&style_vol, &lv_font_montserrat_32);
  lv_obj_add_style(text_label_vol_value, &style_vol, 0);

  // Create an animation to update the text label with the latest temperature value every 10 seconds
=======
  lv_arc_set_rotation(arc, 160);
  lv_arc_set_bg_angles(arc, 0, 220);
  lv_arc_set_range(arc, TEMP_ARC_MIN, TEMP_ARC_MAX);
  lv_obj_set_style_bg_color(arc, lv_color_hex(0xffff00), LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, lv_color_hex(0x0000ff), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(arc, lv_color_hex(0x0000ff), LV_PART_KNOB);
  lv_obj_align(arc, LV_ALIGN_CENTER, 0, 60);

  // Create a text label in font size 48 to display the current volume
  lv_obj_t *text_label_vol_value = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_vol_value, "--.--");

  lv_obj_set_style_text_align(text_label_vol_value, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color((lv_obj_t *)text_label_vol_value, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_align(text_label_vol_value, LV_ALIGN_CENTER, 0, 80);

  static lv_style_t style_vol;
  lv_style_init(&style_vol);
  lv_style_set_text_font(&style_vol, &lv_font_montserrat_48);
  lv_obj_add_style(text_label_vol_value, &style_vol, 0);

  // Create an animation to update the text label with the latest temperature value every 10 milli-seconds
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e
  lv_anim_t a_vol;
  lv_anim_init(&a_vol);
  lv_anim_set_exec_cb(&a_vol, set_volume);
  lv_anim_set_duration(&a_vol, 10);
  lv_anim_set_playback_duration(&a_vol, 10);
  lv_anim_set_var(&a_vol, text_label_vol_value);
  lv_anim_set_values(&a_vol, 0, 100);
  lv_anim_set_repeat_count(&a_vol, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a_vol);
}

void setup()
{
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);
  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // This tells the library that the encoder has no pull-up resistors and to use ESP32 internal ones
  rotaryEncoder.setEncoderType(EncoderType::FLOATING);

  // The encoder will only return -1, 0, or 1, and will not wrap around.
  rotaryEncoder.setBoundaries(-10, 10, false);

  // The function specified here will be called every time the knob is turned
  // and the current value will be passed to it
  rotaryEncoder.onTurned(&knobCallback);

  // The function specified here will be called every time the button is pushed and
  // the duration (in milliseconds) that the button was down will be passed to it
  rotaryEncoder.onPressed(&buttonCallback);

  // This is where the rotary inputs are configured and the interrupts get attached
  rotaryEncoder.begin();

  // This initialises the Source select pins as outputs, all deselected (i.e. o/p=low)
  MCP.begin();
  MCP.pinMode8(0x00); //  0 = output , 1 = input

  // Initialize muses (SPI, pin modes)...
  Muses.begin();
  Muses.setExternalClock(false); // must be set!
  Muses.setZeroCrossingOn(true);
  Muses.mute();
  // Load saved settings (volume, balance, source)
  preferences.begin("settings", RW_MODE);
  source = preferences.getUInt("SOURCE", 1);
  volume = preferences.getInt("VOLUME", -447);
  if (volume > 0)
    volume = -447;
  delay(10);

  // Create a display object
  lv_display_t *disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
<<<<<<< HEAD
  // Function to draw the GUI
  isMuted = 1;
  lv_create_main_gui();
=======

  // set the volume and source to stored values
  oldsource = source;
  oldvolume = volume;
  lv_create_splash_screen();
  isMuted = 1;
>>>>>>> 0c121cf88f444a5154308dc15444367c3999609e
}

void loop()
{
  RC5Update();
  RotaryUpdate();
  lv_task_handler(); // let the GUI do its work
  lv_tick_inc(5);    // tell LVGL how much time has passed
  delay(5);          // let this time pass
}