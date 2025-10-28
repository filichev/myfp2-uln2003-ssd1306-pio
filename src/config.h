//----------------------------------------------------------------------
// myFocuserPro2 Focuser Config File ULN2003
// (c) R Brown, 2014-2024, All rights reserved.
//----------------------------------------------------------------------
#if !defined(config_h)
#define config_h

#include <Arduino.h>
#include "myBoardDefs.h"
#include "defines.h"


//-----------------------------------------------------------------------
// WARNING: SKETCH AND GLOBAL VARIABLE SIZE LIMITS
//-----------------------------------------------------------------------
// When enabling options in this file, you will need to try and keep the 
// Arduino sketch and Global variables to allow the Arduino chip to 
// work without running out of space, which results in random reboots and 
// the Serial interface not working.
// 
// When the sketch is compiled, these messages are found in the Output 
// Window and looks like
//      Sketch uses 26548 bytes (86%) of program storage space. Maximum is 30720 bytes.
//      Global variables use 973 bytes (47%) of dynamic memory
//
// The sketch usage should be less that 83% most of the time (varies on 
// what is enabled.) Higher sketch usage will result in errors and 
// unexpected values being returned.
//
// The Global variables should be less than 60-70% (if debugging is enabled
// then the Global variable usage will exceed the safety limit and make 
// the sketch unusable). 
// 
// If either these are too high, you will need to disable an option to 
// decrease the Sketch size and Global var size to recommended levels
// 
// Refer to the spreadsheet Options Program Size for more information


//-----------------------------------------------------------------------
// BOARDS SUPPORTED
//-----------------------------------------------------------------------
// PCB  ULN2003_M_MT_F_BT
// BT.BUZZ.LEDs.LCD.OLED.HPSW.PB.TMP.SPWR

// PCB  ULN2003_NOKIA
// BUZZ.LEDs.NOKIA.PB.TMP.SPWR

// PCB  ULN2003_M_MT_F_BT_IR_RE_BT
// BT.BUZZ.LEDs.LCD.OLED.HPSW.PB.TMP.SPWR.IR.RE

// PCB  ULN2003_M_MT_F
// BUZZ.LEDs.LCD.OLED.PB.TMP


//-----------------------------------------------------------------------
// BOARDS
// Enable ONE of the following boards
//-----------------------------------------------------------------------
#define DRVBRD  ULN2003_M_MT_F_BT
// PCB: BT.BUZZ.LEDs.LCD.OLED.HPSW.PB.TMP.SPWR

//#define DRVBRD  ULN2003_NOKIA
// PCB: BUZZ.LEDs.NOKIA.PB.TMP.SPWR

//#define DRVBRD  ULN2003_M_MT_F_BT_IR_RE_BT
// BT.BUZZ.LEDs.LCD.OLED.HPSW.PB.IR.RE.TMP.SPWR

//#define DRVBRD  ULN2003_M_MT_F
// Stripboard: BUZZ.LEDs.LCD.OLED.PB.TMP


//-----------------------------------------------------------------------
// INPUT DEVICES
// HOME POSITION SWITCH, PUSH BUTTONS, STEPPER POWER DETECT, TEMPERATURE PROBE
//-----------------------------------------------------------------------
// do NOT uncomment HOMEPOSITIONSWITCH if you do not have the switch fitted
// To enable the HOMEPOSITION SWITCH, uncomment the next line
//#define HOMEPOSITIONSWITCH 1

// To enable the Push Buttons for manual focusing, uncomment the next line
#define PUSHBUTTONS 1

// To specify the number of motor steps to move for 1 push button press
// change the 1 value for PB_STEPS below. The max value should be 1/2 the
// focuser step size, [1-255 is the range]. If you set PB_STEPS to 0
// then the push buttons will not move the focuser.
#define PB_STEPS 1

// Push button long-press speed boost settings
// To disable the speed boost feature, comment out the next line
#define PB_SPEED_BOOST 1


// To specify the hold time threshold in milliseconds before the speed boost activates
#define PB_LONGPRESS_MS 1000

// To specify the speed multiplier during boost (higher = faster)
#define PB_SPEED_BOOST_MULTIPLIER 200

// To specify the minimum step delay in microseconds during boost (safety floor)
#define PB_BOOST_MIN_DELAY_US 500

// This prevents the stepper motor moving when 12V to the stepper is OFF - needs special circuitry or has no effect
// To enable the 12V power detect to the stepper motor, uncomment the next line  (only available on some boards)
//#define STEPPERPWRDETECT 1

// To enable the temperature probe, uncomment next line
#define TEMPERATURE_PROBE 1

// To enable a Rotary encoder. uncomment the next line (only available on some boards)
//#define ROTARYENCODER 1

// To enable an Infra-red Remote, uncomment the next line (only available on some boards)
//#define INFRARED 1


//-----------------------------------------------------------------------
// OUTPUT DEVICES
// BUZZER AND LEDS
//-----------------------------------------------------------------------
// Buzzer is used as a power on boot test, and with push-buttons as a
// feedback for push button operation
// To enable the buzzer, uncomment the next line
//#define BUZZER 1

// To enable the IN-OUT LEDS, uncomment the next line
//#define INOUTLEDS 1


//-----------------------------------------------------------------------
// CONTROLLER CONNECTION OPTIONS: BLUETOOTH, CONTROLLERISAMICRO
//-----------------------------------------------------------------------
// To enable bluetooth, uncomment the next line  (only available on some boards)
//#define BLUETOOTH 1

// provided by IL, enables the serialEventRun function for a Micro
// To enable support when usng a Micro instead of a Nano, uncomment the next line
//#define CONTROLLERISAMICRO 1


