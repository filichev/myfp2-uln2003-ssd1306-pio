//-----------------------------------------------------------------------
// myFOCUSERPRO2 OFFICIAL FIRMWARE RELEASE 338 (11-Oct-2024)
// (c) Robert Brown 2014-2024. All Rights Reserved.
// (c) Holger, 2019-2022. All Rights Reserved.
// (c) Joel Collet, move-timer, 2021-2022, All rights reserved
// (c) Bojan Stajcar added Rotary Encoder support
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// BOARDS SUPPORTED
//-----------------------------------------------------------------------
// PCB ULN2003_M_MT_F_BT 
// BT.BUZZ.LEDs.LCD.OLED.HPSW.PB.TEMP.SPWR
// https://sourceforge.net/projects/arduinoascomfocuserpro2diy/files/BOARDS%20PCB/PCB%20ULN2003_M_MT_F_BT/
//
// PCB ULN2003_NOKIA
// BT.BUZZ.LEDs.NOKIA.PB.TMP.SPWR
// https://sourceforge.net/projects/arduinoascomfocuserpro2diy/files/BOARDS%20PCB/PCB%20ULN2003_NOKIA/
//
// PCB ULN2003_M_MT_F_BT_IR_RE_BT
// BT.BUZZ.LEDs.LCD.OLED.HPSW.PB.TMP.SPWR.IR.RE
//
// Stripboard ULN2003_M_MT_F
// BUZZ.LEDs.LCD.OLED.PB.TEMP
// https://sourceforge.net/projects/arduinoascomfocuserpro2diy/files/BOARDS%20STRIPBOARDS/ULN2003_M_MT_F/


// -----------------------------------------------------------------------
// DONATING A CUP OF COFFEE
// -----------------------------------------------------------------------
// If you wish to express your thanks for this project, please donate the amount of
// a Coffee in thanks for this project, please use PayPal.com and send the amount to
// user rbb1brown@gmail.com (Robert Brown). All contributions are gratefully accepted.


// -----------------------------------------------------------------------
// INSTRUCTIONS : SPECIFY ARDUINO IDE SETTINGS
// -----------------------------------------------------------------------
// Set Tools-Board as Arduino Nano
// Set Tools-Processor-ATmega328P-ATMega328P (Old Bootloader)
// Set File-Preferences
//   Show Verbose output during compile (checked), upload (checked)
//   Compiler warnings - Default
// MAKE SURE that the libaries for myFP2 firmware have been imported


// -----------------------------------------------------------------------
// INSTRUCTIONS : SPECIFY FOCUSER CONFIGURATION
// -----------------------------------------------------------------------
// Go to file config.h and enable the hardware options you have


//-----------------------------------------------------------------------
// FIRMWARE CODE START - INCLUDES AND LIBRARIES
//-----------------------------------------------------------------------
#include <Arduino.h>
#include <myQueue.h>
#include <myEEPROM.h>          
#include <myeepromanything.h>  
#include <TimerOne.h>

#include "config.h"

#include "myBoards.h"
DriverBoard* driverboard;

extern void timer_stepmotor(void);
extern volatile bool hpswstate;

#if defined(DISPLAYTYPE)
extern void DisplayUpdatePosition(void);
extern void DisplayUpdatePages(void);
extern void DisplayInit(void);
#include "display.h"
#endif

#if defined(TEMPERATURE_PROBE)
extern void TemperatureInit(void);
extern float read_temp(void);
extern void update_temp();
#include "temperature.h"
#endif

#include "serialcomms.h"

#if defined(ROTARYENCODER)
#include <myQuadrature.h>     // needed for Keyes 040 rotary encoder
#include <Bounce2.h>          // needed to debounce Home Position switch, see https://github.com/thomasfredericks/Bounce2
#endif

#if defined(INFRARED)
#include <myIRremote1.h>
#include "myIRRemotemappings.h"
#endif


//-----------------------------------------------------------------------
// NEW PUSHBUTTON DATA - DO NOT CHANGE
//-----------------------------------------------------------------------
enum btnval { Inbtn,
              Outbtn,
              Bothbtn,
              Nobtn };
btnval btn;

#if defined(PUSHBUTTONS) && defined(PB_SPEED_BOOST)
byte pb_boost = 0;                   // push button speed boost flag (0=off, 1=on)
byte boost_speed = 1; 
#endif


//-----------------------------------------------------------------------
// GLOBAL DATA - DO NOT CHANGE
//-----------------------------------------------------------------------
struct config_t myfocuser;
char firmwareVersion[] = "338";

