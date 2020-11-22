#include "../hardware_api.h"

#if defined(ESP_H)

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

// empty motor slot 
#define _EMPTY_SLOT -20
#define _TAKEN_SLOT -21

// structure containing motor slot configuration
// this library supports up to 4 motors
typedef struct {
  int pinA;
  mcpwm_dev_t* mcpwm_num;
  mcpwm_unit_t mcpwm_unit;
  mcpwm_operator_t mcpwm_operator;
  mcpwm_io_signals_t mcpwm_a;
  mcpwm_io_signals_t mcpwm_b;
  mcpwm_io_signals_t mcpwm_c;
} bldc_3pwm_motor_slots_t;
typedef struct {
  int pin1A;
  mcpwm_dev_t* mcpwm_num;
  mcpwm_unit_t mcpwm_unit;
  mcpwm_operator_t mcpwm_operator1;
  mcpwm_operator_t mcpwm_operator2;
  mcpwm_io_signals_t mcpwm_1a;
  mcpwm_io_signals_t mcpwm_1b;
  mcpwm_io_signals_t mcpwm_2a;
  mcpwm_io_signals_t mcpwm_2b;
} stepper_motor_slots_t;

typedef struct {
  int pinAH;
  mcpwm_dev_t* mcpwm_num;
  mcpwm_unit_t mcpwm_unit;
  mcpwm_operator_t mcpwm_operator1;
  mcpwm_operator_t mcpwm_operator2;
  mcpwm_io_signals_t mcpwm_ah;
  mcpwm_io_signals_t mcpwm_bh;
  mcpwm_io_signals_t mcpwm_ch;
  mcpwm_io_signals_t mcpwm_al;
  mcpwm_io_signals_t mcpwm_bl;
  mcpwm_io_signals_t mcpwm_cl;
} bldc_6pwm_motor_slots_t;

// define bldc motor slots array
bldc_3pwm_motor_slots_t esp32_bldc_3pwm_motor_slots[4] =  { 
  {_EMPTY_SLOT, &MCPWM0, MCPWM_UNIT_0, MCPWM_OPR_A, MCPWM0A, MCPWM1A, MCPWM2A}, // 1st motor will be MCPWM0 channel A
  {_EMPTY_SLOT, &MCPWM0, MCPWM_UNIT_0, MCPWM_OPR_B, MCPWM0B, MCPWM1B, MCPWM2B}, // 2nd motor will be MCPWM0 channel B
  {_EMPTY_SLOT, &MCPWM1, MCPWM_UNIT_1, MCPWM_OPR_A, MCPWM0A, MCPWM1A, MCPWM2A}, // 3rd motor will be MCPWM1 channel A
  {_EMPTY_SLOT, &MCPWM1, MCPWM_UNIT_1, MCPWM_OPR_B, MCPWM0B, MCPWM1B, MCPWM2B}  // 4th motor will be MCPWM1 channel B
  };

// define stepper motor slots array
stepper_motor_slots_t esp32_stepper_motor_slots[2] =  { 
  {_EMPTY_SLOT, &MCPWM0, MCPWM_UNIT_0, MCPWM_OPR_A, MCPWM_OPR_B, MCPWM0A, MCPWM1A, MCPWM0B, MCPWM1B}, // 1st motor will be on MCPWM0
  {_EMPTY_SLOT, &MCPWM1, MCPWM_UNIT_1, MCPWM_OPR_A, MCPWM_OPR_B, MCPWM0A, MCPWM1A, MCPWM0B, MCPWM1B}, // 1st motor will be on MCPWM1
  };  

// define stepper motor slots array
bldc_6pwm_motor_slots_t esp32_bldc_6pwm_motor_slots[2] =  { 
  {_EMPTY_SLOT, &MCPWM0, MCPWM_UNIT_0, MCPWM_OPR_A, MCPWM_OPR_B, MCPWM0A, MCPWM1A, MCPWM2A, MCPWM0B, MCPWM1B, MCPWM2B}, // 1st motor will be on MCPWM0
  {_EMPTY_SLOT, &MCPWM1, MCPWM_UNIT_1, MCPWM_OPR_A, MCPWM_OPR_B, MCPWM0A, MCPWM1A, MCPWM2A, MCPWM0B, MCPWM1B, MCPWM2B}, // 1st motor will be on MCPWM0
  };  

