#include <Arduino.h>

#define DHT_PIN 36 //DATA PIN 


u_int8_t luftfeuchtigkeit;
u_int8_t temperatur;
u_int8_t checksum;


hw_timer_t * timer1 = nullptr;
volatile bool has_interrupt = false; 
u_int8_t data[5];  // array 5x8, 40 bits of data


void IRAM_ATTR start_interrupt(){
  has_interrupt = true;
}

void send_start_signal()
{
  digitalWrite(36, LOW);  
  delay(20);              
  digitalWrite(36, HIGH); 
  pinMode(36, INPUT);     
}

void read_data()
{
  while (digitalRead(36) == HIGH); // start of communication
  delayMicroseconds(160); // wait until response is received
  for (int i = 0; i < 5; i++){
    int j = 0;
    u_int8_t value = 0;
 
    for (j = 0; j < 8; j++) {
      while (digitalRead(36) == LOW);
      // if pin stays high longer than 26-28ms than the bit is 1 
      delayMicroseconds(30);          
      if (digitalRead(36) == HIGH){
        // place all 1 bits in value, in order
        value |= (1 << (8 - j)); 
      }
      // bit operation resource: 
      // http://www.convertalot.com/bitwise_operators.html
      // https://www.arduino.cc/reference/tr/language/structure/bitwise-operators/bitshiftleft/
      // https://www.arduino.cc/reference/en/language/structure/compound-operators/compoundbitwiseor/
            
      // wait for next bit 
      while (digitalRead(36) == HIGH); 
    }
    data[i] = value;
    switch (i){
      case 0: luftfeuchtigkeit = data[i]; break;
      case 2: temperatur = data[i]; break;
      case 4: checksum = data[i]; break;
    }

  } 
    
  pinMode(36, OUTPUT);    
  digitalWrite(36, HIGH); 
}




void setup() {
  Serial.begin(115200);
  
  pinMode(36, OUTPUT);
  digitalWrite(36,HIGH);
  while(!Serial); 
  timer1 = timerBegin(1, 40000, true);
  timerAttachInterrupt(timer1, &start_interrupt, true);
  timerAlarmWrite(timer1, 10000, true); 
  timerAlarmEnable(timer1);   
}

void loop() {
  noInterrupts();
  bool local_interrupt = has_interrupt;
  has_interrupt = false;
  interrupts();

  if (local_interrupt){
    send_start_signal();
    read_data();

    Serial.print("Luftfeuchtigkeit = ");
    Serial.print(luftfeuchtigkeit);
    Serial.print("% - ");

    Serial.print("Temperatur = ");
    Serial.print(temperatur);
    Serial.println("°C");

  }

  
}