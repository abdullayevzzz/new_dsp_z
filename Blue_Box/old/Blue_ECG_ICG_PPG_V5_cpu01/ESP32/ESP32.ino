#include "BluetoothSerial.h"

#define packet_length 25
#define RX_PIN 16  // Example RX pin
#define TX_PIN 17  // TX pin


// Check if BluetoothSerial library is compiled in
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Make sure to run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

void setup() {
  // Serial.begin(460800, SERIAL_8N1); // Start debugging serial communication at 115200 baud rate
  Serial1.begin(460800, SERIAL_8N1, RX_PIN, TX_PIN); // Setup for UART communication
  SerialBT.begin("ESP32_BB_1"); // Start Bluetooth with the name "ESP32_Sawtooth"
  delay(2000); // Optional: give some time to connect
}

void loop() {
  static uint8_t packet[packet_length];
  static int index = 0;
  static bool isCollecting = false;
  static bool isAuthentificated = false;
  static uint8_t byte_from_dsp;
  static uint8_t byte_from_pc;

  while (Serial1.available()) {
    byte_from_dsp = Serial1.read();

    // Check for start of packet
    if (index == 0 && byte_from_dsp == 0x7F) {
      packet[index++] = byte_from_dsp; // Store the first byte and increment index
      isCollecting = true; // Start collecting the packet
    } else if (index == 1 && byte_from_dsp == 0xFF && isCollecting) {
      packet[index++] = byte_from_dsp; // Store the second byte and increment index
    } else if (index >= 2 && index < packet_length && isCollecting) {
      packet[index++] = byte_from_dsp; // Continue collecting the packet
    }

    // Check if packet is complete
    if (index == packet_length) {
      // Send the complete packet over Bluetooth
      SerialBT.write(packet, sizeof(packet));
      // Reset for the next packet
      index = 0;
      isCollecting = false;
    }
  }

  while (SerialBT.available()) {
    byte_from_pc = SerialBT.read();

    // Autentification for port finding
    if (byte_from_pc == 0x01){
      SerialBT.flush();
      delay(500);
      SerialBT.write(0x02);
      }

    // Redirect the command
    else
      Serial1.write(byte_from_pc);
  }
}