#if (DRVBRD == ULN2003_M_MT_F_BT)
char firmwareName[] = "ULN2003_M_MT_F_BT";
#elif (DRVBRD == ULN2003_NOKIA)
char firmwareName[] = "ULN2003_NOKIA";
#elif (DRVBRD == ULN2003_M_MT_F_BT_IR_RE_BT)
char firmwareName[] = "ULN2003_M_MT_F_BT_IR_RE_BT";
#elif (DRVBRD == ULN2003_M_MT_F)
char firmwareName[] = "ULN2003_M_MT_F";
#else
char firmwareName[] = "UNKNOWN";
#endif

byte MainStateMachine;

long fcurrentposition;  // current position
long ftargetposition;   // target position
byte isMoving;          // is the motor currently moving
byte writenow;          // save settings in EEPROM
byte movedirection;     // direction of new planned move
byte jogging;           // jogging in Windows application
byte joggingDirection;  // defined jogging direction, 0 = IN, 1 = OUT
byte stepperpower;      // stepperpower state 0 or 1

// TEMPERATURE
byte tprobe1;            // was a temperature probe found?
float temp;              // temperature
byte tempcompstate;      // temperature compensation state, 0=off, 1=on
byte tempcompavailable;  // temperature compensation available
unsigned long lasttempconversion;


// DISPLAY
byte display_pages;               // number of pages for current defined display type
bool display_found;               // true if display is found
unsigned long lastdisplayupdate;  // time that display was last updated

int currentaddr;        // address in EEPROM of the data stored
Queue<String> queue(4); // queue FIFO for commands from Serial port
String line;            // received data from the Serial port

#if defined(BLUETOOTH)
#include <SoftwareSerial.h>  
String btline;
SoftwareSerial btSerial(BTTX, BTRX);
#endif

#if defined(ROTARYENCODER)
Quadrature myrotaryencoder(ROTENCPIN9, ROTENCPIN10);  //  Rotary encoder is connected to pins 9 and 10
long encswpress;           // how long PB of encoder is held down
int encswval;              // used to determine increment value of encoder, +1, +10, +100 per tick
bool RESWVal;              // state of Rotary Encoder Switch
Bounce RESWdb = Bounce();  // Setup debouncer for Rotary Encoder switch
#endif  

#ifdef INFRARED
IRrecv irrecv(RECV_PIN);  // create IR receiver on RECV_PIN [cannot change pin number]
decode_results results;   // decoded result from IR receiver
#endif                    // #ifdef INFRARED


//-----------------------------------------------------------------------
// STEPPER MOTOR ROUTINES
//-----------------------------------------------------------------------
void MotorStepClockwise() {
  (!myfocuser.reversedirection) ? driverboard->movemotor(1) : driverboard->movemotor(0);
}

void MotorStepAntiClockwise() {
  (!myfocuser.reversedirection) ? driverboard->movemotor(0) : driverboard->movemotor(1);
}

// -----------------------------------------------------------------------
// EEPROM stuff
// -----------------------------------------------------------------------
void writeEEPROMNow() {
  EEPROM_writeAnything(currentaddr, myfocuser);
  writenow = 0;
}

// -----------------------------------------------------------------------
// RESET DISPLAY OPTIONS
// -----------------------------------------------------------------------
void resetdisplay_option() {
  // display_option: determines which pages are enabled to display
  // display_pages: the total number of display pages for that display type
#if (DISPLAYTYPE == DISPLAY_LCD1602)
  myfocuser.display_option = 63;  // 111111
  display_pages = 6;              // pg1-pg6
#elif (DISPLAYTYPE == DISPLAY_LCD1604)
  myfocuser.display_option = 7;  // 111
  display_pages = 3;             // pg1-pg3
#elif (DISPLAYTYPE == DISPLAY_LCD2004)
  myfocuser.display_option = 7;  // 111
  display_pages = 3;             // pg1-pg3
#elif (DISPLAYTYPE == DISPLAY_OLED12864)
  myfocuser.display_option = 3;  // 11
  display_pages = 2;             // pg1-pg2
#elif (DISPLAYTYPE == DISPLAY_TFT)
  myfocuser.display_option = 3;  // 11
  display_pages = 2;             // pg1-pg2
#elif (DISPLAYTYPE == DISPLAY_UTFT)
  myfocuser.display_option = 3;  // 11
  display_pages = 2;             // pg1-pg2
#elif (DISPLAYTYPE == DISPLAY_NOKIA)
  myfocuser.display_option = 7;  // 111
  display_pages = 3;             // pg1-pg3
#else
  // no display
  myfocuser.display_option = 1;
  display_pages = 0;
#endif
}

