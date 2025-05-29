#ifndef SAFE_TOUCH_CONTROLLER_H
#define SAFE_TOUCH_CONTROLLER_H

#include <Arduino.h>
#include <Wire.h>

// GT911 Touch Controller Registers
#define GT911_I2C_ADDR_DEFAULT      0x5D  // Default I2C address of GT911
#define GT911_PRODUCT_ID_REG        0x8140
#define GT911_COORD_REG             0x814E  // Start of coordinate registers
#define GT911_POINT_STATUS_REG      0x814E  // Read this to get touch point status
#define GT911_TOUCH_REG             0x814F  // Contains number of touch points
#define GT911_POINT1_X_REG          0x8150  // First point X position, followed by Y

class SafeTouchController {
public:
    // Basic info about touch state
    bool touchDetected = false;
    uint8_t touchPoints = 0;
    int16_t x = 0;
    int16_t y = 0;
    
    // Constructor
    SafeTouchController(int sda, int scl, int rst, uint16_t width, uint16_t height);
    
    // Safe initialization
    bool begin();
    
    // Read touch data
    bool read();
    
    // Check if touched
    bool isTouched() { return touchDetected; }
    
    // Reset the controller (hardware method)
    bool hardwareReset();
    
private:
    int _sda;
    int _scl;
    int _rst;
    uint16_t _width;
    uint16_t _height;
    bool _initialized = false;

    // Low-level I2C communication with error handling
    bool writeReg(uint16_t reg, uint8_t data);
    uint8_t readReg(uint16_t reg);
    bool readBlockData(uint16_t reg, uint8_t *buffer, uint8_t length);
};

#endif // SAFE_TOUCH_CONTROLLER_H
