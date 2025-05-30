#include "SafeTouchController.h"
#include "Globals.h" // For I2C management functions

// Constructor
SafeTouchController::SafeTouchController(int sda, int scl, int rst, uint16_t width, uint16_t height) 
    : _sda(sda), _scl(scl), _rst(rst), _width(width), _height(height) {
    // Input validation to prevent errors
    if (_sda < 0 || _sda >= 48) _sda = 17; // Default to 17 if invalid
    if (_scl < 0 || _scl >= 48) _scl = 18; // Default to 18 if invalid
    if (_rst < 0 || _rst >= 48) _rst = 38; // Default to 38 if invalid
}

// Initialize the touch controller safely
bool SafeTouchController::begin() {
    Serial.println("Initializing SafeTouchController...");
    
    // Initialize I2C (using our global init function to avoid duplication)
    if (!initI2C(_sda, _scl, true)) {
        Serial.println("[SafeTouch] Failed to initialize I2C");
        return false;
    }
    
    // Perform hardware reset
    if (!hardwareReset()) {
        Serial.println("[SafeTouch] Hardware reset failed");
        // Continue anyway, might still work
    }
    
    // Short delay after reset
    delay(50);
    
    // Try to read product ID to check if touch controller is responding
    Wire.beginTransmission(GT911_I2C_ADDR_DEFAULT);
    Wire.write(GT911_PRODUCT_ID_REG >> 8);     // High byte first
    Wire.write(GT911_PRODUCT_ID_REG & 0xFF);   // Low byte
    if (Wire.endTransmission(false) != 0) {
        Serial.println("[SafeTouch] Touch controller not responding");
        return false;
    }
    
    // Request 4 bytes (product ID is 4 bytes)
    Wire.requestFrom((uint8_t)GT911_I2C_ADDR_DEFAULT, (uint8_t)4);
    if (Wire.available() < 4) {
        Serial.println("[SafeTouch] Failed to read product ID");
        return false;
    }
    
    char id[5] = {0}; // 4 bytes + null terminator
    for (uint8_t i = 0; i < 4; i++) {
        if (Wire.available()) {
            id[i] = static_cast<char>(Wire.read());
        } else {
            Serial.println("[SafeTouch] Unexpected end of data while reading product ID");
            return false;
        }
    }
    
    Serial.printf("[SafeTouch] GT911 Product ID: %s\n", id);
    
    // Simple verification - most GT911 controllers have a product ID starting with '9'
    if (id[0] != '9') {
        Serial.println("[SafeTouch] Warning: Unexpected product ID");
        // Continue anyway
    }
    
    _initialized = true;
    Serial.println("[SafeTouch] Touch controller initialized successfully");
    return true;
}

// Read touch information
bool SafeTouchController::read() {
    if (!_initialized) {
        return false;
    }
    
    // Read status register to see if there are any touch points
    uint8_t touchStatus = readReg(GT911_POINT_STATUS_REG);
    
    // Clear the status if there is touch data
    if ((touchStatus & 0x80) == 0x80) {
        // Clear touch status flag
        writeReg(GT911_POINT_STATUS_REG, 0);
    }
    
    // Check if any touch points are detected (lower 7 bits of status)
    touchPoints = touchStatus & 0x0F; // Get number of touch points (up to 5)
    touchDetected = (touchPoints > 0);
    
    // If no touch detected, return early
    if (!touchDetected) {
        return true;
    }
    
    // Read point 1 coordinates (only supporting single touch for now)
    uint8_t pointData[4];
    if (!readBlockData(GT911_POINT1_X_REG, pointData, 4)) {
        Serial.println("[SafeTouch] Failed to read point data");
        return false;
    }
    
    // GT911 data format: 
    // - X coordinate is little-endian (low byte first)
    // - Y coordinate follows the same pattern
    x = static_cast<int16_t>((pointData[1] << 8) | pointData[0]);
    y = static_cast<int16_t>((pointData[3] << 8) | pointData[2]);
    
    // Rotate coordinates if needed based on display rotation
    // For now, just making sure they're within bounds
    x = constrain(x, 0, static_cast<int16_t>(_width - 1));
    y = constrain(y, 0, static_cast<int16_t>(_height - 1));
    
    return true;
}

// Hardware reset using the reset pin
bool SafeTouchController::hardwareReset() {
    if (_rst < 0) {
        Serial.println("[SafeTouch] No reset pin defined, skipping hardware reset");
        return false;
    }
    
    Serial.printf("[SafeTouch] Performing hardware reset using pin %d\n", _rst);
    
    // Configure reset pin
    pinMode(_rst, OUTPUT);
    
    // Perform the reset sequence
    digitalWrite(_rst, HIGH);
    delay(10);
    digitalWrite(_rst, LOW);
    delay(20);
    digitalWrite(_rst, HIGH);
    delay(100);  // Allow time for device to initialize
    
    return true;
}

// Write data to register
bool SafeTouchController::writeReg(uint16_t reg, uint8_t data) {
    Wire.beginTransmission(GT911_I2C_ADDR_DEFAULT);
    Wire.write(reg >> 8);        // High byte
    Wire.write(reg & 0xFF);      // Low byte
    Wire.write(data);
    return (Wire.endTransmission() == 0);
}

// Read a single byte from register
uint8_t SafeTouchController::readReg(uint16_t reg) {
    Wire.beginTransmission(GT911_I2C_ADDR_DEFAULT);
    Wire.write(reg >> 8);        // High byte
    Wire.write(reg & 0xFF);      // Low byte
    if (Wire.endTransmission(false) != 0) {
        return 0;
    }
    
    Wire.requestFrom((uint8_t)GT911_I2C_ADDR_DEFAULT, (uint8_t)1);
    if (!Wire.available()) {
        return 0;
    }
    
    return Wire.read();
}

// Read multiple bytes from register
bool SafeTouchController::readBlockData(uint16_t reg, uint8_t *buffer, uint8_t length) {
    if (buffer == nullptr || length == 0) {
        return false;
    }
    
    Wire.beginTransmission(GT911_I2C_ADDR_DEFAULT);
    Wire.write(reg >> 8);        // High byte
    Wire.write(reg & 0xFF);      // Low byte
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    
    Wire.requestFrom((uint8_t)GT911_I2C_ADDR_DEFAULT, (uint8_t)length);
    if (Wire.available() < length) {
        return false;
    }
    
    for (uint8_t i = 0; i < length; i++) {
        // Check if there's data available before reading
        if (Wire.available()) {
            buffer[i] = Wire.read();
        } else {
            Serial.println("[SafeTouch] Unexpected end of data while reading block");
            return false;
        }
    }
    
    return true;
}
