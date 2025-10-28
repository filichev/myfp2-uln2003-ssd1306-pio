//---------------------------------------------------------------------
// myFocuserPro2 Driver Board Code
// (c) R Brown, 2014-2024, All rights reserved.
// (c) Joel Collet, move-timer, 2021-2022, All rights reserved
//---------------------------------------------------------------------

#include <Arduino.h>
#include "config.h"
#include "myBoardDefs.h"
#include "myBoards.h"

#if defined(HOMEPOSITIONSWITCH)
#include <Bounce2.h>           // needed to debounce Home Position switch (HPSW)
Bounce hpswbounce = Bounce();  // Setup debouncer for HPSW
#endif
// v1 of the HPSW which has switch normally open to D12 and ground
// when HPSW is NOT activated (contacts open), D12 = 5V = HIGH
// when HPSW is activated (contacts closed), D12 = GND = LOW
// Normal operation, HPSW not activated, contacts open, D12 high, motor can move
// Fault operation, D12 (disconnected switch or power) floats high, motor can move
#include <TimerOne.h>


//-----------------------------------------------------------------------
// DEFINES
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// EXTERNS
//-----------------------------------------------------------------------
extern void MotorStepClockwise(void);
extern void MotorStepAntiClockwise(void);
extern struct config_t myfocuser;
extern long fcurrentposition;  // current focuser position
extern long ftargetposition;   // target position
extern byte isMoving;          // is the motor currently moving
extern byte MainStateMachine;
extern byte movedirection;  // holds direction of new planned move
extern DriverBoard* driverboard;

#if defined(PUSHBUTTONS) && defined(PB_SPEED_BOOST)
extern byte pb_boost;  // push button speed boost flag
#endif


//-----------------------------------------------------------------------
// LOCAL DATA
//-----------------------------------------------------------------------
volatile bool hpswstate;


//-----------------------------------------------------------------------
// void timer_stepmotor(void)
// Timer1, steps motor till fcurrentposition == ftargetposition
//-----------------------------------------------------------------------
void timer_stepmotor() {
  // must come first else cannot halt
  if (fcurrentposition == ftargetposition) {
    driverboard->stop_motortimer();
    DebugPrint("MAS013");
    MainStateMachine = State_MoveEnded;
    DebugPrint("MAS010");
  } else {
    // focuser not finished the move yet
    (movedirection == MOVINGIN) ? MotorStepAntiClockwise() : MotorStepClockwise();
    (movedirection == MOVINGIN) ? fcurrentposition-- : fcurrentposition++;
    // no need for validation as target has already been validated before move

#if defined(HOMEPOSITIONSWITCH)
    if (movedirection == MOVINGIN) {
      // if moving in. check if hpsw closed or position = 0
      hpswstate = driverboard->get_hpsw();
      if ((hpswstate == true) && (fcurrentposition >= 0)) {
        // if switch state = CLOSED and position >= 0
        // need to back OUT a little till switch opens and then set position to 0
        driverboard->stop_motortimer();
        //isMoving = 1;
        fcurrentposition = ftargetposition = 0;
        MainStateMachine = State_SetHomePosition;
        DebugPrintln("MAS014");
        HPSWDebugPrintln("hpswstate=closed, pos >= 0");
      } else if ((hpswstate == false) && (fcurrentposition == 0)) {
        // if switchstate = OPEN and Position = 0
        // need to move IN a little till switch CLOSES then
        driverboard->stop_motortimer();
        //isMoving = 1;
        fcurrentposition = ftargetposition = 0;
        MainStateMachine = State_FindHomePosition;
        DebugPrintln("MAS015");
        HPSWDebugPrintln("HPSW=Open, position=0");
        HPSWDebugPrintln("State -> State_FindHomePosition");
      }
    }
#endif
  }
}


