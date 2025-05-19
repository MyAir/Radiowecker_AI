#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "touch.h"
#include <TAMC_GT911.h>
#include <Wire.h>
#include <ui.h>

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
#define PWM_CHANNEL 0
#define PWM_FREQ 5000 // Hz
#define pwm_resolution_bits 10

/*Don't forget to set Sketchbook location in File/Preferences to the path of your UI project (the parent foder of this INO file)*/

/*Change to your screen resolution*/
static const uint16_t screenWidth  = 800;
static const uint16_t screenHeight = 480;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 10 ];


#if 1  //for Arduino_GFX_Library v1.3.1
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

#if 0 //for Arduino_GFX_Library v1.4.7
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

#if 0
int touch_last_x = 0, touch_last_y = 0;

TAMC_GT911 ts = TAMC_GT911(I2C_SDA_PIN, I2C_SCL_PIN, TOUCH_INT, TOUCH_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

void touch_init(void)
{
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ts.begin();
  ts.setRotation(TOUCH_ROTATION);
}
#endif

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap( area->x1, area->y1, (uint16_t*) &color_p->full, w, h );
#else
    gfx->draw16bitRGBBitmap( area->x1, area->y1, (uint16_t*) &color_p->full, w, h );
#endif

    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{    
  ts.read();
  if (ts.isTouched)
  {
    int16_t touchX = ts.points[0].x;
    int16_t touchY = ts.points[0].y;

    data->state = LV_INDEV_STATE_PR;

    data->point.x = touchX;
    data->point.y = touchY;

    Serial.print("x=");
    Serial.println(touchX);
    Serial.print("y=");
    Serial.println(touchY);
    Serial.println("..........................");
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

void setup()
{
    Serial.begin( 115200 ); /* prepare for possible serial debug */

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println( LVGL_Arduino );
    Serial.println( "I am LVGL_Arduino" );

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    // digitalWrite(TFT_BL, LOW);
    
    pinMode(TOUCH_RST, OUTPUT);
    delay(100);
    digitalWrite(TOUCH_RST, LOW);
    delay(1000);
    digitalWrite(TOUCH_RST, HIGH);
    delay(1000);
  
    ledcSetup(PWM_CHANNEL, PWM_FREQ, pwm_resolution_bits);
    ledcAttachPin(TFT_BL, PWM_CHANNEL);
  
    // ledcWrite(PWM_CHANNEL, 1023); // output PWM
    ledcWrite(PWM_CHANNEL, 10); // output PWM

    lv_init();

    digitalWrite(TOUCH_RST, LOW);
    delay(1000);
    digitalWrite(TOUCH_RST, HIGH);
    delay(1000);
    touch_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print ); /* register print function for debugging */
#endif

    // Init Display
    gfx->begin();
    gfx->fillScreen(BLACK);
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, LOW);

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * screenHeight / 10 );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register( &disp_drv );

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register( &indev_drv );


    ui_init();

    Serial.println( "Setup done" );
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}


