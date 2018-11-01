#include <Fsm.h>
#include <MD_DS3231.h>
#include <Wire.h>
#include <Dusk2Dawn.h>

/*************************************************************
Motor Shield Stepper Demo
by Randy Sarafan

For more information see:
https://www.instructables.com/id/Arduino-Motor-Shield-Tutorial/

*************************************************************/

// functionnal variable
int delaylength   = 5;     // delay between motor moves
int rotations     = 2100;  // number of iteration to open the door
int safety_button = 7;     // I/O pin to stop motor in open position
int toggle_button = 6;     // I/O pin to toggle door

// PIN 4 and 5 are reserved for RTC

int previous      = 0;   //debounce signal from pin

// State machine variables
#define TOGGLE_DOOR  1
#define OPEN_DOOR    2
#define CLOSE_DOOR   3

State started(&on_init, NULL, NULL);  // init here
State state_open(&on_open, &checkInterrupts, NULL); // door is opened
State state_closed(&on_close, &checkInterrupts, NULL); // door closed
Fsm fsm(&started);

// Multiple instances can be created. Arguments are longitude, latitude, and
// time zone offset in hours from UTC.
//
// The first two must be in decimal degrees (DD), e.g. 10.001, versus the more
// common degrees, minutes, and seconds format (DMS), e.g. 10° 00′ 3.6″. The
// time zone offset can be expressed in decimal fractions, e.g. "5.75" for
// Nepal Standard Time, in the few cases where the zones are offset by 30 or
// 45 minutes.
//
// HINT: An easy way to find the longitude and latitude for any arbitrary
// location is to find the spot in Google Maps, right click the place on the
// map, and select "What's here?". At the bottom, you’ll see a card with the
// coordinates.

// edit this to reflect your position (Beware, Time HAS TO BE SET TO UTC otherwise use the shift ( third arg to compensate )
Dusk2Dawn myCoordinates(48.585995, 7.506461, 0);

/** setup the state machine **/
void setup() {
  Serial.begin(9600);
  fsm.add_transition(&started, &state_open, TOGGLE_DOOR, NULL );
  fsm.add_transition(&state_open, &state_closed, TOGGLE_DOOR, NULL );
  fsm.add_transition(&state_closed, &state_open, TOGGLE_DOOR, NULL );
  fsm.add_transition(&state_open, &state_closed, CLOSE_DOOR, NULL );
  fsm.add_transition(&state_closed, &state_open, OPEN_DOOR, NULL );
  fsm.add_timed_transition(&started, &state_open, 10, NULL);
  Serial.println("State machine parametered");
}


/**
 * prepare clock alarm to open and close the door
 **/
void setupClock(){
  RTC.control(DS3231_12H, DS3231_OFF);  // 24 hour clock
  setOpenAlarm();
  setCloseAlarm();
}

/**
 * prepare clock alarm to open the door
 **/
void setOpenAlarm() {
  RTC.readTime();
  int sunrise = myCoordinates.sunrise(RTC.yyyy, RTC.mm, RTC.dd, false);
  int open_hour = (int) sunrise / 60;
  int open_minute = sunrise % 60;
  Serial.print("setting alarm 1 at ");
  Serial.print(open_hour);
  Serial.print(":");
  Serial.println(open_minute);
  RTC.setAlarm1Callback(openDoor);
  RTC.yyyy = RTC.mm = RTC.dd = 0;
  RTC.h = open_hour;
  RTC.m = open_minute;
  RTC.s = RTC.dow = 0;
  RTC.writeAlarm1(DS3231_ALM_HMS);
}

/**
 * prepare clock alarm to close the door
 **/
