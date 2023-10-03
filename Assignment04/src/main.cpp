#include <Arduino.h>

const int DHT11 = 36;
const int TIMER_DIV_US = 80;
const int UART_BUF_MAX_LEN = 1024;

char uart_buf[UART_BUF_MAX_LEN];

typedef enum {START, ZERO, ONE} bit_t;

QueueHandle_t sensor_data_queue;

struct dht_data_type {
  float temperature;
  float humidity;
};

EventGroupHandle_t flags;
const uint8_t button_press = (1 << 0);
const uint8_t sensor_data = (1 << 1);
const uint8_t dht_flag = (1 << 2);

hw_timer_t *timer0_cfg = nullptr;
volatile bool started = false;
volatile bool dht11_finished = false;
volatile uint8_t dht11_idx = 0;
volatile uint8_t dht11_humidity1 = 0;
volatile uint8_t dht11_humidity2 = 0;
volatile uint8_t dht11_temperature1 = 0;
volatile uint8_t dht11_temperature2 = 0;

void handle_dht11(uint32_t value);
void handle_error();

void IRAM_ATTR dht11_pin_isr() {
  xEventGroupSetBits(flags, dht_flag);
}

void IRAM_ATTR timer0_isr() {
  pinMode(DHT11, INPUT_PULLUP);
  timerAlarmDisable(timer0_cfg);
  started = true;
}

void start_dht11() {
  noInterrupts();
  dht11_finished = false;
  started = false;
  interrupts();

  pinMode(DHT11, OUTPUT);
  digitalWrite(DHT11, LOW);
  timerWrite(timer0_cfg, 0);
  timerAlarmWrite(timer0_cfg, 18000, true);
  timerAlarmEnable(timer0_cfg);
 

  bool local_finished;
  uint8_t local_idx, local_humidity1, local_humidity2, local_temperature1, local_temperature2;
  do {
    delay(100);
    noInterrupts();
    local_finished = dht11_finished;
    local_idx = dht11_idx;
    local_humidity1 = dht11_humidity1;
    local_humidity2 = dht11_humidity2;
    local_temperature1 = dht11_temperature1;
    local_temperature2 = dht11_temperature2;
     
    interrupts();
  } while(!local_finished);

  
  snprintf(uart_buf, UART_BUF_MAX_LEN, "%3u: %u.%01u%% %u.%01uC", local_idx, local_humidity1, local_humidity2, local_temperature1, local_temperature2);
  xQueueSend(sensor_data_queue, uart_buf, 1/ portTICK_PERIOD_MS);
  
  
}

void serial_task(void *args){
  char buffer[]{};
  while(true){
    
    if (xQueueReceive(sensor_data_queue, buffer, 1/ portTICK_PERIOD_MS) == pdPASS){
      Serial.println(buffer);
    }
    EventBits_t active_flags = xEventGroupWaitBits(flags, button_press , pdTRUE,pdFALSE, portMAX_DELAY);
    if ( active_flags & button_press == button_press){
      Serial.println("io23m005");
    }
    /* if( active_flags & sensor_data == sensor_data){
      if ( xQueueReceive(sensor_data_queue, buffer, 1/ portTICK_PERIOD_MS) == pdPASS){
      Serial.println(buffer);
    } 
    }*/
    vTaskDelay(1/ portTICK_PERIOD_MS);
  }

}

void dht_task(void *args){

  while(true){
    vTaskDelay(3000/ portTICK_PERIOD_MS);
    start_dht11();
    
  }

}

void dht_handle(void *args){

  while(true){
    EventBits_t flags_bits = xEventGroupWaitBits(flags, dht_flag, pdTRUE, pdFALSE,portMAX_DELAY);
    if( flags_bits & dht_flag  == dht_flag){
      Serial.println("entered handle");
    handle_dht11(timerRead(timer0_cfg));
    }
    }
  

}

void IRAM_ATTR buttonISR(){
  xEventGroupSetBits(flags, button_press );
}

/* void button_task(void *args){
  //attachInterrupt(0,buttonISR)
  while(true){

    current = digitalRead(0);
    if (current == LOW && old == HIGH){
      xEventGroupSetBits(flags, button_press );
    }
    old = current;
    
    vTaskDelay(100/ portTICK_PERIOD_MS);
    
  }

} */



void setup() {

  pinMode(DHT11, INPUT_PULLUP);
  attachInterrupt(DHT11, dht11_pin_isr, CHANGE);
  neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0);
  timer0_cfg = timerBegin(0, TIMER_DIV_US, true);  
  timerAttachInterrupt(timer0_cfg, &timer0_isr, true);

  sensor_data_queue = xQueueCreate(10, 1024);

  flags = xEventGroupCreate();

  xTaskCreatePinnedToCore(dht_task, "dht11 task", configMINIMAL_STACK_SIZE + (2*1024), NULL, tskIDLE_PRIORITY +1, NULL, 1);

  xTaskCreatePinnedToCore(dht_handle, "dht11 handle", configMINIMAL_STACK_SIZE + (1*1024), NULL, tskIDLE_PRIORITY +1, NULL, 1);

  xTaskCreatePinnedToCore(serial_task, "serial task", configMINIMAL_STACK_SIZE + (2*1024), NULL, tskIDLE_PRIORITY +1, NULL, 1);

  //xTaskCreatePinnedToCore(button_task, "button task", configMINIMAL_STACK_SIZE + (2*1024), NULL, tskIDLE_PRIORITY +1, NULL, 1);

  
  Serial.begin(115200);
  while (!Serial);
}

void loop() {
  
  
}

void handle_error() {
  noInterrupts();
  started = false;
  interrupts();
  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0);
}

void handle_dht11(uint32_t value) {
  static bool first = true;
  static uint32_t last = 0;
  static uint32_t low_portion = 0;
  static uint8_t val = 0;
  static uint32_t count = 0;
  
  if (started) {
    if (first) {
      first = false;
      last = value;
    } else {
      uint32_t duration;
      duration = value - last;
      if (duration == 0)
        handle_error();

      last = value;
      if (low_portion == 0)
        low_portion = duration;
      else {
        bit_t bit;

        if (low_portion > 70 && low_portion < 90 && duration > 70 && duration < 90)
          bit = START;
        else if (low_portion > 40 && low_portion < 60 && duration > 20 && duration < 35)
          bit = ZERO;
        else if (low_portion > 40 && low_portion < 60 && duration > 60 && duration < 80)
          bit = ONE;
        else
          handle_error();
        low_portion = 0;

        switch(bit) {
          case START:
            val = 0;
            count = 0;
            break;
          case ZERO:
            val <<= 1;
            count ++;
            break;
          case ONE:
            val <<= 1;
            val |= 1;
            count ++;
            break;
        }

        if (count == 8) {
          dht11_humidity1 = val;
          val = 0;
        } else if (count == 16) {
          dht11_humidity2 = val;
          val = 0;
        } else if (count == 24) {
          dht11_temperature1 = val;
          val = 0;
        } else if (count == 32) {
          dht11_temperature2 = val;
          val = 0;
        } else if (count == 40) {
          if (dht11_humidity1 + dht11_humidity2 + dht11_temperature1 + dht11_temperature2 != val)
            handle_error();
          else {
            started = false;
            first = true;
            dht11_idx ++;
            dht11_finished = true;
          }
        }
      }
    }
  } 
}
