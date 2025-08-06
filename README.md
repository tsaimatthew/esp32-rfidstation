### ESP32 RFID Station (work in progress)

This ESP32 image is designed have basic BLE and WiFi capabilities. The end goal is to assist with home automation and environment monitoring. Interfacing with the [PN532 RFID module from HiLetGo](https://www.amazon.com/gp/product/B01I1J17LC/) and the [BME280 Environment Monitor from HiLetGo](https://www.amazon.com/gp/product/B01N47LZ4P) over SPI, it communicates real time data over BLE and/or WiFi to trigger webhooks or mobile alerts. It allows for configuration via a mobile app over BLE and supports OTA updates.

## Networking Processes
The networking tasks (WiFi, BLE, and OTA) are tied to core 0. If enabled in settings, updates will be checked for once per day from the GitHub releases: latest tag. BLE will allow users to modify the system, including the broadcasting name and enable/disable sensors, OTA updates, and WiFi. All data will be sent over MQTT to a centralized website where data can be monitored remotely. Users can configure the actions to run based on the RFID tag and see environmental data in graphs.

## Important Configuration Options
- lwip -> Local netif hostname: WiFi broadcasting name
- ESP32 Networking Defaults;
  - Enable WiFi for webhook/OTA usage
  - WiFi SSID and Password for default WiFi login. This can be configured via the app. 
  - Maximum WiFi retry 
- ESP32 BLE Defaults: Default Bluetooth name
- The MQTT QoS is set to 0 and will send data every 10 seconds.

## Components
### BLE
This provides the BLE-related functions, including broadcasting and connecting to device. \
To Do:
- Add security features such as push to connect. 
- Provide write hooks to update device state
### Common
This provides common includes and global variables which are loaded from NVS into RAM upon device initialization
### OTA
This provides the WiFi and OTA related functions. It contains the WiFi and SNTP intialization functions as well as the OTA related task.
### Sensors
This provides the functions to read the sensors via SPI. It also contains the MQTT functions responsible for sending the bundled MQTT periodically.