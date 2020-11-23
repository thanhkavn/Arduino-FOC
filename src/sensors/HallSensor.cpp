#include "HallSensor.h"


/*
  HallSensor(int hallA, int hallB , int cpr, int index)
  - hallA, hallB, hallC    - HallSensor A, B and C pins
  - pp           - pole pairs
*/
HallSensor::HallSensor(int _hallA, int _hallB, int _hallC, int _pp){
  
  // hardware pins
  pinA = _hallA;
  pinB = _hallB;
  pinC = _hallC;

  // hall has 6 segments per electrical revolution
  cpr = _pp * 6; 

  // extern pullup as default
  pullup = Pullup::EXTERN;
}

//  HallSensor interrupt callback functions
// A channel
void HallSensor::handleA() {
  A_active= digitalRead(pinA);
  updateState();
}
// B channel
void HallSensor::handleB() {
  B_active = digitalRead(pinB);
  updateState();
}

// C channel
void HallSensor::handleC() {
  C_active = digitalRead(pinC);
  updateState();
}

/**
 * Updates the state and sector following an interrupt
 */ 
void HallSensor::updateState() {
  long new_pulse_timestamp = _micros();
  hall_state = C_active + (B_active << 1) + (A_active << 2);
  int8_t new_electric_sector = ELECTRIC_SECTORS[hall_state];
  static Direction old_direction;
  if (new_electric_sector - electric_sector > 3) {
    //underflow
    direction = static_cast<Direction>(natural_direction * -1);
    electric_rotations += direction;
  } else if (new_electric_sector - electric_sector < (-3)) {
    //overflow
    direction = static_cast<Direction>(natural_direction);
    electric_rotations += direction;
  } else {
    direction = (new_electric_sector > electric_sector)? static_cast<Direction>(natural_direction) : static_cast<Direction>(natural_direction * (-1));
  }
  electric_sector = new_electric_sector;
  if (direction == old_direction) {
    // not oscilating or just changed direction
    pulse_diff = new_pulse_timestamp - pulse_timestamp;
  } else {
    pulse_diff = 0;
  }
  
  pulse_timestamp = new_pulse_timestamp;
  total_interrupts++;
  old_direction = direction;
  if (onSectorChange != nullptr) onSectorChange(electric_sector);
}

/**
 * Optionally set a function callback to be fired when sector changes
 * void onSectorChange(int sector) {
 *  ... // for debug or call driver directly?
 * }
 * sensor.attachSectorCallback(onSectorChange);
 */ 
void HallSensor::attachSectorCallback(void (*_onSectorChange)(int sector)) {
  onSectorChange = _onSectorChange;
}

/*
	Shaft angle calculation
*/
float HallSensor::getAngle() {
  return natural_direction * ((electric_rotations * 6 + electric_sector) / cpr) * _2PI;
}

/*
  Shaft velocity calculation
  function using mixed time and frequency measurement technique
*/
float HallSensor::getVelocity(){
  if (pulse_diff == 0 || ((_micros() - pulse_timestamp) > pulse_diff) ) { // last velocity isn't accurate if too old
    return 0;
  } else {
    return direction * (_2PI / cpr) / (pulse_diff / 1000000.0);
  }

}

// getter for index pin
// return -1 if no index
int HallSensor::needsAbsoluteZeroSearch(){
  return 0;
}

int HallSensor::hasAbsoluteZero(){
  return 1;
}

// set current angle as zero angle 
// return the angle [rad] difference
float HallSensor::initRelativeZero(){

  // nothing to do.  The interrupts should have changed sector.
  electric_rotations = 0;
  return 0;

}

// set absolute zero angle as zero angle
// return the angle [rad] difference
float HallSensor::initAbsoluteZero(){

  return -getAngle();
  
}

// HallSensor initialisation of the hardware pins 
// and calculation variables
void HallSensor::init(){
  
  // HallSensor - check if pullup needed for your HallSensor
  if(pullup == Pullup::INTERN){
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    pinMode(pinC, INPUT_PULLUP);
  }else{
    pinMode(pinA, INPUT);
    pinMode(pinB, INPUT);
    pinMode(pinC, INPUT);
  }

    // init hall_state
  A_active= digitalRead(pinA);
  B_active = digitalRead(pinB);
  C_active = digitalRead(pinC);
  updateState();
  
  pulse_timestamp = _micros();

}

// function enabling hardware interrupts for the callback provided
// if callback is not provided then the interrupt is not enabled
void HallSensor::enableInterrupts(void (*doA)(), void(*doB)(), void(*doC)()){
  // attach interrupt if functions provided

  // A, B and C callback
  if(doA != nullptr) attachInterrupt(digitalPinToInterrupt(pinA), doA, CHANGE);
  if(doB != nullptr) attachInterrupt(digitalPinToInterrupt(pinB), doB, CHANGE);
  if(doC != nullptr) attachInterrupt(digitalPinToInterrupt(pinC), doC, CHANGE);
}

