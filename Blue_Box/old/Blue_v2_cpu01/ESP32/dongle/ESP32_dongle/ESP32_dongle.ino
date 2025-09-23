#include "BluetoothSerial.h"

// Check if BluetoothSerial library is compiled in
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Make sure to run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  // Connect to the first ESP32 device with the name "ESP32_BB_4"
  if (SerialBT.begin("ESP32_Dongle")) {  // Initialize Bluetooth as a client
    Serial.println("Bluetooth started successfully.");
  } else {
    Serial.println("Bluetooth failed to start.");
  }

  // Connect to the Bluetooth server (ESP32_BB_4)
  while (!SerialBT.connect("ESP32_BB_4")) {
    Serial.println("Connecting to ESP32_BB_4");
    delay(1000);
  }
  
  verify_bluetooth_device();

  /*
  else {
    Serial.println("Failed to connect to ESP32_BB_4.");
    while (true); // Stay here if connection failed
  }
  */
}

void loop() {
  // Send data to the first ESP32
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }

  // Receive data from the first ESP32
  if (SerialBT.available()) {
    char incomingChar = SerialBT.read();
    Serial.print("Received from ESP32_BB_4: ");
    Serial.println(incomingChar);
  }
}


void verify_bluetooth_device() {
  Serial.println("Attempting to authenticate...");

  // Send the '0x01' byte to initiate authentication
  SerialBT.write(0x01);

  // Wait for the response
  unsigned long startTime = millis();
  while (millis() - startTime < 1000) {  // 1 second timeout
    if (SerialBT.available()) {
      char response = SerialBT.read();

      // Check if the response is '0x02'
      if (response == 0x02) {
        Serial.println("Authentication successful! Device found.");
        return;
      }
    }
  }
  Serial.println("Authentication failed or timed out.");
}