// -----------------------------------------------------------------------
// SET FOCUSER SETTINGS TO DEFAULT
// -----------------------------------------------------------------------
void setfocuserdefaults() {
  myfocuser.validdata = VALIDDATAFLAG;
  myfocuser.fposition = DEFAULTPOSITION;
  myfocuser.maxstep = DEFAULTMAXSTEP;
  myfocuser.stepsize = DEFAULTSTEPSIZE;  // default is celsius
  myfocuser.tempcoefficient = 20;
  myfocuser.stepmode = DEFAULTSTEPMODE;
  myfocuser.reversedirection = 0;
  myfocuser.coilpower = 0;
  myfocuser.tempmode = CELCIUS;
  myfocuser.display_pagetime = DEFAULT_PAGETIME;
  myfocuser.stepsizeenabled = 1;
  myfocuser.display_updateonmove = 0;
  myfocuser.tempresolution = TEMPRESOLUTION;
  myfocuser.delayaftermove = 0;
  myfocuser.backlashsteps_in = 0;
  myfocuser.backlashsteps_out = 0;
  myfocuser.focuserdirection = MOVINGIN;
  myfocuser.backlash_in_enabled = 0;
  myfocuser.backlash_out_enabled = 0;  
  myfocuser.tcdirection = MOVINGIN;
  myfocuser.motorspeed = FAST;
  myfocuser.display_enabled = 1;
  myfocuser.pbsteps = PB_STEPS;
  resetdisplay_option();
  myfocuser.stepdelay = MSDELAY;
  writeEEPROMNow();
  ftargetposition = fcurrentposition = myfocuser.fposition;
}


// -----------------------------------------------------------------------
// REBOOT THE ARDUINO
// -----------------------------------------------------------------------
void software_Reboot() {
  asm volatile("jmp 0");  // jump to the start of the program
}


// -----------------------------------------------------------------------
// READ PUSH BUTTONS
// -----------------------------------------------------------------------
btnval readpushbuttons(void) {
#if defined(PUSHBUTTONS)
  static int sampletable[5]; // 0, 1, 2, 3, 4
  static int pbtotal;
  static int pbval;
  digitalWrite(PBSWITCHESPIN, HIGH);
  delay(5);
  pbtotal = 0;
  for (int sample=0; sample < 5; sample++)
  {
    delay(5);
    pbval = analogRead(A0);
    sampletable[sample] = pbval;   
    pbtotal += pbval;
  }

  //0000 - 1719 Ignore
  //1720 - 1920	OUT
  //1921 - 2199 Ignore
  //2200 - 2700	BOTH
  //2701 - 3299	Ignore
  //3300 - 3800	IN
  //3801+ ---	Ignore

  if (pbtotal > 1720 && pbtotal < 1920) {
    // sw1 ON and SW2 OFF
    //Serial.println("updatepushbuttons IN:");
    return Inbtn;
  } else if (pbtotal > 2200 && pbtotal < 2700) {
    // sw1 ON and sw2 ON
    //Serial.println("updatepushbuttons ZERO:");
    return Bothbtn;
  } else if (pbtotal > 3300 && pbtotal < 3800) {
    // sw2 ON and SW1 OFF
    // Serial.println("updatepushbuttons OUT:");
    return Outbtn;
  }
#endif
  // for any other value do nothing
  return Nobtn;
}


