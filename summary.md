# Web Radio Alarm Clock - Project Summary

## Project Overview
A feature-rich web radio alarm clock built for the Makerfabs MaTouch ESP32-S3 Parallel TFT with Touch 4.3" module. The device combines radio streaming, alarm functionality, and environmental monitoring in a user-friendly interface.

## Hardware Specifications
- **MCU**: ESP32-S3-WROOM-1 (16MB Flash, 8MB PSRAM)
- **Display**: 4.3" IPS TFT (800x480) with capacitive touch
- **Audio**: I2S audio output with hardware volume control
- **Sensors**:
  - SHT31: Temperature & Humidity
  - SGP30: TVOC & eCO2 Air Quality
- **Storage**: MicroSD card slot
- **Connectivity**: WiFi 4, Bluetooth 5.0, USB Type-C

## Key Features

### Audio Playback
- Web radio streaming (MP3/AAC)
- Local MP3 file playback
- Volume control (0-100%)
- Audio buffering for smooth playback

### Alarm System
- Multiple configurable alarms
- Repeat schedules (daily, weekdays, weekends, custom)
- Snooze functionality
- Configurable alarm sources (radio or MP3)
- Volume ramping for gentle wake-up

### User Interface
- Touch-optimized LVGL interface
- Home screen with:
  - Large time display
  - Current date
  - Weather information
  - Next alarm
  - Environmental data
- Alarm management interface
- Radio station selection
- Settings menu

### Environmental Monitoring
- Temperature and humidity display
- Air quality (TVOC and eCO2) monitoring
- Real-time updates

### Connectivity
- WiFi configuration
- NTP time synchronization
- Web-based configuration interface
- OTA (Over-The-Air) updates

## Software Architecture

### Core Components
1. **AudioManager**: Handles all audio playback
2. **AlarmManager**: Manages alarm scheduling and triggering
3. **ConfigManager**: Handles configuration storage/retrieval
4. **UIManager**: Manages the LVGL-based user interface

### Task Structure
1. **LVGL Task** (Core 1): UI rendering
2. **Display Task** (Core 0): Updates time and UI elements
3. **Sensors Task** (Core 0): Reads environmental sensors
4. **Audio Task** (Core 0, High Priority): Handles audio processing

## Dependencies
- LVGL (Light and Versatile Graphics Library)
- ArduinoJson
- ESP8266Audio (modified for ESP32-S3)
- Adafruit SGP30 Library
- Adafruit SHT31 Library
- AsyncTCP
- ESPAsyncWebServer
- Arduino_GFX_Library

## Configuration
Configuration is stored in JSON format on the SD card, including:
- WiFi credentials
- Alarm settings
- Radio station presets
- Display preferences
- System settings

## Pin Configuration
- **Display**: Parallel RGB interface
- **Touch**: I2C (SDA: 17, SCL: 18, INT: 16, RST: 15)
- **Audio**: I2S (BCLK: 12, LRC: 13, DOUT: 10)
- **Sensors**: I2C (SDA: 38, SCL: 37)
- **SD Card**: SD_MMC (CMD: 13, CLK: 12, D0: 11)
- **Backlight**: PWM on GPIO 44

## Build Instructions
1. Install PlatformIO
2. Clone the repository
3. Copy `data/config.example.json` to `data/config.json`
4. Configure your settings in `config.json`
5. Upload files to SPIFFS: `pio run -t uploadfs`
6. Build and upload: `pio run -t upload`

## Usage
1. Power on the device
2. Connect to the WiFi access point
3. Configure WiFi and other settings via the web interface
4. Set up alarms and radio presets
5. Enjoy your web radio alarm clock!

## Future Enhancements
- Add more radio streaming protocols
- Implement sleep timer
- Add alarm fade-in effect
- Support for more audio formats
- Weather forecast integration
- Voice control

## License
[Specify your preferred open-source license here]
