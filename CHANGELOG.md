# Radiowecker AI Changelog

## [1.1.0] - 2025-05-29

### Added
- Custom `SafeTouchController` class for direct I2C communication with GT911 touch controller
- Comprehensive error handling for sensor initialization
- I2C bus management with proper error checking and reset functionality

### Fixed
- Critical touch controller initialization crashes during boot
- Re-enabled SGP30 (air quality) and SHT31 (temperature/humidity) environmental sensors
- Fixed variable naming inconsistencies in main.cpp
- Fixed static/non-static variable handling in LVGL callbacks
- Resolved missing closing braces in DisplayManager.cpp

### Changed
- Replaced TAMC_GT911 library with custom safer implementation
- Set I2C clock speed to 100kHz for better stability with sensors
- Updated DisplayManager to use the new SafeTouchController for touch input
- Improved initialization sequence to prioritize system stability

### Technical Details
- The new `SafeTouchController` communicates directly with the GT911 touch controller via I2C
- All sensor initializations are now wrapped in try-catch blocks to prevent crashes
- ConfigManager properly tracks sensor availability
- Touch input events are successfully detected and processed
- Environmental sensor data is now correctly displayed in the UI

## [1.0.0] - 2025-05-20

### Initial Release
- Basic Radiowecker functionality with display, touch, and environmental sensors
- Weather forecast integration
- Alarm functionality
- Radio playback
- Web interface for configuration