// configuring high frequency pwm timer
// a lot of help from this post from Paul Gauld
// https://hackaday.io/project/169905-esp-32-bldc-robot-actuator-controller
void _configureTimerFrequency(long pwm_frequency, mcpwm_dev_t* mcpwm_num,  mcpwm_unit_t mcpwm_unit, float dead_zone = NOT_SET){

  mcpwm_config_t pwm_config;
  pwm_config.frequency = pwm_frequency;  //frequency
  pwm_config.counter_mode = MCPWM_UP_DOWN_COUNTER; // Up-down counter (triangle wave)
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0; // Active HIGH
  mcpwm_init(mcpwm_unit, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
  mcpwm_init(mcpwm_unit, MCPWM_TIMER_1, &pwm_config);    //Configure PWM0A & PWM0B with above settings
  mcpwm_init(mcpwm_unit, MCPWM_TIMER_2, &pwm_config);    //Configure PWM0A & PWM0B with above settings

  if (dead_zone != NOT_SET){
    // dead zone is configured in dead_time x 100 nanoseconds 
    float dead_time = (float)(1e7 / pwm_frequency) * dead_zone;  
    mcpwm_deadtime_enable(mcpwm_unit, MCPWM_TIMER_0, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, dead_time, dead_time); 
    mcpwm_deadtime_enable(mcpwm_unit, MCPWM_TIMER_1, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, dead_time, dead_time);     
    mcpwm_deadtime_enable(mcpwm_unit, MCPWM_TIMER_2, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, dead_time, dead_time);
  }

  _delay(100);

  mcpwm_sync_enable(mcpwm_unit, MCPWM_TIMER_0, MCPWM_SELECT_SYNC_INT0, 0);
  mcpwm_sync_enable(mcpwm_unit, MCPWM_TIMER_1, MCPWM_SELECT_SYNC_INT0, 0);
  mcpwm_sync_enable(mcpwm_unit, MCPWM_TIMER_2, MCPWM_SELECT_SYNC_INT0, 0);
  _delay(1);
  mcpwm_num->timer[0].sync.out_sel = 1;
  _delay(1);
  mcpwm_num->timer[0].sync.out_sel = 0;
}


// function setting the high pwm frequency to the supplied pins
// - BLDC motor - 3PWM setting
// - hardware speciffic
// supports Arudino/ATmega328, STM32 and ESP32 
void _configure3PWM(long pwm_frequency,const int pinA, const int pinB, const int pinC) {

  if(!pwm_frequency || pwm_frequency == NOT_SET) pwm_frequency = 40000; // default frequency 20khz - centered pwm has twice lower frequency
  else pwm_frequency = _constrain(2*pwm_frequency, 0, 100000); // constrain to 50kHz max - centered pwm has twice lower frequency

  bldc_3pwm_motor_slots_t m_slot = {};

  // determine which motor are we connecting
  // and set the appropriate configuration parameters 
  int slot_num;
  for(slot_num = 0; slot_num < 4; slot_num++){
    if(esp32_bldc_3pwm_motor_slots[slot_num].pinA == _EMPTY_SLOT){ // put the new motor in the first empty slot
      esp32_bldc_3pwm_motor_slots[slot_num].pinA = pinA;
      m_slot = esp32_bldc_3pwm_motor_slots[slot_num];
      break;
    }
  }
  // disable all the slots with the same MCPWM 
  if( slot_num < 2 ){
    // slot 0 of the stepper
    esp32_stepper_motor_slots[0].pin1A = _TAKEN_SLOT;
    // slot 0 of the 6pwm bldc
    esp32_bldc_6pwm_motor_slots[0].pinAH = _TAKEN_SLOT;
  }else{
    // slot 1 of the stepper
    esp32_stepper_motor_slots[1].pin1A = _TAKEN_SLOT;
    // slot 1 of the 6pwm bldc
    esp32_bldc_6pwm_motor_slots[1].pinAH = _TAKEN_SLOT;
  }
  // configure pins
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_a, pinA);
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_b, pinB);
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_c, pinC);

  // configure the timer
  _configureTimerFrequency(pwm_frequency, m_slot.mcpwm_num,  m_slot.mcpwm_unit);

}


