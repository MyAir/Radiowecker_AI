#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

// Forward declarations for manager classes
class DisplayManager;
class UIManager;
class ConfigManager;
class AudioManager;
class AlarmManager;

// No more global instances - use singleton pattern instead
// Example: DisplayManager::getInstance()

// Global variables for system state
extern bool g_i2c_initialized;
extern unsigned long g_last_touch_time;

// Global utility functions for I2C management
bool initI2C(int sda, int scl, bool force = false);
void resetI2C(int sda, int scl);

#endif // GLOBALS_H
