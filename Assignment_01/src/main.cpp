#include <Arduino.h>

struct Color{
int red;
int green;
int blue;
};

//next color for RGB_BUILTIN function
void next(Color *);
//global variables
Color led = {RGB_BRIGHTNESS,0,0};
Color * led_ptr = &led;

void setup() {
  pinMode(RGB_BUILTIN, OUTPUT);
  pinMode(35, INPUT);
  Serial.begin(115200);
  delay(500);
  neopixelWrite(RGB_BUILTIN, led_ptr->red, led_ptr->green, led_ptr->blue); 
}

int currentState;
int lastState= HIGH;
int alternativeState = HIGH;
int captureTime;


void loop() {
  //The ESP32-S3 has a 45k ohm internal pull-up/pull-down resistor at GPIO0 
  //https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/boot-mode-selection.html
  currentState =digitalRead(35);

  if (lastState == LOW && currentState == HIGH){
    //added little delay for debounce of button press
    delay(100);
    Serial.println("io23m005");
  }
  lastState = currentState;

  
  if(Serial.available()>0){
    String input = Serial.readStringUntil('\n');
    //both n and N switch the RGB_BUILTIN color
    if (input == "n" || input == "N" ) {
      next(led_ptr);
      neopixelWrite(RGB_BUILTIN, led_ptr->red, led_ptr->green, led_ptr->blue);
    }else{
      Serial.println(input);
    }
    
  }
  
}

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