void setCloseAlarm() {
  RTC.readTime();
  int sunset = myCoordinates.sunset(RTC.yyyy, RTC.mm, RTC.dd, false);
  int close_hour = (int) sunset / 60;
  int close_minute = sunset % 60;
  Serial.print("setting alarm 2 at ");
  Serial.print(close_hour);
  Serial.print(":");
  Serial.println(close_minute);

  RTC.setAlarm2Callback(closeDoor);
  RTC.yyyy = RTC.mm = RTC.dd = 0;
  RTC.h = close_hour;
  RTC.m = close_minute;
  RTC.s = RTC.dow = 0;
  RTC.writeAlarm2(DS3231_ALM_HM);
}

void loop() {
  fsm.run_machine();
}



/** check external button **/
void checkInterrupts() {
  if(digitalRead(toggle_button)==0 && previous ==0 ) {
    toggleDoor();
    previous = 1;
  } else {
    previous = 0;
  }
  
  RTC.checkAlarm1();
  RTC.checkAlarm2();
  
}

// send signal to trigger door movement
void toggleDoor() {
   fsm.trigger(TOGGLE_DOOR);
}


// send signal to open the door
void openDoor() {
  Serial.println("Alarm is opening the black hole...");
  fsm.trigger(OPEN_DOOR);
}

// send signal to close the door
void closeDoor() {
   Serial.println("Alarm is closing the black hole...");
   fsm.trigger(CLOSE_DOOR);
}

/**
 * init State, pins mode and so on
 */
 void on_init() {
  // initialize serial communication at 9600 bits per second:
  delay(1000);
  Serial.println("starting process");

  Serial.println("intializing pins");
   //establish motor direction toggle pins
  pinMode(12, OUTPUT); //CH A -- HIGH = forwards and LOW = backwards???
  pinMode(13, OUTPUT); //CH B -- HIGH = forwards and LOW = backwards???
  
  //establish motor brake pins
  pinMode(9, OUTPUT); //brake (disable) CH A
  pinMode(8, OUTPUT); //brake (disable) CH B
  
  pinMode(safety_button, INPUT_PULLUP); // security button
  pinMode(toggle_button, INPUT_PULLUP); // toggle button
  
  Serial.println("calibrating subspace drift...");
  setupClock();
  
 }

/**
 * entering open mode
 */
void on_open(){
  Serial.println("Warp gate opening...");
  while(digitalRead(safety_button) == 0) rotateMotor(HIGH, LOW);
  Serial.println("Warp gate OPENED !!!");
  powerOff();
  setupClock();
}

/**
 * entering ready mode
 */
void on_close(){
  Serial.println("Dark matter emission detected, shutting down warp gate...");
  for(int i = 0; i< rotations; i++) rotateMotor(LOW, HIGH);
  powerOff();
  setupClock();
}

/**
 * shuts down power to motor shield
 */
void powerOff() {
  digitalWrite(9, LOW);  //ENABLE CH A
  digitalWrite(8, LOW); //DISABLE CH B
  analogWrite(11, 0);  
  analogWrite(3, 0);  
}

/**
 * performs motor rotation
 */
void rotateMotor(int first_direction, int second_direction){
 
  digitalWrite(9, LOW);  //ENABLE CH A
  digitalWrite(8, HIGH); //DISABLE CH B

  digitalWrite(12, HIGH);   //Sets direction of CH A
  analogWrite(3, 255);   //Moves CH A
  
  delay(delaylength);
  
  digitalWrite(9, HIGH);  //DISABLE CH A
  digitalWrite(8, LOW); //ENABLE CH B

  digitalWrite(13, first_direction);   //Sets direction of CH B
  analogWrite(11, 255);   //Moves CH B
  
  delay(delaylength);
  
  
  
  digitalWrite(9, LOW);  //ENABLE CH A
  digitalWrite(8, HIGH); //DISABLE CH B

  digitalWrite(12, LOW);   //Sets direction of CH A
  analogWrite(3, 255);   //Moves CH A
  
  delay(delaylength);
    
  digitalWrite(9, HIGH);  //DISABLE CH A
  digitalWrite(8, LOW); //ENABLE CH B

  digitalWrite(13, second_direction);   //Sets direction of CH B
  analogWrite(11, 255);   //Moves CH B
  
  delay(delaylength);

}

