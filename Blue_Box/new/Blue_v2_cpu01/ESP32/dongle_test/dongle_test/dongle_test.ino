#include "BluetoothSerial.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

// Check if BluetoothSerial library is compiled in
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Make sure to run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

// Function to convert the Bluetooth address to a string
char* bda2str(const uint8_t* bda, char* str, size_t size) {
  if (bda == NULL || str == NULL || size < 18) {
    return NULL;
  }
  snprintf(str, size, "%02X:%02X:%02X:%02X:%02X:%02X",
           bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
  return str;
}

// Bluetooth inquiry result callback
void bt_inquiry_result_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (event == ESP_BT_GAP_DISC_RES_EVT) {
    // Device discovered
    char bda_str[18];
    bda2str(param->disc_res.bda, bda_str, sizeof(bda_str));  // Convert Bluetooth address to string
    Serial.print("Device found: ");
    Serial.println(bda_str);

    // Optional: Try to get the device name if available
    if (param->disc_res.num_prop > 0) {
      for (int i = 0; i < param->disc_res.num_prop; i++) {
        if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR) {
          char device_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
          memcpy(device_name, param->disc_res.prop[i].val, param->disc_res.prop[i].len);
          device_name[param->disc_res.prop[i].len] = '\0';  // Null-terminate the string
          Serial.print("Device name: ");
          Serial.println(device_name);
        }
      }
    }
  } else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
    if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
      Serial.println("Bluetooth scan stopped.");
    } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
      Serial.println("Bluetooth scan started.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  if (SerialBT.begin("ESP32_Dongle")) {  // Initialize Bluetooth
    Serial.println("Bluetooth initialized as ESP32_Dongle");
  } else {
    Serial.println("Failed to initialize Bluetooth");
    return;
  }

  // Start Bluetooth scan
  esp_bt_gap_register_callback(bt_inquiry_result_callback);
  esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 5, 0);  // Scan for 5 seconds

  delay(12000);

  // Connect to BB_4
    Serial.println("Trying to connect to ESP32_BB_4...");

  uint8_t address[6] = { 0x64, 0xB7, 0x08, 0x4D, 0x34, 0x86 };  
  // Increase timeout for connection
  unsigned long startTime = millis();
  while (!SerialBT.connect(address) && millis() - startTime < 5000) {
    Serial.print(".");  // Indicate progress
    delay(500);  // Adjust delay
  }

  if (SerialBT.connected()) {
    Serial.println("Connected to ESP32_BB_4");
    // find_bluetooth_device();
  } else {
    Serial.println("Failed to connect to ESP32_BB_4");
  }

}

void loop() {
  // Main loop
}
