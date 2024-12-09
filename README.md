# _WiFi Scanning Project_

This project demonstrates how to scan for WiFi networks using the ESP32-S3. It saves the scanned networks to NVS (Non-Volatile Storage) and loads them on startup.

## Running the example

1. Set up the ESP-IDF environment by following the [ESP-IDF getting started guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html).
2. Clone this project and navigate to its directory.
3. Run `idf.py set-target esp32s3` to set the target to ESP32-S3.
4. Run `idf.py menuconfig` to configure the project.
5. Run `idf.py build` to build the project.
6. Run `idf.py flash` to flash the project to your ESP32-S3 board.
7. Run `idf.py monitor` to view the output from the ESP32-S3.

The device will scan for WiFi networks every 5 seconds and save any new networks to NVS. The saved networks will be loaded and displayed on startup.

