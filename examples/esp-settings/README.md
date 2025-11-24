## Wi-Fi Access Point Example

This example demonstrates how to render device settings structure in web browser. 
When the example runs, the ESP32 or ESP8266 initializes its Wi-Fi interface in AP mode and starts a wireless network.

### Access Point Details

- SSID (Network Name):
  - `ESP32-AP` (for ESP32 devices)
  - `ESP8266-AP` (for ESP8266 devices)

- Default IP Address:  
  `192.168.4.1`

### How to Test the Example

1. Build and flash the project to your ESP device.
2. Open Wi-Fi settings on your phone or computer.
3. Find and connect to the access point:
   - `ESP32-AP` or
   - `ESP8266-AP`
4. After connecting, open a web browser.
5. Enter the following address in the URL bar:  
   `http://192.168.4.1`
6. You should now see the example’s web interface or configuration page.