// -----------------------------------------------------------------------
// UPDATE PUSH BUTTONS
// -----------------------------------------------------------------------
void updatepushbuttons(void) {
#if defined(PUSHBUTTONS)
#if defined(PB_SPEED_BOOST)
  static btnval heldBtn = Nobtn;       // track which button is held
  static unsigned long holdStart = 0;   // when button hold started
#endif
  
  btn = readpushbuttons();
  delay(5);
  if (readpushbuttons() == btn) {
#if defined(PB_SPEED_BOOST)
    // Check for long-press on IN or OUT buttons
    if (btn == Inbtn || btn == Outbtn) {
      if (heldBtn == btn) {
        // Same button still held, check if long-press threshold reached
        if (pb_boost == 0 && (millis() - holdStart) >= PB_LONGPRESS_MS) {
          pb_boost = 1;  // Enable speed boost
          boost_speed = PB_SPEED_BOOST_MULTIPLIER;  // Enable speed boost
           myfocuser.motorspeed = FAST;
         // if (isMoving && driverboard->get_motortimerstate()) { driverboard->stop_motortimer(); driverboard->start_motortimer(); };
        }
      } else {
        // New button pressed, start tracking
        heldBtn = btn;
        holdStart = millis();
      }
    } else if (btn == Bothbtn) {
      // Both buttons pressed - clear boost and handle as before
      pb_boost = 0;
      boost_speed = 1;
      myfocuser.motorspeed = SLOW;
      heldBtn = Nobtn;
       // if (isMoving && driverboard->get_motortimerstate()) { driverboard->stop_motortimer(); driverboard->start_motortimer(); };
#if defined(BUZZER)
      digitalWrite(BUZZERPIN, 1);  // turn on buzzer
#endif
      while (readpushbuttons() == Bothbtn)  // wait for pb to be released
        ;
      fcurrentposition = 0;
      ftargetposition = 0;
#if defined(DISPLAYTYPE)
      if ( myfocuser.display_enabled == 1) {
        DisplayUpdatePosition();
      }
#endif
      return;
    } else {
      // Nobtn - button released, clear boost
      pb_boost = 0;
      boost_speed = 1;
      myfocuser.motorspeed = SLOW;
      heldBtn = Nobtn;
    }
#else
    // No speed boost - handle Bothbtn case
    if (btn == Bothbtn) {
#if defined(BUZZER)
      digitalWrite(BUZZERPIN, 1);  // turn on buzzer
#endif
      while (readpushbuttons() == Bothbtn)  // wait for pb to be released
        ;
      fcurrentposition = 0;
      ftargetposition = 0;
#if defined(DISPLAYTYPE)
      if ( myfocuser.display_enabled == 1) {
        DisplayUpdatePosition();
      }
#endif
      return;
    }
#endif
    
    switch (btn) {
      case Inbtn:
      
        ftargetposition = ftargetposition - (myfocuser.pbsteps * boost_speed);
        ftargetposition = (ftargetposition < 0) ? 0 : ftargetposition;
        break;
      case Outbtn:
        ftargetposition = ftargetposition + (myfocuser.pbsteps * boost_speed);
        ftargetposition = (ftargetposition > myfocuser.maxstep) ? myfocuser.maxstep : ftargetposition;
        break;
      case Nobtn:
        //
        break;
      case Bothbtn:
        // Already handled above
        break;
    }
#if defined(DISPLAYTYPE)
    if ( myfocuser.display_enabled == 1) {
      DisplayUpdatePosition();
    }
#endif      
  }
#endif
}

// -----------------------------------------------------------------------
// UPDATE ROTARY ENCODER
// -----------------------------------------------------------------------
void updaterotaryencoder(void) {
  // ignore any move requests push buttons or jogging if temperature compensation is enabled
  // check for temperature compensation first!
  // do not process another move if already moving
#if defined(ROTARYENCODER)
  if (isMoving == 0) {
    if (tempcompstate == 0) {
      int lp = myrotaryencoder.getposition();
      if (lp != 0) {
        myfocuser.focuserdirection = movedirection;
        // adjust the target position
        long newPos = fcurrentposition + (myrotaryencoder.getposition() * encswval);
        newPos = (newPos < 0) ? 0 : newPos;
        ftargetposition = (newPos > myfocuser.maxstep) ? myfocuser.maxstep : newPos;
        myrotaryencoder.setposition(0);  // avoid counting more than once
#if defined(DISPLAYTYPE)
        if (display_found) {
          DisplayUpdatePosition();
        }
#endif         
        writenow = 1;
      }
      // check rotary encoder push switch
      // if pushed (returns LOW) then halt focuser or perform, additional action
      RESWdb.update();
      if (RESWdb.fell())  // if pushed then halt focuser or perform, additional action
      {
        long encswstart = millis();  // start time of encoder sw press
        int PressTime = 1;
        beep(PressTime);
        while (!digitalRead(ENCODERSWPIN))  // wait for PB to be released
        {
          if ((int)(millis() - encswstart) > (PressTime * 1000)) {
            PressTime++;
            beep(PressTime);
          }
          if (PressTime == 4) {
            break;
          }
        }
        switch (PressTime) {
          case 1:
            myfocuser.motorspeed = SLOW;
            encswval = 1;
            break;
          case 2:
            myfocuser.motorspeed = MED;
            encswval = 10;
            break;
          case 3:
            myfocuser.motorspeed = FAST;
            myfocuser.pbsteps = 1;
            encswval = 100;
            break;
          case 4:
            encswval = 1;
            fcurrentposition = ftargetposition = 0;
            break;
          default:
            break;
        }
        writenow = 1;
      }
    }  // if (tempcompstate == 1)
  }    // if (isMoving == 1)
#endif
}