//-----------------------------------------------------------------------
// SOFTWARE OPTIONS
//-----------------------------------------------------------------------
// To enable the super slow jogging, uncomment the next line
//#define SUPERSLOWJOG 1


//-----------------------------------------------------------------------
// LCD LIQUID CRYSTAL DISPLAY, I2C
//-----------------------------------------------------------------------
// Uncomment one of the following LCDxxxx lines depending upon your lcd type
//#define DISPLAYTYPE  DISPLAY_LCD1602 // 16 character, 2 lines
//#define DISPLAYTYPE  DISPLAY_LCD1604 // 16 character, 4 lines
//#define DISPLAYTYPE  DISPLAY_LCD2004 // 20 character, 4 lines


//-----------------------------------------------------------------------
// OLED DISPLAY I2C
//-----------------------------------------------------------------------
// Uncomment the next line if using an OLED 128x54 I2C Display
#define DISPLAYTYPE DISPLAY_OLED12864 // 128x64, 0.96" 16 character, 8 lines

// Select the correct driver chip for the OLED 
#define USE_SSD1306 1  // For the OLED 128x64 0.96" display using the SSD1306 driver, uncomment this line
//#define USE_SSH1106 2 // For the OLED 128x64 1.3" display using the SSH1106 driver, uncomment this line


//-----------------------------------------------------------------------
// NOKIA DISPLAY 5110
//-----------------------------------------------------------------------
//#define DISPLAYTYPE  DISPLAY_NOKIA


//-----------------------------------------------------------------------
// USER CONFIGURATION END: DO NOT EDIT BELOW THIS LINE
//-----------------------------------------------------------------------

// check DRVBRD
#if !defined(DRVBRD)
#error "DRVBRD is not defined in config.h"
#endif

// PB STEPS
#if !defined(PB_STEPS)
#define PB_STEPS 1
#endif 

// PB SPEED BOOST
#if !defined(PB_SPEED_BOOST)
#define PB_SPEED_BOOST 1
#endif

// PB LONGPRESS MS
#if !defined(PB_LONGPRESS_MS)
#define PB_LONGPRESS_MS 1000
#endif

// PB SPEED BOOST MULTIPLIER
#if !defined(PB_SPEED_BOOST_MULTIPLIER)
#define PB_SPEED_BOOST_MULTIPLIER 10
#endif

// PB BOOST MIN DELAY US
#if !defined(PB_BOOST_MIN_DELAY_US)
#define PB_BOOST_MIN_DELAY_US 800
#endif


// PCB: BT.BUZZ.LEDs.LCD.OLED.HPSW.PB.TMP.SPWR
#if (DRVBRD == ULN2003_M_MT_F_BT)
#if (DISPLAYTYPE == DISPLAY_NOKIA)
#error  "DISPLAY_NOKIA NOT SUPPORTED"
#endif
#endif


// PCB: BUZZ.LEDs.NOKIA.PB.TMP.SPWR
#if (DRVBRD == ULN2003_NOKIA)
#if (DISPLAYTYPE == DISPLAY_LCD1602) || (DISPLAYTYPE == DISPLAY_LCD1604) || (DISPLAYTYPE == DISPLAY_LCD2004) || (DISPLAYTYPE == DISPLAY_OLED12864) || (DISPLAYTYPE == DISPLAY_TFT) || (DISPLAYTYPE == DISPLAY_UTFT)
#error  "LCDxxx NOT SUPPORTED"
#endif
#if (DISPLAYTYPE == DISPLAY_OLED12864)
#error  "DISPLAY_OLED12864 NOT SUPPORTED"
#endif
#if defined(BLUETOOTH)
#error  "BLUETOOTH NOT SUPPORTED"
#endif
#if defined(HOMEPOSITIONSWITCH)
#error  "HOMEPOSITIONSWITCH NOT SUPPORTED"
#endif
#if defined(ROTARYENCODER)
#error  "ROTARYENCODER NOT SUPPORTED"
#endif
#if defined(INFRARED)
#error  "INFRARED NOT SUPPORTED"
#endif
#endif


// PCB: BT.BUZZ.LEDs.LCD.OLED.HPSW.PB.TMP.SPWR.IR.RE
#if (DRVBRD == ULN2003_M_MT_F_BT_IR_RE_BT)
#if (DISPLAYTYPE == DISPLAY_NOKIA)
#error  "DISPLAY_NOKIA NOT SUPPORTED"
#endif
#if defined(ROTARYENCODER)
#if defined(BLUETOOTH)
#error "BLUETOOTH NOT SUPPORTED WHEN ROTARYENCODER IS ENABLED"
#endif
#if defined(INFRARED)
#error  "INFRARED NOT SUPPORTED WHEN ROTARYENCODER IS ENABLED"
#endif
#endif // RE
#endif // DRV

// Stripboard: BUZZ.LEDs.LCD.OLED.PB.TMP
#if (DRVBRD == ULN2003_M_MT_F)
#if defined(ROTARYENCODER)
#error "ROTARYENCODER NOT SUPPORTED"
#endif
#if defined(INFRARED)
#error  "INFRARED NOT SUPPORTED"
#endif
#if defined(BLUETOOTH)
#error  "BLUETOOTH NOT SUPPORTED"
#endif
#if defined(STEPPERPWRDETECT)
#error  "STEPPERPWRDETECT NOT SUPPORTED"
#endif
#if defined(HOMEPOSITIONSWITCH)
#error  "HOMEPOSITIONSWITCH NOT SUPPORTED"
#endif
#if (DISPLAYTYPE == DISPLAY_NOKIA)
#error  "DISPLAY_NOKIA NOT SUPPORTED"
#endif
#endif

#endif

