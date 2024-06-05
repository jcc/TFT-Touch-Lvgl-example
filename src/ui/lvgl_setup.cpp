#include <Arduino.h>
#include <SimpleButton.h>
#include <lvgl.h>
#define LGFX_USE_V1 // Define before #include <LovyanGFX.hpp>
#include <LovyanGFX.hpp>
#include "pin_config.h"
#include "lvgl_setup.h"

#define LCD_WIDTH 240
#define LCD_HEIGHT 280

#define BUTTON_SUPPORT 0

/***************************************************************************************************
 * LGFX Setup
 ***************************************************************************************************/

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789      _panel_instance;
    lgfx::Bus_SPI           _bus_instance;
    lgfx::Light_PWM         _light_instance;
    lgfx::Touch_CST816S     _touch_instance;

public:
    LGFX(void)
    {
        {
          auto cfg = _bus_instance.config();
          cfg.spi_host = HSPI_HOST;
          cfg.spi_mode = 2;
          cfg.freq_write = 40000000;
          cfg.freq_read  = 16000000;
          cfg.spi_3wire  = true;
          cfg.use_lock   = true;
          cfg.dma_channel = SPI_DMA_CH_AUTO;
          cfg.pin_sclk = LCD_SCK;
          cfg.pin_mosi = LCD_MOSI;
          cfg.pin_miso = LCD_MISO;
          cfg.pin_dc   = LCD_DC;
          _bus_instance.config(cfg);
          _panel_instance.setBus(&_bus_instance);
        }

        {                           
            auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。

            cfg.pin_cs           =    LCD_CS;
            cfg.pin_rst          =    LCD_RST;
            cfg.pin_busy         =    -1;
            cfg.panel_width      =   LCD_WIDTH;
            cfg.panel_height     =   LCD_HEIGHT;
            cfg.offset_x         =     0;
            cfg.offset_y         =     20;
            cfg.offset_rotation  =     0;
            cfg.dummy_read_pixel =     8;
            cfg.dummy_read_bits  =     1;
            cfg.readable         =  true;
            cfg.invert           = true;
            cfg.rgb_order        = true;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       =  true;

            _panel_instance.config(cfg);
        }

        {                                        // バックライト制御の設定を行います。（必要なければ削除）
            auto cfg = _light_instance.config(); // バックライト設定用の構造体を取得します。

            cfg.pin_bl = LCD_BL; // バックライトが接続されているピン番号
            cfg.invert = false;  // バックライトの輝度を反転させる場合 true
            cfg.freq = 44100;    // バックライトのPWM周波数
            cfg.pwm_channel = 7; // 使用するPWMのチャンネル番号

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance); // バックライトをパネルにセットします。
        }

        { // タッチスクリーン制御の設定を行います。（必要なければ削除）
            auto cfg = _touch_instance.config();
            cfg.x_min = 0;              // タッチスクリーンから得られる最小のX値(生の値)
            cfg.x_max = LCD_WIDTH - 1;  // タッチスクリーンから得られる最大のX値(生の値)
            cfg.y_min = 0;              // タッチスクリーンから得られる最小のY値(生の値)
            cfg.y_max = LCD_HEIGHT - 1; // タッチスクリーンから得られる最大のY値(生の値)
            cfg.pin_int = LCD_INT_T;           // INTが接続されているピン番号
            cfg.bus_shared = true;      // 画面と共通のバスを使用している場合 trueを設定
            cfg.offset_rotation = 0;    // 表示とタッチの向きのが一致しない場合の調整 0~7の値で設定

            // SPI接続の場合
            cfg.spi_host = HSPI_HOST;// 使用するSPIを選択 (HSPI_HOST or VSPI_HOST)
            cfg.freq = 1000000;     // SPIクロックを設定
            cfg.pin_sclk = LCD_SCK;     // SCLKが接続されているピン番号
            cfg.pin_mosi = LCD_MOSI;     // MOSIが接続されているピン番号
            cfg.pin_miso = LCD_MISO;     // MISOが接続されているピン番号
            cfg.pin_cs   = LCD_CS;     //   CSが接続されているピン番号

            // I2C接続の場合
            cfg.i2c_port = 1;      // 使用するI2Cを選択 (0 or 1)
            cfg.i2c_addr = 0x15;   // I2Cデバイスアドレス番号
            cfg.pin_sda  = LCD_SDA_T;     // SDAが接続されているピン番号
            cfg.pin_scl  = LCD_SCL_T;     // SCLが接続されているピン番号
            cfg.freq = 400000;     // I2Cクロックを設定

            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance); // タッチスクリーンをパネルにセットします。
        }
        setPanel(&_panel_instance); // 使用するパネルをセットします。
    }
};

LGFX tft;

/***************************************************************************************************
 * LVGL Setup
 ***************************************************************************************************/

/* Display flushing */
static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    // tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
static void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);
    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.println("X = " + String(touchX) + " Y = " + String(touchY));
    }
}

#if BUTTON_SUPPORT
static void buttons_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    static SimpleButton button_next(BUTTON_NEXT);
    static SimpleButton button_prev(BUTTON_PREV);
    static SimpleButton button_enter(BUTTON_ENTER);

    if (button_next.pressed())
    {
        Serial.println("Next");
        data->key = LV_KEY_NEXT;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else if (button_prev.pressed())
    {
        Serial.println("Prev");
        data->key = LV_KEY_PREV;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else if (button_enter.pressed())
    {
        Serial.println("Enter");
        data->key = LV_KEY_ENTER;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
#endif

void lvgl_init()
{
    /*Setup LCD*/
    tft.begin();
    tft.setBrightness(128);
    // tft.setRotation(0);

    /*Calibrate touch screen*/
    // uint16_t calData[] = {239, 3926, 233, 265, 3856, 3896, 3714, 308};
    // tft.setTouchCalibrate(calData);

    /*Initialize the buffer*/
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[LCD_HEIGHT * LCD_WIDTH / 10];
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LCD_HEIGHT * LCD_WIDTH / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    /*Change the following line to your display resolution*/
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the touch device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

#if BUTTON_SUPPORT
    /*Initialize the keypad device driver*/
    static lv_indev_drv_t indev_drv_2;
    lv_indev_drv_init(&indev_drv_2);
    indev_drv_2.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_2.read_cb = buttons_read;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv_2);
    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);
    lv_indev_set_group(indev, g);
#endif

    Serial.printf("Lvgl v%d.%d.%d initialized\n", lv_version_major(), lv_version_minor(), lv_version_patch());
}

void lvgl_handler()
{
    const uint16_t period = 5;
    static uint32_t last_tick = 0;
    uint32_t tick = millis();
    if (tick - last_tick > period)
    {
        last_tick = tick;
        lv_task_handler();
    }
}