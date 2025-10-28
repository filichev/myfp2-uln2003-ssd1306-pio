//-----------------------------------------------------------------------
// myFocuserPro2 Driver Board Definitions
// (c) R Brown, 2014-2024, All rights reserved.
//-----------------------------------------------------------------------
#ifndef myBoards_h
#define myBoards_h

#include <Arduino.h>

#if !defined(SLOW)
#define SLOW 0  // motorspeeds
#endif
#if !defined(MED)
#define MED 1
#endif
#if !defined(FAST)
#define FAST 2
#endif

#if !defined(STEP1)
#define STEP1 1  // stepmodes
#endif
#if !defined(STEP2)
#define STEP2 2
#endif
#if !defined(STEP4)
#define STEP4 4  // for uln2003, stepmodes higher than 2 are not available
#endif
#if !defined(STEP8)
#define STEP8 8
#endif
#if !defined(STEP16)
#define STEP16 16
#endif
#if !defined(STEP32)
#define STEP32 32
#endif
#if !defined(STEP64)
#define STEP64 64
#endif
#if !defined(STEP128)
#define STEP128 128
#endif
#if !defined(STEP256)
#define STEP256 256
#endif
#if !defined(MAXSTEPMODE)
#define MAXSTEPMODE 2
#endif

#define IN1ULN 4
#define IN2ULN 5
#define IN3ULN 6
#define IN4ULN 7

#define MSDELAY 5000


//-----------------------------------------------------------------------
// DRIVERBOARD CLASS DEFINITION
//-----------------------------------------------------------------------
class DriverBoard {
public:
  DriverBoard(byte, byte, byte, byte, byte);
  void init_hpsw(void);
  bool get_hpsw(void);
  
  void set_motorpower(byte);

  void init_motortimer(void);
  void start_motortimer(void);
  void stop_motortimer(void);
  bool get_motortimerstate(void);
  void set_motortimerstate(bool);
  // move motor one step, takes parameter direction 0 | 1
  void movemotor(byte);  

  // this controls the speed of the motor, using delayMicroseconds(int) after each step
  //void set_stepdelay(unsigned long);
  int get_motortimerinterval(void);
  int get_stepdelay(void);

  int get_stepmode(void);
  void set_stepmode(void);

private:
  int inputPins[4];  // The input pin numbers for ULN2003 boards
  byte boardtype;    // ULN2003
  int Step;          // used to control step count
  bool _motortimerstate;

  int ulnfull[4] = { B01100, B00110, B00011, B01001 };
  int ulnhalf[8] = { B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001 };

  //int uln8sfull[4] = { B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001  };
  int uln8shalf[8] = { B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001 };
};

// The NOKIA PCB requires a Nokia 5110 LCD module.
// It is assumed that the LCD module is connected to
// the following pins using a levelshifter to get the
// correct voltage to the module.
//      SCK  - Pin 8
//      MOSI - Pin 9
//      DC   - Pin 10
//      RST  - Pin 11
//      CS   - Pin 12

#endif
