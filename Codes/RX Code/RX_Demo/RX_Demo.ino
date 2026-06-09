//
  //_6ch_rx_servo.ino
//  Receiver: receives 6x uint16 channels via nRF24 and:
  // - prints channels to Serial
   //- outputs servo/ESC-style pulses on 6 pins (writeMicroseconds)

  //Output pin mapping (as in your image):
    //CH1 -> D9
    //CH2 -> D2
    //CH3 -> D3
    //CH4 -> D4
    //CH5 -> D5
    //CH6 -> D6

  //nRF wiring: CE=D7, CSN=D8
//*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

const uint8_t CE_PIN = 7;
const uint8_t CSN_PIN = 8;
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

const uint8_t OUT_PINS[6] = {9, 2, 3, 4, 5, 6};
Servo outs[6];

uint8_t buf[32];
unsigned long lastRx = 0;
bool armed = false;
volatile bool wakeTick = false;

ISR(WDT_vect){
  wakeTick = true;
}

void setup(){
  Serial.begin(115200);
  while(!Serial){}
  Serial.println("RX final starting...");
  if(!radio.begin()){ Serial.println("Radio hardware not responding"); while(1){} }
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(76);
  radio.enableDynamicPayloads();
  radio.openReadingPipe(0, address);
  radio.startListening();
  radio.printDetails();

  for(int i=0;i<6;i++) outs[i].attach(OUT_PINS[i]);
  // initialize outputs to minimum (safe)
  for(int i=0;i<6;i++) outs[i].writeMicroseconds(1000);

  set_sleep_mode(SLEEP_MODE_IDLE);
  cli();
  MCUSR &= ~(1 << WDRF);
  WDTCSR = (1 << WDCE) | (1 << WDE);
  WDTCSR = (1 << WDIE) | (1 << WDP0);
  sei();
}

void printRaw(uint8_t *data, size_t len){
  Serial.print("Raw bytes ["); Serial.print(len); Serial.print("]: ");
  for(size_t i=0;i<len;i++){ if(data[i]<0x10) Serial.print('0'); Serial.print(data[i], HEX); Serial.print(' '); }
  Serial.println();
}

void loop(){
  if(!wakeTick){
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    return;
  }
  wakeTick = false;

  if(radio.available()){
    uint8_t len = radio.getDynamicPayloadSize();
    if(len>32) len=32;
    radio.read(buf, len);
    lastRx = millis();
    Serial.print("Received (bytes="); Serial.print(len); Serial.print("): ");
    printRaw(buf, len);

    // try parse as 6x uint16
    if(len >= 12){
      uint16_t ch[6];
      for(int i=0;i<6;i++) ch[i] = buf[2*i] | (uint16_t(buf[2*i+1])<<8);
      Serial.print("As uint16: ");
      for(int i=0;i<6;i++){ Serial.print("CH"); Serial.print(i+1); Serial.print(":"); Serial.print(ch[i]); if(i<5) Serial.print("  "); }
      Serial.println();
      // Arming by CH5 (index 4). CH1 (index 0) must be low to arm (throttle on CH1).
      if(ch[4] > 1700) {
        if(!armed){
          if(ch[0] < 1100){
            armed = true;
            Serial.println("ARMED");
          } else {
            Serial.println("Move throttle to minimum to arm");
          }
        }
      } else if(ch[4] < 1300) {
        if(armed){
          armed = false;
          Serial.println("DISARMED");
        }
      }
      // apply to outputs (constrain safety). If disarmed, force minimum output.
      for(int i=0;i<6;i++){
        uint16_t out = constrain(ch[i], 1000, 2000);
        if(!armed) out = 1000;
        outs[i].writeMicroseconds(out);
      }
    }
    Serial.println("----");
  }
  // if RX lost for >500ms, set outputs to safe minimum
  if(millis() - lastRx > 500){
    for(int i=0;i<6;i++) outs[i].writeMicroseconds(1000);
  }
}