// provide auditory feedback on encoder push switch
// note: beep will only be heard if there is a buzzer connected and the #define for the BUZZER is uncommented at the top of this file
void beep(int beeps) {
  for (int lp1 = 0; lp1 < beeps; lp1++) {
#if defined(BUZZER)
    digitalWrite(BUZZERPIN, 1);  // turn on buzzer
    delay(100);                  // beep for 100ms
    digitalWrite(BUZZERPIN, 0);  // turn off buzzer
    delay(50);                   // delay between beeps for 50ms
#endif                           // #if defined(BUZZER)
  }
}

// -----------------------------------------------------------------------
// UPDATE INFRA RED REMOTE
// -----------------------------------------------------------------------
void updateirremote() {
#ifdef INFRARED
  // check for temperature compensation first!
  // do not process another move if already moving
  if (tempcompstate == 0) {
    // check IR
    if (irrecv.decode(&results)) {
      int adjpos = 0;
      static long lastcode;
      if (results.value == KEY_REPEAT) {
        results.value = lastcode;  // repeat last code
      } else {
        lastcode = results.value;
      }
      if ((isMoving == 1) && (lastcode == IR_HALT)) {
        // stop a move - like a Halt
        ftargetposition = fcurrentposition;
        isMoving = 0;
        writenow = 1;
      } else {
        switch (lastcode) {
          case IR_SLOW:
            myfocuser.motorspeed = SLOW;
            break;
          case IR_MEDIUM:
            myfocuser.motorspeed = MED;
            break;
          case IR_FAST:
            myfocuser.motorspeed = FAST;
            myfocuser.pbsteps = 1;
            break;
          case IR_IN1:
            adjpos = -1;
            break;
          case IR_OUT1:
            adjpos = 1;
            break;
          case IR_IN10:
            adjpos = -10;
            break;
          case IR_OUT10:
            adjpos = 10;
            break;
          case IR_IN50:
            adjpos = -50;
            break;
          case IR_OUT50:
            adjpos = 50;
            break;
          case IR_IN100:
            adjpos = -100;
            break;
          case IR_OUT100:
            adjpos = 100;
            break;
          case IR_SETPOSZERO:  // 0 RESET POSITION TO 0
            fcurrentposition = ftargetposition = adjpos = 0;
            break;
        }
        irrecv.resume();  // Receive the next value
        long newpos;
        if (adjpos < 0) {
          newpos = fcurrentposition + adjpos;
          newpos = (newpos < 0) ? 0 : newpos;
          ftargetposition = newpos;
        } else if (adjpos > 0) {
          newpos = fcurrentposition + adjpos;
          newpos = (newpos > myfocuser.maxstep) ? myfocuser.maxstep : newpos;
          ftargetposition = newpos;
        }
#if defined(DISPLAYTYPE)
    if (display_found) {
      DisplayUpdatePosition();
    }
#endif         
        writenow = 1;
      }  // switch(last_code)
    }    // if ((isMoving == 1) && (lastcode == IR_HALT))
  }      // if (tempcompstate == 1)
#endif
}

// -----------------------------------------------------------------------
// UPDATE JOGGING
// -----------------------------------------------------------------------
void updatejogging(void) {
  if (joggingDirection == 0) {
    ftargetposition--;
    ftargetposition = (ftargetposition < 0) ? 0 : ftargetposition;
    writenow = 1;
  } else {
    ftargetposition++;
    ftargetposition = (ftargetposition > myfocuser.maxstep) ? myfocuser.maxstep : ftargetposition;
    writenow = 1;
  }
#if defined(DISPLAYTYPE)
    if (display_found) {
      DisplayUpdatePosition();
    }
#endif 
#if defined(SUPERSLOWJOGGING)
  delay(SLOWSPEEDJOGDELAY);
#endif
}

// -----------------------------------------------------------------------
// UPDATE STEPPER POWER
// -----------------------------------------------------------------------
void updatestepperpowerdetect() {
#if defined(STEPPERPWRDETECT)
  stepperpower = (analogRead(STEPPERDETECTPIN)) > 600 ? 1 : 0;
  // for ULN2003 powered from  9V with 4.7VZ, reading was 3.72V = 763
  // for drv8825 powered from 12V with 4.7VZ, reading was 4.07V = 834
  // Each digit = .00488millivolts
#else
  stepperpower = 1;
#endif
}