//-----------------------------------------------------------------------
// DRIVER BOARD CLASS
// ULN2003
//-----------------------------------------------------------------------
DriverBoard::DriverBoard(byte brdtype, byte pin1, byte pin2, byte pin3, byte pin4)
  : boardtype(brdtype) {
  this->inputPins[0] = pin1;
  this->inputPins[1] = pin2;
  this->inputPins[2] = pin3;
  this->inputPins[3] = pin4;
  for (int inputCount = 0; inputCount < 4; inputCount++) {
    pinMode(this->inputPins[inputCount], OUTPUT);
  }
  Step = 0;

#if defined(HOMEPOSITIONSWITCH)
  hpswstate = false;
  HPSWDebugPrintln("init HPSW");
  init_hpsw();
  HPSWDebugPrintln("Read HPSW");
  hpswbounce.update();
  hpswstate = get_hpsw();
#endif
  
  // Set stepmode
  set_stepmode();
  _motortimerstate = false;
}


//-----------------------------------------------------------------------
// INITIALISE HPSW
//-----------------------------------------------------------------------
void DriverBoard::init_hpsw(void) {
#if defined(HOMEPOSITIONSWITCH)
  // Init Home Position Switch pin as an input
  pinMode(HPSWPIN, INPUT_PULLUP);  
  // Setup defaults for debouncing hp Switch
  hpswbounce.attach(HPSWPIN);      
  // Set debounce time
  hpswbounce.interval(5);          
  hpswbounce.update();
#endif
}


//-----------------------------------------------------------------------
// GET HOME POSITION SWITCH STATE
// hpsw pin has 470K pullup, hpsw open = high, hpsw closed = low
// return state of hpswstate needs to return high = sw closed, low = sw open
//-----------------------------------------------------------------------
bool DriverBoard::get_hpsw(void) {
#if defined(HOMEPOSITIONSWITCH)
  hpswbounce.update();
  return !(hpswbounce.read());
#else
  // no switch
  return false;
#endif
}


//-----------------------------------------------------------------------
// SET MOTOR POWER
// turn coil power on, turn coil power off
//-----------------------------------------------------------------------
void DriverBoard::set_motorpower(byte state) {
  if (state == true) {
    // do nothing
    delay(1);  // need to wait 1ms before driver chip is ready for stepping
  } else {
    // power off
    for (int i = 0; i < 4; i++) {
      digitalWrite(inputPins[i], LOW);
    }
  }
}


//-----------------------------------------------------------------------
// INITIALISE MOTOR TIMER
//-----------------------------------------------------------------------
void DriverBoard::init_motortimer(void) {
  Timer1.initialize();
  Timer1.attachInterrupt(timer_stepmotor);
  set_motortimerstate(false);
}


//-----------------------------------------------------------------------
// ENABLE MOTOR TIMER, STEPPER STARTS MOVING
//-----------------------------------------------------------------------
void DriverBoard::start_motortimer(void) {
  set_motortimerstate(true);
}


//-----------------------------------------------------------------------
// STOP MOTOR TIMER, STEPPER STOPS MOVING
//-----------------------------------------------------------------------
void DriverBoard::stop_motortimer(void) {
  set_motortimerstate(false);
}


//-----------------------------------------------------------------------
// GET MOTOR TIMER STATE
//-----------------------------------------------------------------------
bool DriverBoard::get_motortimerstate(void) {
  return _motortimerstate;
}


//-----------------------------------------------------------------------
// SET STATE OF MOTOR TIMER
//-----------------------------------------------------------------------
void DriverBoard::set_motortimerstate(bool state) {
  if (state == false) {
    Timer1.setPeriod(get_motortimerinterval());
    Timer1.stop();
    _motortimerstate = false;
    DebugPrintln("BRD005");
  } else {
    Timer1.setPeriod(get_motortimerinterval());
    Timer1.start();
    _motortimerstate = true;
  }
}


