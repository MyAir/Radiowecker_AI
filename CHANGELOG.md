# Radiowecker AI Changelog

## [1.2.5] - 2025-05-30

### Fixed
- Fixed time calculation issues in weather forecast periods
- Added proper error handling for time-related functions
- Implemented fallback defaults for invalid sunrise/sunset times
- Improved debug output for time range calculations
- Fixed initialization of time structures to prevent January 1st, 1970 timestamps
- Added DST (Daylight Saving Time) awareness to time calculations

## [1.2.4] - 2025-05-30

### Added
- Added night forecast period to complement morning and afternoon forecasts
- Implemented smarter forecast period calculations based on sunrise and sunset times
- Improved forecast time ranges to better match time of day

## [1.2.3] - 2025-05-30

### Fixed
- Fixed Guru Meditation crash caused by null pointer dereference when WiFi is unavailable
- Added fail-safe defaults for weather data when API requests fail
- Improved memory management in WeatherService to prevent crashes
- Fixed multiple issues with weather icon handling in UIManager:
  - Removed duplicate code that attempted to delete icons twice
  - Fixed parent container references for weather icons
  - Added additional null checks to prevent crashes
  - Ensured proper LVGL object creation and deletion

## [1.2.2] - 2025-05-30

### Fixed
- Resolved compiler warnings in SafeTouchController implementation
- Reduced excessive debug output for time updates

### Improved
- Enhanced error handling in I2C communication with touch controller
- Added proper type conversions to prevent potential issues
- Improved robustness of touch data reading

## [1.2.1] - 2025-05-30

### Changed
- Improved status bar update frequency from 30 seconds to 10 seconds
- Verified OpenWeatherMap API query frequency is correctly set to 5 minutes

### Technical Details
- Modified update_display_task to refresh WiFi status, IP address, and signal quality every 10 seconds
- Confirmed WeatherService updateInterval is set to 300000ms (5 minutes) as required

## [1.2.0] - 2025-05-30

### Added
- Ultra-minimal memory-efficient approach for parsing hourly weather forecasts
- Improved debug output for weather forecast processing
- Detailed timestamp logs for hourly weather data

### Fixed
- Resolved memory issues in the hourly weather data processing
- Fixed structural code issues in WeatherService.cpp
- Ensured proper JSON navigation and parsing for OpenWeatherMap API

### Technical Details
- New approach processes one hourly forecast entry at a time to minimize memory usage
- Uses string methods for basic JSON navigation instead of loading entire response at once
- Implements focused JSON document filters to extract only necessary data
- Resolves NoMemory errors that were preventing proper forecast display

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
