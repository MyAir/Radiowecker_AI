#include <Arduino_GFX_Library.h>
#include <TAMC_GT911.h>
#include <Wire.h>

// #define PWM_PIN 44
#define I2C_SDA_PIN 17
#define I2C_SCL_PIN 18
#define TOUCH_INT -1
#define TOUCH_RST 38

#define TOUCH_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_X1 800
#define TOUCH_MAP_X2 0
#define TOUCH_MAP_Y1 480
#define TOUCH_MAP_Y2 0

#define GFX_BL 44
#define TFT_BL GFX_BL

// const int PWM_FREQ = 50000;
// const int PWM_CHANNEL = 0;
// const int pwm_resolution_bits = 8;
#define PWM_CHANNEL 1
#define PWM_FREQ 5000 // Hz
#define pwm_resolution_bits 10

// Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
//     GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
//     40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
//     45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
//     5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
//     8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */
// );

// Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
//     bus,
//     800 /* width */, 0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 8 /* hsync_back_porch */,
//     480 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 8 /* vsync_back_porch */,
//     1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);

#if 0  //for Arduino_GFX_Library v1.3.1
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
    40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
    5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
    8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */
);

// Uncomment for ST7262 IPS LCD 800x480
 Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
   bus,
   800 /* width */, 0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 8 /* hsync_back_porch */,
   480 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 8 /* vsync_back_porch */,
   1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);
#endif  //end of  for Arduino_GFX_Library v1.3.1

#if 1 //for Arduino_GFX_Library v1.4.7
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
     40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
     45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
     5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
     8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
     0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 8 /* hsync_back_porch */,
     0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 8 /* vsync_back_porch */,
     1 /* pclk_active_neg */, 16000000 /* prefer_speed */);
 Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
     800 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */);   
#endif   //End of for Arduino_GFX_Library v1.4.7

int touch_last_x = 0, touch_last_y = 0;

TAMC_GT911 ts = TAMC_GT911(I2C_SDA_PIN, I2C_SCL_PIN, TOUCH_INT, TOUCH_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

void touch_init(void)
{
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ts.begin();
}

void setup()
{
  Serial.begin(115200);
  
  pinMode(TFT_BL, OUTPUT);
  // digitalWrite(TFT_BL, LOW);
  digitalWrite(TFT_BL, HIGH);
  
  
  pinMode(TOUCH_RST, OUTPUT);
  delay(100);
  digitalWrite(TOUCH_RST, LOW);
  delay(1000);
  digitalWrite(TOUCH_RST, HIGH);
  delay(1000);
  
  ledcSetup(PWM_CHANNEL, PWM_FREQ, pwm_resolution_bits); // Einrichten von Kanaelen
  ledcAttachPin(TFT_BL, PWM_CHANNEL);  // Verbinden Sie die Kanaele mit den entsprechenden Pins
  ledcWrite(PWM_CHANNEL, 0); // output PWM
  
  
  digitalWrite(TOUCH_RST, LOW);
  delay(1000);
  digitalWrite(TOUCH_RST, HIGH);
  delay(1000);
  
  touch_init();

  //Init Display
  gfx->begin();
  gfx->fillScreen(BLACK);
  // ledcWrite(PWM_CHANNEL, 240);

  // gfx->drawRoundRect(100,200,300,40,40,WHITE);
  gfx->drawRoundRect(100,200,560,40,40,WHITE);
  gfx->drawCircle(120, 220, 20, BLUE);
  gfx->fillCircle(120, 220, 20, BLUE);
  while (1)
  {
    ts.read();
    if (ts.isTouched) 
    {
      for (int i = 0; i < ts.touches; i++) 
      {
        if(abs(ts.points[i].y-480)>=150&&abs(ts.points[i].y-480)<=300)
        {
          // if(abs(ts.points[i].x-480)>=100&&abs(ts.points[i].x-480)<=410)
          if(abs(ts.points[i].x-800)>=100&&abs(ts.points[i].x-800)<=750)
          {
            gfx->fillScreen(BLACK);
            // gfx->drawRoundRect(100,200,300,40,40,WHITE);
            gfx->drawRoundRect(100,200,560,40,40,WHITE);
            // gfx->fillRoundRect(100,200,abs(ts.points[i].x-380),40,40,BLUE);
            gfx->fillRoundRect(100,200,abs(ts.points[i].x-700),40,40,BLUE);
            gfx->drawCircle(120, 220, 20, BLUE);
            gfx->fillCircle(120, 220, 20, BLUE);
            ledcWrite(PWM_CHANNEL, abs(ts.points[i].x)-130);
          }
          // else if(abs(ts.points[i].x-480)>=410)
          else if(abs(ts.points[i].x-800)>=750)
          {
            gfx->fillScreen(BLACK);
            // gfx->drawRoundRect(100,200,300,40,40,WHITE);
            // gfx->fillRoundRect(100,200,300,40,40,BLUE);
            gfx->drawRoundRect(100,200,560,40,40,WHITE);
            gfx->fillRoundRect(100,200,560,40,40,BLUE);
            gfx->drawCircle(120, 220, 20, BLUE);
            gfx->fillCircle(120, 220, 20, BLUE);
            ledcWrite(PWM_CHANNEL, 0);
          }
          // else if(abs(ts.points[i].x-480)<=100)
          else if(abs(ts.points[i].x-800)<=100)
          {
            gfx->fillScreen(BLACK);
            // gfx->drawRoundRect(100,200,300,40,40,WHITE);
            gfx->drawRoundRect(100,200,560,40,40,WHITE);
            gfx->drawCircle(120, 220, 20, BLUE);
            gfx->fillCircle(120, 220, 20, BLUE);
            // ledcWrite(PWM_CHANNEL, 240);
            ledcWrite(PWM_CHANNEL, 1000);
          }
        }
        Serial.print("Touch "); Serial.print(i + 1); Serial.print(": ");;
        // Serial.print("  x: "); Serial.print(abs(ts.points[i].x-480));
        Serial.print("  x: "); Serial.print(abs(ts.points[i].x-800));
        Serial.print("  y: "); Serial.print(abs(ts.points[i].y-480));
        Serial.print("  size: "); Serial.println(ts.points[i].size);
        Serial.println(' ');
      }
      ts.isTouched = false;
      if (ts.touches > 2) break;
    }
    delay(100);
  }
}

void loop() 
{  
    //ledcWrite(PWM_CHANNEL, 0);//0--250-->Hell-Dunkel

}  
