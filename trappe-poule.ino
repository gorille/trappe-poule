#include <Fsm.h>
#include <MD_DS3231.h>
#include <Wire.h>

/*************************************************************
trappe a poule
https://github.com/gorille/trappe-poule
*************************************************************/

// functionnal variable
int delaylength   = 5;     // delay between motor moves
int rotations     = 2100;  // number of iteration to open the door
int safety_button = 7;     // I/O pin to stop motor in open position
int toggle_button = 6;     // I/O pin to toggle door

int open_hour     = 7;     // time ( hour ) to which open the door
int open_minute   = 0;     // time ( minute ) to which open the door
int close_hour    = 21;    // time ( hour ) to which close the door
int close_minute  = 0;     // time ( minute ) to which close the door

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
  Serial.println("setting alarm 1");
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
  Serial.println("setting alarm 2");
  RTC.setAlarm2Callback(closeDoor);
  RTC.yyyy = RTC.mm = RTC.dd = 0;
  RTC.h = close_hour;
  RTC.m = close_minute;
  RTC.s = RTC.dow = 0;
  RTC.writeAlarm2(DS3231_ALM_HM);
}

void loop() {
  fsm.run_machine();
  printTime();
}


/**
 * print current time
 * */
void printTime() {
  RTC.readTime();
  Serial.print(RTC.h);
  Serial.print(":");
  Serial.print(RTC.m);
  Serial.print(":");
  Serial.println(RTC.s);
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
  digitalWrite(9, LOW);  //ENABLE CH A
  digitalWrite(8, LOW); //DISABLE CH B
  analogWrite(11, 0);  
  analogWrite(3, 0);  
}

/**
 * entering ready mode
 */
void on_close(){
  Serial.println("Dark matter emission detected, shutting down warp gate...");
  for(int i = 0; i< rotations; i++) rotateMotor(LOW, HIGH);
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