//-----------------------------------------------------------------------
// MOVE STEPPER MOTOR IN THE SPECIFIED DIRECTION
//-----------------------------------------------------------------------
void DriverBoard::movemotor(byte ddir) {
  if (ddir == 1) {
    // assume 1 = anticlockwise()
    Step++;
    if (myfocuser.stepmode == STEP1) {
      if (Step >= 4) {
        Step = 0;
      }
      digitalWrite(this->inputPins[0], bitRead(ulnfull[Step], 0));
      digitalWrite(this->inputPins[1], bitRead(ulnfull[Step], 1));
      digitalWrite(this->inputPins[2], bitRead(ulnfull[Step], 2));
      digitalWrite(this->inputPins[3], bitRead(ulnfull[Step], 3));
    } else {
      // assume half stepping
      if (Step >= 8) {
        Step = 0;
      }
      digitalWrite(this->inputPins[0], bitRead(ulnhalf[Step], 0));
      digitalWrite(this->inputPins[1], bitRead(ulnhalf[Step], 1));
      digitalWrite(this->inputPins[2], bitRead(ulnhalf[Step], 2));
      digitalWrite(this->inputPins[3], bitRead(ulnhalf[Step], 3));
    }
  } else {
    // assume 0 = clockwise
    Step--;
    if (myfocuser.stepmode == STEP1) {
      if (Step < 0) {
        Step = 3;
      }
      digitalWrite(this->inputPins[0], bitRead(ulnfull[Step], 0));
      digitalWrite(this->inputPins[1], bitRead(ulnfull[Step], 1));
      digitalWrite(this->inputPins[2], bitRead(ulnfull[Step], 2));
      digitalWrite(this->inputPins[3], bitRead(ulnfull[Step], 3));
    } else {
      // assume half stepping
      if (Step < 0) {
        Step = 7;
      }
      digitalWrite(this->inputPins[0], bitRead(ulnhalf[Step], 0));
      digitalWrite(this->inputPins[1], bitRead(ulnhalf[Step], 1));
      digitalWrite(this->inputPins[2], bitRead(ulnhalf[Step], 2));
      digitalWrite(this->inputPins[3], bitRead(ulnhalf[Step], 3));
    }
  }

#if defined(HOMEPOSITIONSWITCH)
  if (ddir == MOVINGIN) {
    hpswstate = get_hpsw();
  }
#endif
}


//-----------------------------------------------------------------------
// GET THE DELAY INTERVAL FOR THE MOTOR TIMER
//-----------------------------------------------------------------------
int DriverBoard::get_motortimerinterval() {
  int stepdelay;

  switch (myfocuser.stepmode) {
    case STEP1:
      stepdelay = driverboard->get_stepdelay();
      break;
    case STEP2:
      stepdelay = driverboard->get_stepdelay() / 2;
      break;
    case STEP4:
      stepdelay = driverboard->get_stepdelay() / 4;
      break;
    case STEP8:
      stepdelay = driverboard->get_stepdelay() / 8;
      break;
    case STEP16:
      stepdelay = driverboard->get_stepdelay() / 8;
      break;
    case STEP32:
      stepdelay = driverboard->get_stepdelay() / 8;
      break;
    default:
      stepdelay = driverboard->get_stepdelay();
      break;
  }
  return stepdelay;
}


//-----------------------------------------------------------------------
// GET THE DELAY INTERVAL BETWEEN STEPS FOR THE MOTOR TIMER
//-----------------------------------------------------------------------
int DriverBoard::get_stepdelay(void) {
  int sdelay = myfocuser.stepdelay;
  switch (myfocuser.motorspeed) {
    case SLOW:
      sdelay = sdelay + sdelay + sdelay;
      break;
    case MED:
      sdelay += sdelay;
      break;
    case FAST:
      //
      break;
    default:
      //
      break;
  }
  
#if defined(PUSHBUTTONS) && defined(PB_SPEED_BOOST)
  // Apply speed boost if long-press detected
  if (pb_boost == 1) {
    sdelay = sdelay; // PB_SPEED_BOOST_MULTIPLIER;
    // Enforce minimum delay floor
    //if (sdelay < PB_BOOST_MIN_DELAY_US) {
    //  sdelay = PB_BOOST_MIN_DELAY_US;
    //}
  }
#endif
  
  return sdelay;
}


//-----------------------------------------------------------------------
// void driverboard->set_stepdelay(unsigned long);
// deprecated
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// GET STEP MODE
//-----------------------------------------------------------------------
int DriverBoard::get_stepmode(void) {
  return myfocuser.stepmode;
}


//-----------------------------------------------------------------------
// SET STEP MODE
//-----------------------------------------------------------------------
void DriverBoard::set_stepmode() {
  if (myfocuser.stepmode <= 1) {
    myfocuser.stepmode = STEP1;
  } else {
    myfocuser.stepmode = STEP2;
  }
}
