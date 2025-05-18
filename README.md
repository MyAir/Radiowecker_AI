# Web Radio Alarm Clock for ESP32-S3 with 4.3" Touch Display

A feature-rich web radio alarm clock built for the Makerfabs MaTouch ESP32-S3 Parallel TFT with Touch 4.3" module.

## Features

- **Time & Date**: Accurate timekeeping with NTP synchronization
- **Alarms**: Support for multiple alarms with different schedules
- **Web Radio**: Stream internet radio stations
- **Weather**: Current weather and forecast display
- **Air Quality**: SGP30 TVOC/eCO2 and SHT31 temperature/humidity monitoring
- **Touch Interface**: Intuitive LVGL-based touch interface
- **Web Interface**: Configure settings via web browser
- **OTA Updates**: Update firmware over-the-air
- **SD Card Support**: Store configuration and fallback audio

## Hardware Requirements

- Makerfabs MaTouch ESP32-S3 Parallel TFT with Touch 4.3" module
- MicroSD card (formatted as FAT32)
- Power supply (5V, 2A recommended)

## Installation

1. Install [PlatformIO](https://platformio.org/) for VSCode
2. Clone this repository
3. Copy `data/config.json.template` to `data/config.json` and update with your settings
4. Connect your ESP32-S3 board via USB
5. Build and upload the project

## Configuration

Edit `data/config.json` to configure:

- WiFi credentials
- Timezone
- Display settings
- Alarms
- Radio stations
- Weather API key and location
- System settings

## Web Interface

After flashing, connect to the device's WiFi AP or connect it to your local network.
Access the web interface at `http://<device-ip>`

## Libraries Used

- LVGL (Graphics Library)
- ArduinoJson
- ESP8266Audio
- Adafruit SGP30 & SHT31 Libraries
- AsyncTCP & ESPAsyncWebServer

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- LVGL for the amazing graphics library
- PlatformIO for the awesome development platform
- All the library authors and contributors
