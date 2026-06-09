
/*
  uno_esc_controller.ino
  Arduino UNO: reads 6 channel PWM signals from receiver outputs connected
  to A0..A5 (used as digital inputs) and drives 4 ESCs (motors) using Servo
  library. Implements arming via CH5 (A4) 3-way switch and uses CH6 (A5)
  potentiometer to scale max throttle.

  Wiring:
    Receiver CH1..CH6 -> UNO pins A0..A5 (these are digital-capable pins 14..19)
    ESC outputs (motors) -> UNO digital pins: 5, 6, 9, 10 (changeable)

  Arming rule (simple, because CH5 is a 3-way switch on D4):
    - CH5 pulse > 1700 us -> ARMED
    - CH5 pulse < 1300 us -> DISARMED
    - middle -> no change

  Potentiometer (CH6) use (connected to A4):
    - CH6 pulse is mapped (1000..2000) to a throttle scale (0.0..1.0).
    - This adjusts maximum throttle available to motors (soft limit).
*/

#include <Servo.h>

// input pins (reading PWM from receiver outputs)
// New mapping for UNO:
//  CH1 -> A0
//  CH2 -> A1
//  CH3 -> A2
//  CH4 -> A3
//  CH5 (arming switch PWM) -> D4
//  CH6 (pot/pwm) -> A4
const uint8_t IN_PINS[4] = {A0, A1, A2, A3}; // CH1..CH4
const uint8_t ARM_PIN = 4; // D4
const uint8_t POT_PIN = A4; // CH6

// ESC output pins (motors) - four motors mapped to CH1..CH4
const uint8_t ESC_PINS[4] = {3, 5, 6, 9};
Servo escs[4];

unsigned long lastPulse[6];

bool armed = false;
const uint16_t ARM_IDLE_US = 1100; // idle signal sent to ESCs when armed to ensure motors are on

void setup(){
  Serial.begin(115200);
  while(!Serial){}
  Serial.println("UNO ESC controller starting...");
  
  // Configure input pins (A0-A4 and D4) for reading PWM
  for(int i=0;i<4;i++) pinMode(IN_PINS[i], INPUT);
  pinMode(ARM_PIN, INPUT);
  pinMode(POT_PIN, INPUT);
  
  // attach ESC servos
  for(int i=0;i<4;i++) escs[i].attach(ESC_PINS[i]);
  // set motors to minimum (safe)
  for(int i=0;i<4;i++) escs[i].writeMicroseconds(1000);
}

// helper to read pulse width on a pin (use as digital input)
uint32_t readPulse(uint8_t pin){
  // pulseIn returns 0 if no pulse; timeout 30000us (30ms to cover full servo period)
  uint32_t p = pulseIn(pin, HIGH, 30000);
  if(p==0) return 1500; // fallback to center if no signal
  // constrain to valid range
  if(p < 800) return 1000;
  if(p > 2200) return 2000;
  return p;
}

void loop(){
  uint32_t ch[6];
  // Read CH1..CH4 from A0..A3
  for(int i=0;i<4;i++){
    ch[i] = readPulse(IN_PINS[i]);
  }
  // Read CH5 (arming) from D4
  ch[4] = readPulse(ARM_PIN);
  // Read CH6 (pot) from A4
  ch[5] = readPulse(POT_PIN);

  // print channels
  Serial.print("CH: ");
  for(int i=0;i<6;i++){ Serial.print(ch[i]); if(i<5) Serial.print(" "); }
  Serial.println();

    // arming by CH5 (index 4)
    if(ch[4] > 1700) {
      if(!armed){
        // arm sequence: ensure throttle low (throttle is CH1 -> ch[0])
        if(ch[0] < 1100) {
          armed = true;
          Serial.println("ARMED");
        } else {
          Serial.println("Move throttle to minimum to arm");
        }
      }
    } else if(ch[4] < 1300) {
      if(armed){
        armed = false;
        // cut motors
        for(int i=0;i<4;i++) escs[i].writeMicroseconds(1000);
        Serial.println("DISARMED");
      }
    }

  if(armed){
    // CH1=Throttle, CH2=Rudder(Yaw), CH3=Pitch, CH4=Roll (remapped for damaged joystick)
    // Normalize all inputs to -500..+500 range (centered at 1500)
    int16_t throttle = constrain(ch[0], 1000, 2000) - 1000; // 0..1000
    int16_t yaw = constrain(ch[1], 1000, 2000) - 1500;      // -500..+500
    int16_t pitch = constrain(ch[2], 1000, 2000) - 1500;    // -500..+500
    int16_t roll = constrain(ch[3], 1000, 2000) - 1500;     // -500..+500
    
    // CH6 potentiometer scales max throttle (1000..2000 -> 0.0..1.0)
    float scale = (float)(constrain(ch[5], 1000, 2000) - 1000) / 1000.0;
    throttle = (int16_t)(throttle * scale); // apply pot scaling to throttle only
    
    // Quadcopter motor mixing (X configuration):
    // Motor layout: 0=front-right, 1=rear-right, 2=rear-left, 3=front-left
    int16_t motor[4];
    motor[0] = throttle + pitch - roll - yaw; // front-right
    motor[1] = throttle - pitch - roll + yaw; // rear-right
    motor[2] = throttle - pitch + roll - yaw; // rear-left
    motor[3] = throttle + pitch + roll + yaw; // front-left
    
    // Apply to ESCs with idle floor and upper limit
    for(int i=0;i<4;i++){
      int16_t out = 1000 + motor[i];
      if(out < ARM_IDLE_US) out = ARM_IDLE_US; // keep motors spinning when armed
      if(out > 2000) out = 2000; // cap at max
      escs[i].writeMicroseconds(out);
    }
  } else {
    for(int i=0;i<4;i++) escs[i].writeMicroseconds(1000);
  }

  delay(50);
}
