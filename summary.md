# Web Radio Alarm Clock - Project Summary

## Project Overview
A feature-rich web radio alarm clock built for the Makerfabs MaTouch ESP32-S3 Parallel TFT with Touch 4.3" module. The device combines radio streaming, alarm functionality, and environmental monitoring in a user-friendly interface.

## Latest Updates (May 2024)
- Implemented complete LVGL-based user interface
- Added display initialization with TFT_eSPI and LVGL integration
- Created multiple screens: Home, Radio, Alarm Settings, and System Settings
- Added touch input handling
- Implemented time and date display with NTP synchronization
- Added environmental data visualization
- Configured OTA updates for easy firmware deployment

## Hardware Specifications
- **MCU**: ESP32-S3-WROOM-1 (16MB Flash, 8MB PSRAM)
- **Display**: 4.3" IPS TFT (800x480) with capacitive touch (GT911 controller)
- **Audio**: I2S audio output with hardware volume control
- **Sensors**:
  - SHT31: Temperature & Humidity
  - SGP30: TVOC & eCO2 Air Quality
  - Light Sensor (via Mabee interface)
- **Storage**: MicroSD card slot
- **Connectivity**: 
  - WiFi 4 (2.4GHz)
  - Bluetooth 5.0
  - Dual USB Type-C (UART and native USB)

## Key Features

### Audio Playback
- Web radio streaming (MP3/AAC)
- Local MP3 file playback from SD card
- Hardware volume control with software override
- Audio buffering for smooth playback
- Equalizer presets

### Alarm System
- Multiple configurable alarms
- Repeat schedules (daily, weekdays, weekends, custom)
- Snooze functionality with configurable duration
- Configurable alarm sources (radio, MP3, or buzzer)
- Volume ramping for gentle wake-up
- Sunrise simulation with display backlight control

### User Interface
- Touch-optimized LVGL 8.x interface
- Responsive design for 800x480 resolution
- Home screen with:
  - Large time display with smooth animations
  - Current date and day of week
  - Weather information with icons
  - Next alarm indicator
  - Environmental data (temperature, humidity, air quality)
- Alarm management interface with:
  - Time picker
  - Day selection
  - Sound source selection
  - Volume and duration settings
- Radio interface with:
  - Station presets
  - Volume control
  - Current track information
- Settings menu for:
  - Display brightness
  - Timezone configuration
  - WiFi settings
  - System information

### Environmental Monitoring
- Real-time temperature and humidity display
- Air quality monitoring (TVOC and eCO2)
- Light level sensing for automatic brightness
- Historical data logging (optional)

### Connectivity
- WiFi configuration via captive portal
- NTP time synchronization
- Web-based configuration interface
- OTA (Over-The-Air) updates
- REST API for remote control
- MQTT support for home automation integration

## Software Architecture

### Core Components
1. **AudioManager**: Handles all audio playback and processing
2. **AlarmManager**: Manages alarm scheduling, triggering, and volume ramping
3. **ConfigManager**: Handles configuration storage/retrieval from NVS/SD card
4. **UIManager**: Manages the LVGL-based user interface and touch input
5. **NetworkManager**: Handles WiFi, NTP, and network services
6. **SensorManager**: Manages environmental sensors and data collection

### Task Structure
1. **LVGL Task** (Core 1): 
   - UI rendering and touch input handling
   - Animation and transition effects
   - Screen updates at 30fps

2. **Display Task** (Core 0):
   - Time and date updates
   - Screen brightness control
   - Notification management

3. **Sensors Task** (Core 0):
   - Environmental sensor polling
   - Data filtering and processing
   - Event generation for threshold crossings

4. **Audio Task** (Core 0, High Priority):
   - Audio decoding and playback
   - Volume control and mixing
   - Stream buffering

5. **Network Task** (Core 0):
   - WiFi management
   - Time synchronization
   - OTA updates
   - Web server requests

