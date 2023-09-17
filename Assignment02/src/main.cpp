#include <Arduino.h>
gpio_num_t la = GPIO_NUM_36;


hw_timer_t * timer1 = nullptr;
hw_timer_t * timer2 = nullptr;

uint8_t dhtData[80];  // Store the data


void read_data(){
  ets_delay_us(160);
  Serial.write(digitalRead(la));
  

}

void IRAM_ATTR end_start_signal(){
  
  digitalWrite(la,HIGH);
  pinMode(la,INPUT_PULLUP);
  ets_delay_us(40);
  Serial.write("a");
  //read_data();

}

void IRAM_ATTR interrupt_dht11(){
  for (int i = 0; i <80 ; i++){
    dhtData[i] = 0;
  }
  pinMode(la,OUTPUT);
  // send start signal 
  digitalWrite(la,LOW);
  timerRestart(timer2);
  timerWrite(timer2,0);
  
  
  

}



bool readDHT();

void setup() {
  pinMode(RGB_BUILTIN,OUTPUT);
  Serial.begin(115200);
  
  // configure timer
  timer1 = timerBegin(1, 40000, true);
  timerAttachInterrupt(timer1, &interrupt_dht11, true);
  timerAlarmWrite(timer1, 10000, true); 
  timerAlarmEnable(timer1);   

  timer2 = timerBegin(2, 40000, true);
  timerAttachInterrupt(timer2, end_start_signal, true);
  timerAlarmWrite(timer2, 40,false);
  timerAlarmEnable(timer2);

  /* timer3 = timerBegin(3, 400, true);
  timerAttachInterrupt(timer3, end_start_signal, true);
  timerAlarmWrite(timer3, 40,true);
  timerAlarmEnable(timer3); */
  
  
  

  
 

  
  
  
}

void loop() {
  
  /* if (readDHT()) {
    float humidity = dhtData[0];
    float temperature = dhtData[2];
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");
  }
  delay(2000); */
}
/* 
bool readDHT() {
  uint8_t lastState = HIGH;
  uint8_t counter = 0;
  uint8_t j = 0, i;

  memset(dhtData, 0, 5);

  // Send start signal
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(20);
  digitalWrite(DHT_PIN, HIGH);
  pinMode(DHT_PIN, INPUT);

  // Read the response and data
  for (i = 0; i < 85; i++) {
    counter = 0;
    while (digitalRead(DHT_PIN) == lastState) {
      counter++;
      delayMicroseconds(1);
      if (counter == 255) {
        break;
      }
    }
    lastState = digitalRead(DHT_PIN);
    if (counter == 255) {
      break;
    }
    if (i >= 4 && i % 2 == 0) {
      dhtData[j / 8] <<= 1;
      if (counter > 16) {
        dhtData[j / 8] |= 1;
      }
      j++;
    }
  }
  if (j >= 40 && (dhtData[0] + dhtData[1] + dhtData[2] + dhtData[3] == dhtData[4])) {
    return true;
  } else {
    return false;
  }
} */