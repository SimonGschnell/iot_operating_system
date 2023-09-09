#include <Arduino.h>

// put function declarations here:
void led_blinking(int, int);



struct Color{
int red;
int green;
int blue;
};

void next(Color *c){
   if(c->red == RGB_BRIGHTNESS){
   c->red = 0;
   c->green = RGB_BRIGHTNESS;
   }else if (c->green == RGB_BRIGHTNESS){
   c->green = 0;
   c->blue = RGB_BRIGHTNESS;
   }else if (c->blue == RGB_BRIGHTNESS){
    c->blue = 0;
    c->red = RGB_BRIGHTNESS;
   }
}

Color led = {RGB_BRIGHTNESS,0,0};

void setup() {
  pinMode(RGB_BUILTIN, OUTPUT);
  pinMode(35, INPUT);
  Serial.begin(115200);
  delay(500);
  Color * led_ptr = &led;
  neopixelWrite(RGB_BUILTIN, led_ptr->red, led_ptr->green, led_ptr->blue);
  
  
   
}

int currentState;
int lastState= HIGH;
int stateChange =0;
int startTime= 0;
void loop() {
  /*
    At start, the RGB-LED is red. When receiving ”n” via the serial port, switch the LED to green. When receiving
    “n” again, switch it to blue. Receiving another “n” will start the cycle from the beginning (red > green > blue >
    red > green > …).
  */
  Color * led_ptr = &led;
  currentState =digitalRead(35);
  
  if (currentState != stateChange){
    startTime = millis();
    stateChange = currentState;
  }

  if (millis() - startTime > 50){
  if(lastState == HIGH && currentState == HIGH){
    Serial.println("The state changed from LOW to HIGH");
}
  // save the last state
  lastState = currentState;
  }
  /* if (currentState != LOW){
    startTime = millis();
  }

  if(millis() - startTime > 50){
    
    if (lastState == HIGH && currentState == LOW){
    Serial.println("io23m005");
    
  }
    lastState=currentState;
  } */
  
  
  
  
  
  if(Serial.available()>0){
    
    
    String input = Serial.readStringUntil('\r\n');
    if (input == "n") {
      next(led_ptr);
      
      neopixelWrite(RGB_BUILTIN, led_ptr->red, led_ptr->green, led_ptr->blue);
    }else{
      Serial.println(input);
    }
    
    
  }
  
}

void led_blinking(int LED_PIN, int DELAY) {
  digitalWrite(LED_PIN, HIGH);
  delay(DELAY);
  digitalWrite(LED_PIN, LOW);
  delay(DELAY);
}
