#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// Define pins
#define BACKLIGHT_PIN 44
#define LED_PIN 2

// Global variables for the display
Arduino_ESP32RGBPanel *rgbpanel = NULL;
Arduino_RGB_Display *gfx = NULL;
bool display_initialized = false;

// Timing variables
unsigned long lastHeartbeat = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long bootTime = 0;

// Forward declarations
void setup();
void loop();
void initDisplay();
void printDiagnostics();
void blinkLed(int times, int delayMs);

// Helper function to blink the LED
void blinkLed(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
  digitalWrite(LED_PIN, HIGH); // Leave it on when done
}

// Initialize display carefully
void initDisplay() {
  Serial.println("\n[DISPLAY] Starting display initialization...");
  
  // Step 1: Create the RGB panel object
  Serial.println("[DISPLAY] Creating RGB panel controller...");
  try {
    rgbpanel = new Arduino_ESP32RGBPanel(
      40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
      45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
      5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
      8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
      0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 8 /* hsync_back_porch */,
      0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 8 /* vsync_back_porch */,
      1 /* pclk_active_neg */, 16000000 /* prefer_speed */);
    
    if (!rgbpanel) {
      Serial.println("[DISPLAY] ERROR: Failed to create RGB panel controller!");
      return;
    }
    
    // Step 2: Create the display object
    Serial.println("[DISPLAY] Creating display object...");
    gfx = new Arduino_RGB_Display(
      800 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */);
    
    if (!gfx) {
      Serial.println("[DISPLAY] ERROR: Failed to create display object!");
      return;
    }
    
    // Step 3: Initialize the display
    Serial.println("[DISPLAY] Initializing display...");
    if (!gfx->begin()) {
      Serial.println("[DISPLAY] ERROR: Display initialization failed!");
      return;
    }
    
    // Success!
    Serial.println("[DISPLAY] Display initialized successfully!");
    display_initialized = true;
    
    // Set display brightness
    analogWrite(BACKLIGHT_PIN, 127); // 50% brightness
    
    // Draw a test pattern
    Serial.println("[DISPLAY] Drawing test pattern...");
    gfx->fillScreen(BLACK);
    delay(500);
    
    // Draw color bars
    gfx->fillRect(0, 0, 800, 80, RED);
    gfx->fillRect(0, 80, 800, 80, GREEN);
    gfx->fillRect(0, 160, 800, 80, BLUE);
    gfx->fillRect(0, 240, 800, 80, YELLOW);
    gfx->fillRect(0, 320, 800, 80, MAGENTA);
    gfx->fillRect(0, 400, 800, 80, CYAN);
    
    // Add text
    gfx->setTextColor(WHITE);
    gfx->setTextSize(3);
    gfx->setCursor(200, 200);
    gfx->println("Display Test Success!");
    
  } catch (...) {
    Serial.println("[DISPLAY] EXCEPTION: Caught an error during display initialization!");
    return;
  }
  
  Serial.println("[DISPLAY] Display initialization complete!");
}

// Print system diagnostics
void printDiagnostics() {
  Serial.println("\n[DIAG] ESP32-S3 System Diagnostics");
  Serial.println("----------------------------------");
  
  // System information
  Serial.printf("[DIAG] Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("[DIAG] CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("[DIAG] Flash Size: %d bytes\n", ESP.getFlashChipSize());
  Serial.printf("[DIAG] Flash Speed: %d Hz\n", ESP.getFlashChipSpeed());
  
  // Memory information
  Serial.printf("[DIAG] Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("[DIAG] Min Free Heap: %d bytes\n", ESP.getMinFreeHeap());
  Serial.printf("[DIAG] Max Alloc Heap: %d bytes\n", ESP.getMaxAllocHeap());
  
  // PSRAM information (if available)
  #ifdef BOARD_HAS_PSRAM
    Serial.printf("[DIAG] PSRAM Size: %d bytes\n", ESP.getPsramSize());
    Serial.printf("[DIAG] Free PSRAM: %d bytes\n", ESP.getFreePsram());
  #else
    Serial.println("[DIAG] PSRAM: Not available");
  #endif
  
  // Runtime statistics
  Serial.printf("[DIAG] Uptime: %lu seconds\n", millis()/1000);
  
  // Display status
  Serial.printf("[DIAG] Display: %s\n", display_initialized ? "Initialized" : "Not initialized");
  
  // Temperature sensor (available on ESP32-S3)
  #ifdef ESP_IDF_VERSION_MAJOR // ESP-IDF 4.0+
    float tsens = temperatureRead();
    Serial.printf("[DIAG] Temperature: %.1f °C\n", tsens);
  #else
    Serial.println("[DIAG] Temperature sensor not available");
  #endif
  
  Serial.println("----------------------------------");
}

void setup() {
  // Configure basic pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  
  // Ensure LED is on to show we're alive
  digitalWrite(LED_PIN, HIGH);
  
  // Initially set backlight to LOW (off)
  digitalWrite(BACKLIGHT_PIN, LOW);
  
  // Delay for stability
  delay(3000);
  
  // Initialize serial port
  Serial.begin(115200);
  delay(500);
  
  // Disable watchdog timers
  disableCore0WDT();
  disableCore1WDT();
  disableLoopWDT();
  
  // Boot banner
  Serial.println("\n\n");
  Serial.println("*************************************");
  Serial.println("*  ESP32-S3 INCREMENTAL GFX BUILD  *");
  Serial.println("*************************************");
  
  // Print initial diagnostics
  printDiagnostics();
  
  // Visual indicator we're starting display initialization
  blinkLed(3, 200);
  
  // Initialize display
  initDisplay();
  
  // Mark boot time
  bootTime = millis();
  
  // Final setup message
  Serial.println("[BOOT] Setup complete - entering main loop");
  Serial.println("=========================================");
}

void loop() {
  // Heartbeat with LED toggle and serial output
  if (millis() - lastHeartbeat >= 1000) {
    lastHeartbeat = millis();
    
    // Toggle LED
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    
    // Print heartbeat message
    Serial.printf("[HEARTBEAT] Uptime: %lu s, Display: %s\n", 
                 millis()/1000, 
                 display_initialized ? "OK" : "Failed");
  }
  
  // Update display if initialized (every 5 seconds)
  if (display_initialized && (millis() - lastDisplayUpdate >= 5000)) {
    lastDisplayUpdate = millis();
    
    // Print message that we're updating the display
    Serial.println("[DISPLAY] Updating display...");
    
    // Draw a simple animation or update
    static int counter = 0;
    counter++;
    
    // Update counter on screen
    gfx->fillRect(300, 300, 200, 50, BLACK);
    gfx->setCursor(300, 300);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->printf("Counter: %d", counter);
  }
  
  // Small delay to prevent tight looping
  delay(10);
}
