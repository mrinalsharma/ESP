/**
 * Demo for ESP32s-CAM - Flase ON/OFF
 * 
 * Toggles the LED on and off in its own task/thread.
 * From the arduino serial monitor, you can send command ON/OFF to turn the flash ON or OFF
 */

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Pins
static const int led_pin = 4;

//Queue Length
static const uint8_t msg_queue_len = 1;
static QueueHandle_t msg_queue_handle;

void toggleLED(void *parameter) {
  int ulValReceived;
  while (1) {
    if (xQueueReceive(msg_queue_handle, &ulValReceived, 0) == pdTRUE) {
      if (ulValReceived == 1) {
        digitalWrite(led_pin, HIGH);
        //vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.println("Turning LED ON");
      }
      if (ulValReceived == 0) {
        digitalWrite(led_pin, LOW);
        //vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.println("Turning LED OFF");
      }

    } else {
      //Serial.println("Queue is Empty.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  // Configure pin
  pinMode(led_pin, OUTPUT);
  msg_queue_handle = xQueueCreate(msg_queue_len, sizeof(int));
  // Task to run forever
  xTaskCreatePinnedToCore(toggleLED,"Toggle LED",1024, NULL, 1, NULL, app_cpu);              
}

void loop() {
  if (Serial.available() > 0) {
    int daviceState = 0;
    String command = Serial.readString();
    command.trim();
    command.toLowerCase();
    Serial.printf("Received command %s", command);
    if (command == "on") {
      daviceState = 1;
      if (xQueueSend(msg_queue_handle, (void *)&daviceState, (TickType_t)10) != pdTRUE) {
        Serial.println("Failed to post ON\n");
      }
    }
    if (command == "off") {
      daviceState = 0;
      if (xQueueSend(msg_queue_handle, (void *)&daviceState, (TickType_t)10) != pdTRUE) {
        Serial.println("Failed to post OFF\n");
      }
    }
  }
}