// function setting the high pwm frequency to the supplied pins
// - Stepper motor - 4PWM setting
// - hardware speciffic
void _configure4PWM(long pwm_frequency,const int pinA, const int pinB, const int pinC, const int pinD) {

  if(!pwm_frequency || pwm_frequency == NOT_SET) pwm_frequency = 40000; // default frequency 20khz - centered pwm has twice lower frequency
  else pwm_frequency = _constrain(2*pwm_frequency, 0, 100000); // constrain to 50kHz max - centered pwm has twice lower frequency
  stepper_motor_slots_t m_slot = {};
  // determine which motor are we connecting
  // and set the appropriate configuration parameters 
  int slot_num;
  for(slot_num = 0; slot_num < 2; slot_num++){
    if(esp32_stepper_motor_slots[slot_num].pin1A == _EMPTY_SLOT){ // put the new motor in the first empty slot
      esp32_stepper_motor_slots[slot_num].pin1A = pinA;
      m_slot = esp32_stepper_motor_slots[slot_num];
      break;
    }
  }
  // disable all the slots with the same MCPWM 
  if( slot_num == 0 ){
    // slots 0 and 1 of the 3pwm bldc
    esp32_bldc_3pwm_motor_slots[0].pinA = _TAKEN_SLOT;
    esp32_bldc_3pwm_motor_slots[1].pinA = _TAKEN_SLOT;
    // slot 0 of the 6pwm bldc
    esp32_bldc_6pwm_motor_slots[0].pinAH = _TAKEN_SLOT;
  }else{
    // slots 2 and 3 of the 3pwm bldc
    esp32_bldc_3pwm_motor_slots[2].pinA = _TAKEN_SLOT;
    esp32_bldc_3pwm_motor_slots[3].pinA = _TAKEN_SLOT;
    // slot 1 of the 6pwm bldc
    esp32_bldc_6pwm_motor_slots[1].pinAH = _TAKEN_SLOT;
  } 

  // configure pins
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_1a, pinA);
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_1b, pinB);
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_2a, pinC);
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_2b, pinD);

  // configure the timer
  _configureTimerFrequency(pwm_frequency, m_slot.mcpwm_num,  m_slot.mcpwm_unit);
}

// function setting the pwm duty cycle to the hardware
// - BLDC motor - 3PWM setting
// - hardware speciffic
//  ESP32 uses MCPWM
void _writeDutyCycle3PWM(float dc_a,  float dc_b, float dc_c, int pinA, int pinB, int pinC){
  // determine which motor slot is the motor connected to 
  for(int i = 0; i < 4; i++){
    if(esp32_bldc_3pwm_motor_slots[i].pinA == pinA){ // if motor slot found
      // se the PWM on the slot timers
      // transform duty cycle from [0,1] to [0,100]
      mcpwm_set_duty(esp32_bldc_3pwm_motor_slots[i].mcpwm_unit, MCPWM_TIMER_0, esp32_bldc_3pwm_motor_slots[i].mcpwm_operator, dc_a*100.0);
      mcpwm_set_duty(esp32_bldc_3pwm_motor_slots[i].mcpwm_unit, MCPWM_TIMER_1, esp32_bldc_3pwm_motor_slots[i].mcpwm_operator, dc_b*100.0);
      mcpwm_set_duty(esp32_bldc_3pwm_motor_slots[i].mcpwm_unit, MCPWM_TIMER_2, esp32_bldc_3pwm_motor_slots[i].mcpwm_operator, dc_c*100.0);
      break;
    }
  }
}

// function setting the pwm duty cycle to the hardware
// - Stepper motor - 4PWM setting
// - hardware speciffic
//  ESP32 uses MCPWM
void _writeDutyCycle4PWM(float dc_1a,  float dc_1b, float dc_2a, float dc_2b, int pin1A, int pin1B, int pin2A, int pin2B){
  // determine which motor slot is the motor connected to 
  for(int i = 0; i < 2; i++){
    if(esp32_stepper_motor_slots[i].pin1A == pin1A){ // if motor slot found
      // se the PWM on the slot timers
      // transform duty cycle from [0,1] to [0,100]
      mcpwm_set_duty(esp32_stepper_motor_slots[i].mcpwm_unit, MCPWM_TIMER_0, esp32_stepper_motor_slots[i].mcpwm_operator1, dc_1a*100.0);
      mcpwm_set_duty(esp32_stepper_motor_slots[i].mcpwm_unit, MCPWM_TIMER_1, esp32_stepper_motor_slots[i].mcpwm_operator1, dc_1b*100.0);
      mcpwm_set_duty(esp32_stepper_motor_slots[i].mcpwm_unit, MCPWM_TIMER_0, esp32_stepper_motor_slots[i].mcpwm_operator2, dc_2a*100.0);
      mcpwm_set_duty(esp32_stepper_motor_slots[i].mcpwm_unit, MCPWM_TIMER_1, esp32_stepper_motor_slots[i].mcpwm_operator2, dc_2b*100.0);
      break;
    }
  }
}

