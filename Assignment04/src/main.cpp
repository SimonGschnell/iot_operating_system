#include <Arduino.h>


#include "DHT_Async.h"

/* Uncomment according to your sensortype. */
#define DHT_SENSOR_TYPE DHT_TYPE_11
#define HEARTBEAT 37
#define FAST_HEARTBEAT 38

static const int DHT_SENSOR_PIN = 36;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

//queue for dht data
QueueHandle_t dht_queue{};

//flags
EventGroupHandle_t flags{};

//flag bits
constexpr uint8_t button_flag = (1<<0);
constexpr uint8_t heartbeat_flag = (1<<1);
constexpr uint8_t dht_flag = (1<<2);

struct dht_data {
  float temperature;
  float humidity;
  int counter;
};

/*
 * Poll for a measurement, keeping the state machine alive.  Returns
 * true if a measurement is available.
 */
static bool measure_environment(float *temperature, float *humidity) {
    static unsigned long measurement_timestamp = millis();

    /* Measure once every four seconds. */
    if (millis() - measurement_timestamp > 3000ul) {
        if (dht_sensor.measure(temperature, humidity)) {
            measurement_timestamp = millis();
            return (true);
        }
    }

    return (false);
}



void measure_environment_task(void *args){
float temperature;
    float humidity;
    int counter = 0;
  while (true)
  {
     /* Measure temperature and humidity.  If the functions returns
       true, then a measurement is available. */
    if (measure_environment(&temperature, &humidity)) {
      counter++;
        dht_data data = {temperature, humidity, counter};
        xQueueSend(dht_queue, &data, 1/portTICK_PERIOD_MS);
        xEventGroupSetBits(flags, dht_flag);
    }
    vTaskDelay(1/ portTICK_PERIOD_MS);
  }
  

}

void serial_task(void *args){
  dht_data data{};
  while (true)
  {
    uint8_t available_flags = xEventGroupWaitBits(flags, (button_flag | dht_flag), pdTRUE, pdFALSE, portMAX_DELAY);
    if((available_flags & button_flag) == button_flag){
      Serial.println("io23m005");
      
    }else if ( (available_flags & dht_flag) == dht_flag){
      if ( xQueueReceive(dht_queue, &data, 1/ portTICK_PERIOD_MS) == pdPASS){
      Serial.print(data.counter);
      Serial.print(": ");
      Serial.print("T = ");
      Serial.print(data.temperature, 1);
      Serial.print(" deg. C, H = ");
      Serial.print(data.humidity, 1);
      Serial.println("%");
    }
    }
    
  }
  

}


void button_task(void *args){
  pinMode(0, INPUT_PULLUP);
  int current = HIGH;
  int old = HIGH;
  while (true)
  {
    current = digitalRead(0);
    if( current == LOW && old == HIGH ){
      xEventGroupSetBits(flags,button_flag);
    }
    old = current;
    vTaskDelay(1/ portTICK_PERIOD_MS);
  }
  

}

//checks the heartbeat flag
void heartbeat_task(void *args){
  
  while (true)
  {
    uint8_t bits = xEventGroupWaitBits(flags, heartbeat_flag, pdTRUE, pdFALSE, portMAX_DELAY);
    if ((bits & heartbeat_flag) == heartbeat_flag){
      digitalWrite(HEARTBEAT, !digitalRead(HEARTBEAT));
    }
  }
  

}
//high priority heartbeat with isr
void IRAM_ATTR fast_heartbeat(void *args){
  digitalWrite(FAST_HEARTBEAT, !digitalRead(FAST_HEARTBEAT));
}

//sets the heartbeat flag every 200ms
void heartbeat(void *args){
  xEventGroupSetBits(flags, heartbeat_flag);
}


void setup() {
    Serial.begin(115200);
    while (!Serial);

    pinMode(HEARTBEAT, OUTPUT);
    pinMode(FAST_HEARTBEAT, OUTPUT);

    //software timer for fast heartbeat with high priority short ISR
    TimerHandle_t fast_heartbeat_timer = xTimerCreate("fastheartbeat", 1/ portTICK_RATE_MS, pdTRUE, nullptr, fast_heartbeat);
    xTimerStart(fast_heartbeat_timer,0);

    //software timer to set flag for heartbeat
    TimerHandle_t heartbeat_timer = xTimerCreate("heartbeat", 200/ portTICK_RATE_MS, pdTRUE, nullptr, heartbeat);
    xTimerStart(heartbeat_timer,0);

    flags = xEventGroupCreate();

    dht_queue = xQueueCreate(10, sizeof(dht_data));
    
    xTaskCreatePinnedToCore(measure_environment_task, "dht task", configMINIMAL_STACK_SIZE + 1024 , NULL, tskIDLE_PRIORITY +1 , NULL,1);
    xTaskCreatePinnedToCore(serial_task, "dht task", configMINIMAL_STACK_SIZE + 1024 , NULL, tskIDLE_PRIORITY +1 , NULL,1);
    xTaskCreatePinnedToCore(button_task, "button task", configMINIMAL_STACK_SIZE + 1024 , NULL, tskIDLE_PRIORITY +1 , NULL,1);
    xTaskCreatePinnedToCore(heartbeat_task, "heartbeat task", configMINIMAL_STACK_SIZE + 1024 , NULL, tskIDLE_PRIORITY +1 , NULL,1);
}


void loop() {
    
    
}