// -----------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------
void setup() {
  int datasize;    // will hold size of the struct myfocuser - 6 bytes
  int nlocations;  // number of storage locations available in EEPROM
  byte found;

  Serial.begin(SERIALPORTSPEED);  // initialize serial port
  clearSerialPort();              // clear any garbage from serial buffer

#if defined(BLUETOOTH)
  btline = "";
  btSerial.begin(BTPORTSPEED);  // start bt adapter
  clearbtPort();
#endif

#if defined(BUZZER)
  pinMode(BUZZERPIN, OUTPUT);  // turn ON the Buzzer - provide power ON beep
  digitalWrite(BUZZERPIN, 1);
#endif

#if defined(INOUTLEDS)
  DebugPrintln("LED001");
  pinMode(INLED, OUTPUT);  // turn ON both LEDS as power on cycle indicator
  pinMode(OUTLED, OUTPUT);
  digitalWrite(INLED, 1);
  digitalWrite(OUTLED, 1);
#endif

  stepperpower = 1;
#if defined(STEPPERPWRDETECT)
  pinMode(STEPPERDETECTPIN, INPUT);
  DebugPrintln("STP001");
  updatestepperpowerdetect();
#endif

  isMoving = writenow = jogging = joggingDirection = tprobe1 = 0;

  currentaddr = 0;  // start at 0 if not found later
  found = 0;
  datasize = sizeof(myfocuser);
  nlocations = EEPROMSIZE / datasize;
  for (int lp1 = 0; lp1 < nlocations; lp1++) {
    int addr = lp1 * datasize;
    EEPROM_readAnything(addr, myfocuser);
    if (myfocuser.validdata == VALIDDATAFLAG)  // check to see if the data is valid
    {
      currentaddr = addr;  // data was erased so write some default values
      found = 1;
      break;
    }
  }
  if (found == 1) {
    // Set the focuser back to the previous settings
    // done after this in one hit
    // mark current eeprom address as invalid and use next one
    // each time focuser starts it will read current storage, set it to invalid, goto next location and
    // write values to there and set it to valid - so it doesnt always try to use same locations over and
    // over and destroy the eeprom
    // using it like an array of [0-nlocations], ie 100 storage locations for 1k EEPROM
    EEPROM_readAnything(currentaddr, myfocuser);
    myfocuser.validdata = 0;
    EEPROM_writeAnything(currentaddr, myfocuser);
    currentaddr += datasize;  // goto next free address and write data
    // bound check the eeprom storage and if greater than last index [0-EEPROMSIZE-1] then set to 0
    if (currentaddr >= (nlocations * datasize)) {
      currentaddr = 0;
    }
    myfocuser.validdata = VALIDDATAFLAG;
    writeEEPROMNow();  // update values in EEPROM
  } else {
    // Set defaults because not found
    DebugPrintln("CFG002");
    setfocuserdefaults();
  }

  // temperature probe
  DebugPrintln("TMP001");
  temp = 20.0;            // dummy temp value
  tprobe1 = 0;            // disable tprobe state
  tempcompstate = 0;      // disable temperature compensation on
  tempcompavailable = 0;  // disable temperature compensation off
  lasttempconversion = millis();
#if defined(TEMPERATURE_PROBE)
  // Init will set tprobe1, tempcompstate and tempcompavailable
  TemperatureInit();  // start temperature probe
  if (tprobe1 == 1) {
    read_temp();  // read temperature
  }
#endif

  // check display option and reset if necessary
  if (myfocuser.display_option == 0) {
    resetdisplay_option();
  }

  display_found = false;
#if defined(DISPLAYTYPE)
  // If display not found, then DisplayInit()
  // will set display_found to 0
  DisplayInit();
  if (display_found) {
    DebugPrintln("display found");
  } else {
    DebugPrintln("display NOT found");
  }
  lastdisplayupdate = millis();
#endif

  movedirection = myfocuser.focuserdirection;

  DebugPrintln("BRD001");
  driverboard = new DriverBoard(DRVBRD, IN1ULN, IN2ULN, IN3ULN, IN4ULN);
  DebugPrintln("BRD002");

  if (myfocuser.coilpower == 0) {
    driverboard->set_motorpower(false);
  }

  fcurrentposition = ftargetposition = myfocuser.fposition;
  writenow = 1;  

  // Set up timer
  driverboard->init_motortimer();

#if defined(ROTARYENCODER)
  myrotaryencoder.minimum(-1000);  // Setup defaults for rotary encoder
  myrotaryencoder.maximum(1000);
  encswval = 1;
  pinMode(ENCODERSWPIN, INPUT_PULLUP);
  RESWdb.attach(ENCODERSWPIN);  // Setup defaults for debouncing Rotary Encoder Switch
  RESWdb.interval(5);
#endif

#if defined(INFRARED)
  irrecv.enableIRIn();  // Start the IR
#endif  

#if defined(INOUTLEDS)
  // turn off the IN/OUT LEDS and BUZZER
  digitalWrite(INLED, 0);
  digitalWrite(OUTLED, 0);
#endif

#if defined(BUZZER)
  digitalWrite(BUZZERPIN, 0);
#endif

  MainStateMachine = State_Idle;

  DebugPrintln("Setup end");
}


