/*
  _6ch_tx_final.ino
  Final transmitter: reads 6 controls (joysticks/pots) on analog pins,
  prints channels on Serial and transmits 6x uint16 (1000-2000) via nRF24.

  Pin mapping (per your image):
    CH1 AILERON  -> A3
    CH2 ELEVATOR -> A2
    CH3 THROTTLE -> A0
    CH4 RUDDER   -> A1
    CH5 AUX1     -> A7 (3-way switch)
      CH6 AUX2     -> D2 (3-way/arming switch)

  nRF wiring: CE=D7, CSN=D8, MOSI=D11, MISO=D12, SCK=D13
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <avr/sleep.h>

const uint8_t CE_PIN = 7;
const uint8_t CSN_PIN = 8;
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

// CH mapping updated per image and user request:
//   CH1 (throttle) -> A3  (yellow)
//   CH2 (rudder)   -> A2  (yellow)
//   CH3            -> A0  (green)
//   CH4            -> A1  (green)
//   CH5 (arming)   -> D2  (digital, 3-way switch)
//   CH6 (pot)      -> A7
const uint8_t CH_PINS[5] = {A3, A2, A0, A1, A7};
const uint8_t SWITCH_PIN = 2; // D2 on Nano for 3-way arming switch (CH5)
volatile bool sampleTick = false;

ISR(TIMER1_COMPA_vect){
  sampleTick = true;
}

void setup(){
  Serial.begin(115200);
  while(!Serial){}
  Serial.println("TX final starting...");
  if(!radio.begin()){ Serial.println("radio.begin() failed"); while(1){} }
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(76);
  radio.setRetries(5, 15);
  radio.enableDynamicPayloads();
  // For control links you may prefer ACKs enabled for reliability.
  radio.setAutoAck(true);
  radio.openWritingPipe(address);
  radio.stopListening();
  radio.printDetails();
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  set_sleep_mode(SLEEP_MODE_IDLE);
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 9999;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11) | (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

uint8_t payload[12];

void printRaw(uint8_t *data, size_t len){
  Serial.print("Raw bytes ["); Serial.print(len); Serial.print("]: ");
  for(size_t i=0;i<len;i++){ if(data[i]<0x10) Serial.print('0'); Serial.print(data[i], HEX); Serial.print(' '); }
  Serial.println();
}

void loop(){
  if(!sampleTick){
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    return;
  }
  sampleTick = false;

  uint16_t ch[6];
  // Read analog controls for CH1..CH4 (A0,A1,A3,A2)
  for(int i=0;i<4;i++){
    int v = analogRead(CH_PINS[i]);
    ch[i] = map(v, 0, 1023, 1000, 2000);
  }
  // CH5: arming switch on D2 (digital)
  // Using INPUT_PULLUP: LOW = activated (e.g., GND), HIGH = inactive (pulled-up).
  int sw = digitalRead(SWITCH_PIN);
  ch[4] = (sw == HIGH) ? 2000 : 1000;
  // CH6: potentiometer on A7 (CH_PINS[4]) mapped to 1000..2000
  int v6 = analogRead(CH_PINS[4]);
  ch[5] = map(v6, 0, 1023, 1000, 2000);
  for(int i=0;i<6;i++){ payload[2*i] = ch[i] & 0xFF; payload[2*i+1] = (ch[i]>>8)&0xFF; }

  Serial.print("Channels: ");
  for(int i=0;i<6;i++){ Serial.print("CH"); Serial.print(i+1); Serial.print(":"); Serial.print(ch[i]); if(i<5) Serial.print("  "); }
  Serial.println();
  printRaw(payload, sizeof(payload));

  bool ok = radio.write(payload, sizeof(payload));
  Serial.print("Transmit: "); Serial.println(ok?"OK":"FAILED");
}
