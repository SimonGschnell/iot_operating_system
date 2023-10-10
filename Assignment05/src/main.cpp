
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "DHT_Async.h"

// BLE device name
const std::string device_name = "BLE DEVICE 005";

// check if device is connected
volatile bool connected = false;

// uuid's for service and characteristics
const std::string service_uuid{"45a47431-c2f0-4db0-a21e-2542ab0f54c5"};
const std::string characteristic_uuid_temperature{"bb4c0f52-b2e8-473e-b972-b01d6524dbae"};
const std::string characterisitc_uuid_humidity{"0e54d08f-eccf-44b8-98ea-d4fc55d0c068"};


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
constexpr uint8_t measurement_flag = (1<<3);

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
    if (millis() - measurement_timestamp > 50ul) {
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
    EventBits_t flag_bits = xEventGroupGetBits(flags);
    
    // dht is only measured if a device is connected
    if(connected){
      
      if ( (flag_bits & measurement_flag) == measurement_flag ) {
        
        if (measure_environment(&temperature, &humidity)) {
          counter++;
          dht_data data = {temperature, humidity, counter};
          xQueueSend(dht_queue, &data, 1/portTICK_PERIOD_MS);
          xEventGroupClearBits(flags, measurement_flag);
      }
      
      }
    }
    
    
    vTaskDelay(1/ portTICK_PERIOD_MS);
  }
  

}







class server_callbacks: public BLEServerCallbacks{
  void onConnect(BLEServer *server){
    connected = true;
  }
  void onDisconnect(BLEServer *server){
    connected = false;
  }
};

void ble_task(void *args){

  BLEDevice::init(device_name);\
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new server_callbacks());
  BLEService* service = server->createService(service_uuid);
  BLECharacteristic* temp_characteristic = service->createCharacteristic(characteristic_uuid_temperature, BLECharacteristic::PROPERTY_NOTIFY|BLECharacteristic::PROPERTY_READ);
  BLECharacteristic* humid_characteristic =service->createCharacteristic(characterisitc_uuid_humidity, BLECharacteristic::PROPERTY_NOTIFY|BLECharacteristic::PROPERTY_READ);
  temp_characteristic->addDescriptor(new BLE2902());
  humid_characteristic->addDescriptor(new BLE2902());

  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(service_uuid);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  BLEDevice::startAdvertising();

  bool last_connected{false};

  dht_data data{};

  float readValueTemperature{};
  float readValueHumidity{};

  while (true)
  {      
    xQueueReceive(dht_queue, &data, 1/portTICK_PERIOD_MS);
      

    if(server && temp_characteristic){
      if(connected){
        
        auto char_data =  temp_characteristic->getValue();
        
        // Assuming that the value is stored as a float in little-endian format
        if (char_data.length() == sizeof(float)) {
    
          memcpy(&readValueTemperature, char_data.data(), sizeof(float));
        } else {
            // Handle the case where the value doesn't have the expected length
        }
        
        
        
      if (data.temperature != readValueTemperature) {
        temp_characteristic->setValue(data.temperature);
        temp_characteristic->notify();
      }
      }
    }

    if(server && humid_characteristic){
      if(connected){
        
        auto char_data =  humid_characteristic->getValue();
        
        // Assuming that the value is stored as a float in little-endian format
        if (char_data.length() == sizeof(float)) {
    
          memcpy(&readValueHumidity, char_data.data(), sizeof(float));
        } else {
            // Handle the case where the value doesn't have the expected length
        }
        
        
        
      if (data.humidity != readValueHumidity) {
        humid_characteristic->setValue(data.humidity);
        humid_characteristic->notify();
      }
      }
    }

    // disconnecting
      if (!connected && last_connected) {
        last_connected = connected;
        delay(500); // give the bluetooth stack the chance to get things ready
        server->startAdvertising(); // restart advertising
      }

      // connecting
      if (connected && !last_connected) {
        last_connected = connected;
      }
    vTaskDelay(1/portTICK_PERIOD_MS);
  }
}


void measurement_timer_callback(void *args){
  xEventGroupSetBits(flags,measurement_flag);
}


void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    flags = xEventGroupCreate();

    dht_queue = xQueueCreate(10, sizeof(dht_data));

    TimerHandle_t measurement_timer = xTimerCreate("measurement timer", 5000/ portTICK_RATE_MS, pdTRUE, nullptr, measurement_timer_callback);
    xTimerStart(measurement_timer,0);
    
    xTaskCreatePinnedToCore(measure_environment_task, "dht task", configMINIMAL_STACK_SIZE + 1024 , NULL, tskIDLE_PRIORITY +1 , NULL,1);
    xTaskCreate(ble_task, "ble task", configMINIMAL_STACK_SIZE + (4* 1024) , NULL, tskIDLE_PRIORITY +3 , NULL);
}


void loop() {
    
    
}
