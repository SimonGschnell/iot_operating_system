#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHT_PIN 36 //DATA PIN 
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);
hw_timer_t * timer1 = nullptr;
volatile bool has_interrupt = false; 

void IRAM_ATTR start_interrupt(){
  has_interrupt = true;
}

void print_dht_data(){
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C\t");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  timer1 = timerBegin(1, 40000, true);
  timerAttachInterrupt(timer1, &start_interrupt, true);
  timerAlarmWrite(timer1, 10000, true); 
  timerAlarmEnable(timer1);   
  while(!Serial);
}

void loop() {
  noInterrupts();
  bool local_interrupt = has_interrupt;
  has_interrupt = false;
  interrupts();

  if (local_interrupt){
    print_dht_data();
  }

  
}