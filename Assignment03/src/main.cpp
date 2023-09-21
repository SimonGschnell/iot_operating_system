#include <Arduino.h>

 QueueHandle_t RGB_queue;
 QueueHandle_t BUTTON_COUNTER_queue;

void task1(void *args){
  int button_counter{};
  for(;;) {
      
     
      if(Serial.available()>0){
        String input = Serial.readStringUntil('\n');
      
        if( input == "r" || input == "g" ||  input == "b" ){
          char color = input.charAt(0);
          auto status = xQueueSend(RGB_queue,&color,1/portTICK_PERIOD_MS);
          if (status == pdFAIL){
            Serial.println("ERROR WHILE SENDING LED COLOR");
          }
        }else{
          Serial.println(input);
        }
        
      }else{
        //check every ms if button was pressed without blocking the rest
         if (xQueueReceive(BUTTON_COUNTER_queue,&button_counter,1/portTICK_PERIOD_MS ) == pdPASS){
              Serial.print("Hello, the button was clicked ");
              Serial.print(button_counter);
              Serial.println(" times");
            } 
      }
  }

}

void task2(void *args){
  char c{};
  // at start the LED is red
  neopixelWrite(LED_BUILTIN,RGB_BRIGHTNESS,0,0);
  for(;;) {
      //waits until something was received from queue
      if (xQueueReceive(RGB_queue,&c,portMAX_DELAY) == pdPASS){
        switch (c)
        {
        case 'r': neopixelWrite(LED_BUILTIN,RGB_BRIGHTNESS,0,0);break;
        case 'g': neopixelWrite(LED_BUILTIN,0,RGB_BRIGHTNESS,0);break;
        case 'b': neopixelWrite(LED_BUILTIN,0,0,RGB_BRIGHTNESS);break;
        default: neopixelWrite(LED_BUILTIN,RGB_BRIGHTNESS,0,0);break;
        }
      }
  }
}

void task3(void *args){
  int counter{};
  pinMode(0, INPUT);
  u8_t currentValue = HIGH;
  u8_t oldValue = HIGH;
  for(;;) {
      currentValue = digitalRead(0);
      if (oldValue == HIGH && currentValue == LOW){
        counter++;
        auto status = xQueueSend(BUTTON_COUNTER_queue, &counter, portMAX_DELAY);
        if (status == pdFAIL){
          Serial.println("ERROR WHILE SENDING BUTTON COUNTER");
        }
      }
      oldValue = currentValue;
  }
}

void setup() {
  pinMode(36,OUTPUT);
  Serial.begin(115200);
  while(!Serial);

  //creates RGB_queue
  RGB_queue = xQueueCreate(10, sizeof(char));

  //creates BUTTON_COUNTER_queue
  BUTTON_COUNTER_queue = xQueueCreate(10, sizeof(int));
  
  TaskHandle_t* task1_h{};
  //has the same priority as idle task because otherwise it would starve the idle tasks
  xTaskCreate(task1, "task1", CONFIG_MB_SERIAL_TASK_STACK_SIZE, NULL,tskIDLE_PRIORITY+1,NULL);
  xTaskCreate(task2, "task2", CONFIG_MB_SERIAL_TASK_STACK_SIZE, NULL,tskIDLE_PRIORITY,NULL);
  xTaskCreate(task3, "task3", CONFIG_MB_SERIAL_TASK_STACK_SIZE, NULL,tskIDLE_PRIORITY,NULL);
  //vTaskStartScheduler(); not needed because esp32 does it automatically
  //! for other microcontrollers it is mandatory
}

void loop() {

}