void loop() {
  static unsigned long timestampdelayaftermove = millis();
  static unsigned long previousMillis = millis();
  static unsigned long DisplayUpdate = millis();
  static unsigned long TempUpdate = millis();
  static byte backlash_count = 0;
  static byte backlash_enabled = 0;
  static byte updatecount = 0;

#if defined(HOMEPOSITIONSWITCH)
  static int stepstaken = 0;  // used with physical home position switch
#endif

#if defined(BLUETOOTH)
  btSerialEvent();  // check for command from bt adapter
#endif

  DebugPrintln("STP001");
  updatestepperpowerdetect();

  // check for serial command
  if (queue.count() >= 1) {
    ser_comms();
  }

  switch (MainStateMachine) {
    case State_Idle:
      if (fcurrentposition != ftargetposition) {
        // due to timing issue with TFT must be set here
        isMoving = 1;
        driverboard->set_motorpower(true);
        driverboard->set_motortimerstate(false);
        MainStateMachine = State_InitMove;
      } else {
        isMoving = 0;
        // update temperature
#if defined(TEMPERATURE_PROBE)
        if (tprobe1 == 1) {
          TempUpdate = millis();
          if (((TempUpdate - lasttempconversion) > TEMP_REFRESHRATE) || (TempUpdate < lasttempconversion)) {
            update_temp();
            lasttempconversion = TempUpdate;  // update time stamp
          }
        }
#endif
#if defined(DISPLAYTYPE)
        if (display_found) {
          unsigned long DisplayUpdate = millis();
          if ((DisplayUpdate - lastdisplayupdate) > ((unsigned long)(myfocuser.display_pagetime * 1000)) || (DisplayUpdate < lastdisplayupdate)) {
            // update timestamp
            lastdisplayupdate = DisplayUpdate;
            DebugPrintln("DIS002");
            DisplayUpdatePages();
          }
        }  // if (display_found)
#endif
        if (stepperpower == 1) {
          // update push buttons
          updatepushbuttons();
        
          // update rotary encoder
          updaterotaryencoder();

          // update infra red remote
          updateirremote();
                  
          // update jogging
          if (jogging == 1) {
            updatejogging();
          }
        }

        if (writenow == 1) {
          // decide if we have waited 10s after the last move, if so, update the EEPROM
          static unsigned long currentMillis;
          currentMillis = millis();
          if (((currentMillis - previousMillis) > EEPROMWRITEINTERVAL) || (currentMillis < previousMillis)) {
            previousMillis = currentMillis;
            // copy current settings and write the data to EEPROM
            myfocuser.validdata = 99;
            myfocuser.fposition = fcurrentposition;
            // update values in EEPROM
            EEPROM_writeAnything(currentaddr, myfocuser);
            writenow = 0;
          }
        }
      }  // if (fcurrentposition != ftargetposition)
      break;

    case State_InitMove:
      isMoving = 1;
      if (ftargetposition < fcurrentposition) {
        movedirection = MOVINGIN;
        backlash_count = myfocuser.backlashsteps_in;
        backlash_enabled = myfocuser.backlash_in_enabled;
      } else {
        movedirection = MOVINGOUT;
        backlash_count = myfocuser.backlashsteps_out;
        backlash_enabled = myfocuser.backlash_out_enabled;
      }
      // enable leds
#if defined(INOUTLEDS)
      if (movedirection == MOVINGIN) {
        (!myfocuser.reversedirection) ? digitalWrite(OUTLED, 1) : digitalWrite(INLED, 1);
      } else  // moving out
      {
        (!myfocuser.reversedirection) ? digitalWrite(INLED, 1) : digitalWrite(OUTLED, 1);
      }
#endif
      if (movedirection != myfocuser.focuserdirection) {
        if (backlash_enabled == 1) {
          //apply backlash
          myfocuser.focuserdirection = movedirection;
          MainStateMachine = State_ApplyBacklash;
          DebugPrintln("MAS001");
        } else {
          // do not apply backlash, go straight to moving
          MainStateMachine = State_Moving;
          DebugPrintln("MAS002");
        }
      } else {
        MainStateMachine = State_Moving;
        DebugPrintln("MAS002");
      }
      break;

    case State_ApplyBacklash:
      if (backlash_count > 0) {
        (movedirection == MOVINGIN) ? MotorStepAntiClockwise() : MotorStepClockwise();
        delayMicroseconds(driverboard->get_stepdelay());
        backlash_count--;
      } else {
        MainStateMachine = State_Moving;
      }  // if (backlash_count)
      break;

    case State_Moving:
      if (driverboard->get_motortimerstate() == false) {
        // enable motor timer, start moving
        driverboard->set_motorpower(true);
        driverboard->start_motortimer();
      }
#if defined(DISPLAYTYPE)
      if (display_found)
      {
        if (myfocuser.display_updateonmove == 1) {
          updatecount++;
          if (updatecount > DISPLAY_STEPCOUNT) {
            updatecount = 0;
            DisplayUpdatePosition();
          }
        }  // if (myfocuser.display_updateonmove == 1)
      }
  #endif
      break;

    case State_FindHomePosition:
      // move in till home position switch closes
#if defined(HOMEPOSITIONSWITCH)
      stepstaken = 0;
      HPSWDebugPrintln("State_FindHomePosition: MoveIN till closed");
      static bool swstate = driverboard->get_hpsw();
      while (swstate == false) {
        // step IN till switch closes
        MotorStepAntiClockwise();
        delayMicroseconds(myfocuser.stepdelay);
        stepstaken++;
        HPSWDebugPrintln(".");
        // this prevents the endless loop if the hpsw is not connected or is faulty
        if (stepstaken > HOMESTEPS) {
          HPSWDebugPrintln("HPSW MoveIN ERROR: HOMESTEPS exceeded");
          break;
        }
        swstate = driverboard->get_hpsw();
      }
      HPSWDebugPrintln();
      HPSWDebugPrint("HPSW state=");
      HPSWDebugPrint(driverboard->get_hpsw());
      HPSWDebugPrint("HP MoveIN stepstaken=");
      HPSWDebugPrintln(stepstaken);
      HPSWDebugPrintln("HP MoveIN finished");
#endif  // HOMEPOSITIONSWITCH
      MainStateMachine = State_SetHomePosition;
      HPSWDebugPrintln("State -> SetHomePosition");
      break;

    case State_SetHomePosition:
      // move out till home position switch opens
#if defined(HOMEPOSITIONSWITCH)
      stepstaken = 0;
      HPSWDebugPrintln("State_SetHomePosition Move out till HPSW is OPEN");
      // if the previous moveIN failed at HOMESTEPS and HPSWITCH is still open then the
      // following while() code will drop through and have no effect and position = 0
      myfocuser.focuserdirection = !movedirection;
      while (driverboard->get_hpsw() == true) {
        // step out till switch opens
        MotorStepClockwise();
        delayMicroseconds(myfocuser.stepdelay);
        stepstaken++;
        HPSWDebugPrintln(".");
        if (stepstaken > HOMESTEPS)  // this prevents the endless loop if the hpsw is not connected or is faulty
        {
          HPSWDebugPrintln("HP MoveOUT ERROR: HOMESTEPS exceeded");
          break;
        }
      }
      HPSWDebugPrintln();
      HPSWDebugPrint("HP MoveOUT stepstaken=");
      HPSWDebugPrintln(stepstaken);
      HPSWDebugPrintln("HP MoveOUT finished");
      HPSWDebugPrintln("State -> State_MoveEnded");
#endif  // HOMEPOSITIONSWITCH
      timestampdelayaftermove = millis();
      MainStateMachine = State_MoveEnded;
      DebugPrintln("MAS012");
      break;

    case State_MoveEnded:
      timestampdelayaftermove = millis();
      MainStateMachine = State_DelayAfterMove;
      break;

    case State_DelayAfterMove:
      {
        static unsigned long timenow;
        timenow = millis();
        if (((timenow - timestampdelayaftermove) > myfocuser.delayaftermove) || (timenow < timestampdelayaftermove)) {
          timestampdelayaftermove = timenow;
          MainStateMachine = State_FinishedMove;
          DebugPrintln("MAS013");
        }
      }
      break;

    case State_FinishedMove:
      // turn off leds
#if defined(INOUTLEDS)
      digitalWrite(INLED, 0);
      digitalWrite(OUTLED, 0);
#endif
      if (myfocuser.coilpower == 0) {
        DebugPrintln("PAR000");
        driverboard->set_motorpower(false);
      } 
      isMoving = 0;
      writenow = 1;
      previousMillis = millis();
      MainStateMachine = State_Idle;
      DebugPrintln("MAS014");
      break;

    default:
      MainStateMachine = State_Idle;
      break;
  }
}  // end Loop()