// Configuring PWM frequency, resolution and alignment
// - BLDC driver - 6PWM setting
// - hardware specific
int _configure6PWM(long pwm_frequency, float dead_zone, const int pinA_h, const int pinA_l,  const int pinB_h, const int pinB_l, const int pinC_h, const int pinC_l){
  
  if(!pwm_frequency || pwm_frequency == NOT_SET) pwm_frequency = 40000; // default frequency 20khz - centered pwm has twice lower frequency
  else pwm_frequency = _constrain(2*pwm_frequency, 0, 60000); // constrain to 30kHz max - centered pwm has twice lower frequency
  bldc_6pwm_motor_slots_t m_slot = {};
  // determine which motor are we connecting
  // and set the appropriate configuration parameters 
  int slot_num;
  for(slot_num = 0; slot_num < 2; slot_num++){
    if(esp32_bldc_6pwm_motor_slots[slot_num].pinAH == _EMPTY_SLOT){ // put the new motor in the first empty slot
      esp32_bldc_6pwm_motor_slots[slot_num].pinAH = pinA_h;
      m_slot = esp32_bldc_6pwm_motor_slots[slot_num];
      break;
    }
  }
  // if no slots available
  if(slot_num >= 2) return -1;

  // disable all the slots with the same MCPWM 
  if( slot_num == 0 ){
    // slots 0 and 1 of the 3pwm bldc
    esp32_bldc_3pwm_motor_slots[0].pinA = _TAKEN_SLOT;
    esp32_bldc_3pwm_motor_slots[1].pinA = _TAKEN_SLOT;
    // slot 0 of the 6pwm bldc
    esp32_stepper_motor_slots[0].pin1A = _TAKEN_SLOT;
  }else{
    // slots 2 and 3 of the 3pwm bldc
    esp32_bldc_3pwm_motor_slots[2].pinA = _TAKEN_SLOT;
    esp32_bldc_3pwm_motor_slots[3].pinA = _TAKEN_SLOT;
    // slot 1 of the 6pwm bldc
    esp32_stepper_motor_slots[1].pin1A = _TAKEN_SLOT;
  } 

  // configure pins
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_ah, pinA_h);
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_al, pinA_l);
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_bh, pinB_h);
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_bl, pinB_l);
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_ch, pinC_h);
  mcpwm_gpio_init(m_slot.mcpwm_unit, m_slot.mcpwm_cl, pinC_l);

  // configure the timer
  _configureTimerFrequency(pwm_frequency, m_slot.mcpwm_num,  m_slot.mcpwm_unit, dead_zone);
  // return 
  return 0;
}

// Function setting the duty cycle to the pwm pin (ex. analogWrite())
// - BLDC driver - 6PWM setting
// - hardware specific
void _writeDutyCycle6PWM(float dc_a,  float dc_b, float dc_c, float dead_zone, int pinA_h, int pinA_l, int pinB_h, int pinB_l, int pinC_h, int pinC_l){
  // determine which motor slot is the motor connected to 
  for(int i = 0; i < 2; i++){
    if(esp32_bldc_6pwm_motor_slots[i].pinAH == pinA_h){ // if motor slot found
      // se the PWM on the slot timers
      // transform duty cycle from [0,1] to [0,100.0]
      mcpwm_set_duty(esp32_bldc_6pwm_motor_slots[i].mcpwm_unit, MCPWM_TIMER_0, MCPWM_OPR_A, dc_a*100.0);
      mcpwm_set_duty(esp32_bldc_6pwm_motor_slots[i].mcpwm_unit, MCPWM_TIMER_0, MCPWM_OPR_B, dc_a*100.0);
      mcpwm_set_duty(esp32_bldc_6pwm_motor_slots[i].mcpwm_unit, MCPWM_TIMER_1, MCPWM_OPR_A, dc_b*100.0);
      mcpwm_set_duty(esp32_bldc_6pwm_motor_slots[i].mcpwm_unit, MCPWM_TIMER_1, MCPWM_OPR_B, dc_b*100.0);
      mcpwm_set_duty(esp32_bldc_6pwm_motor_slots[i].mcpwm_unit, MCPWM_TIMER_2, MCPWM_OPR_A, dc_c*100.0);
      mcpwm_set_duty(esp32_bldc_6pwm_motor_slots[i].mcpwm_unit, MCPWM_TIMER_2, MCPWM_OPR_B, dc_c*100.0);
      break;
    }
  }
}
#endif