## Dependencies
- **LVGL** v8.x - Light and Versatile Graphics Library
- **ArduinoJson** - For configuration handling
- **ESP8266Audio** (modified for ESP32-S3) - Audio processing
- **Adafruit SGP30 Library** - Air quality sensor
- **Adafruit SHT31 Library** - Temperature/Humidity sensor
- **AsyncTCP** - Asynchronous TCP library
- **ESPAsyncWebServer** - Asynchronous web server
- **Arduino_GFX_Library** - Hardware-accelerated graphics
- **TFT_eSPI** - Display driver with LVGL integration
- **AsyncElegantOTA** - For OTA updates

## Configuration
Configuration is stored in JSON format with the following structure:
- **Network**:
  - WiFi SSID and password
  - Hostname
  - Static IP configuration (optional)
  - NTP server settings

- **Alarms**:
  - Multiple alarm configurations
  - Time, repeat settings, and sound source
  - Volume and ramp settings

- **Display**:
  - Brightness levels (day/night)
  - Screen timeout
  - Theme (light/dark/auto)
  - Time format (12/24 hour)

- **Audio**:
  - Default volume levels
  - Equalizer settings
  - Radio station presets

- **Sensors**:
  - Calibration data
  - Update intervals
  - Alert thresholds

## Pin Configuration
- **Display**:
  - Parallel RGB interface (40-45, 47-48, 1, 3-9, 14-16, 21)
  - Backlight: GPIO 44 (PWM)

- **Touch**:
  - I2C (SDA: 17, SCL: 18)
  - INT: 16
  - RST: 15

- **Audio**:
  - I2S (BCLK: 12, LRC: 13, DOUT: 10)
  - Mute/Unmute control
  - Headphone detection

- **Sensors**:
  - I2C (SDA: 38, SCL: 37)
  - Light sensor: GPIO input

- **SD Card**:
  - SD_MMC (CMD: 13, CLK: 12, D0: 11)
  - Card detect

- **Buttons**:
  - Flash button: GPIO 0
  - Reset button: EN

## Build and Deployment

### Prerequisites
- PlatformIO Core
- Python 3.7+
- Required Python packages: platformio, esptool

### Build Instructions
1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/radiowecker-ai.git
   cd radiowecker-ai
   ```

2. Install dependencies:
   ```bash
   pio pkg install
   ```

3. Configure your settings:
   - Copy `data/config.example.json` to `data/config.json`
   - Edit `data/config.json` with your preferences

4. Build and upload:
   ```bash
   # Upload filesystem
   pio run -t uploadfs
   
   # Build and upload firmware
   pio run -t upload
   ```

5. For OTA updates:
   - Ensure device is connected to WiFi
   - Update `platformio.ini` with your device's IP address
   - Use `pio run -t upload --upload-port=<device-ip>`

### First Time Setup
1. Power on the device
2. Connect to the "RadioWecker-AP" WiFi network
3. Open http://192.168.4.1 in a web browser
4. Configure WiFi settings and save
5. The device will reboot and connect to your network
6. Access the web interface at the assigned IP address

## Troubleshooting

### Common Issues
1. **Display not working**:
   - Check all display connections
   - Verify correct pin assignments in `TFT_eSPI/User_Setup.h`
   - Ensure backlight is enabled (GPIO 44)

2. **Touch not working**:
   - Verify I2C connections (SDA/SCL)
   - Check touch controller address (0x14 for GT911)
   - Ensure INT and RST pins are properly connected

3. **Audio issues**:
   - Check I2S connections
   - Verify audio format compatibility
   - Ensure sufficient power supply

4. **WiFi connection problems**:
   - Check credentials in config
   - Verify router settings (2.4GHz band)
   - Check for IP conflicts

## Future Enhancements
- Add support for more audio codecs
- Implement sleep mode for power saving
- Add support for external temperature sensors
- Implement weather forecast display
- Add voice control integration
- Support for multiple alarm profiles
- Enhanced home automation integration

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments
- LVGL community for the excellent graphics library
- ESP32 Arduino core developers
- All library maintainers and contributors

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
