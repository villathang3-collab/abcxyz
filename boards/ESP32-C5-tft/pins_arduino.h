#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

/* --- BUS GIAO TIẾP CHÍNH --- */
static const uint8_t SDA = 8;
static const uint8_t SCL = 9;

#define SPI_MOSI_PIN 7
#define SPI_SCK_PIN 6
#define SPI_MISO_PIN 15 
#define SPI_SS_PIN 1 

static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK = SPI_SCK_PIN;
static const uint8_t SS = SPI_SS_PIN;

/* --- CẤU HÌNH MÀN HÌNH (ST7789) --- */
#define HAS_SCREEN 1
#define ST7789_DRIVER 1
#define TFT_WIDTH 240
#define TFT_HEIGHT 296

#define ROTATION 2
#define TFT_INVERSION_ON 1 // Đảo màu

#define MINBRIGHT 1
#define USER_SETUP_LOADED 1
#define SMOOTH_FONT 1

#define TFT_MOSI SPI_MOSI_PIN
#define TFT_MISO SPI_MISO_PIN
#define TFT_SCLK SPI_SCK_PIN
#define TFT_CS   23
#define TFT_DC   24
#define TFT_RST  10
#define TFT_BL   25
#define TFT_BACKLIGHT_ON 1

/* --- CẤU HÌNH CẢM ỨNG (CST816S) --- */
#define HAS_TOUCH 1
#define TOUCH_SDA 8
#define TOUCH_SCL 9
#define TOUCH_INT 2
/* SỬ DỤNG CHÂN 14 LÀM CHÂN ẢO ĐỂ TRÁNH LỖI INVALID IO 255 */
#define TOUCH_RST 14
#define TOUCH_CS  -1

/* =========================================
   CẤU HÌNH LED RGB
   ========================================= */
#define PIN_RGB_LED 27
static const uint8_t LED_BUILTIN = SOC_GPIO_PIN_COUNT + PIN_RGB_LED;
#define BUILTIN_LED LED_BUILTIN
#define RGB_BUILTIN LED_BUILTIN
#define RGB_BRIGHTNESS 64

#define HAS_RGB_LED 1
#define LED_ORDER GRB
#define LED_TYPE_IS_RGBW 1
#define LED_COUNT 1
#define LED_TYPE WS2812
#define LED_COLOR_STEP 15
#define RGB_LED 27

/* --- NGOẠI VI & MODULE --- */
static const uint8_t TX = 11;
static const uint8_t RX = 12;
#define BAD_RX 4
#define BAD_TX 5
#define SERIAL_TX 5
#define SERIAL_RX 4
#define RXLED 26
#define TXLED 3
#define LED_ON HIGH
#define LED_OFF LOW

/* --- ĐÃ SỬA LẠI CHÂN CS ĐỂ TRÁNH XUNG ĐỘT BUS SPI --- */
#define RF_CE_PIN 28 
#define RF_CS_PIN 13 // Chuyển sang chân 13

#define CC1101_GDO0_PIN RF_CE_PIN
#define CC1101_SS_PIN 3 // Chuyển sang chân 3
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define NRF24_CE_PIN RF_CE_PIN
#define NRF24_SS_PIN 4 // Chuyển sang chân 4
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

#define W5500_INT_PIN RF_CE_PIN
#define W5500_SS_PIN 5 // Chuyển sang chân 5
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN

/* --- THẺ NHỚ --- */
#define SDCARD_CS SPI_SS_PIN // Giữ thẻ nhớ ở chân 1
#define SDCARD_SCK SPI_SCK_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN

#endif /* Pins_Arduino_h */