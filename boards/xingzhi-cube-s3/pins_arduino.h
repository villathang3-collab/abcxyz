#ifndef Pins_Arduino_h
#define Pins_Arduino_h
#include "soc/soc_caps.h"
#include <stdint.h>
#define USB_VID 0x303a
#define USB_PID 0x1001
#define DEVICE_NAME "Xingzhi-Cube"
#define HAS_SCREEN
#define USER_SETUP_LOADED
#define ST7789_DRIVER
#define USE_HSPI_PORT 1
#define TFT_WIDTH 240
#define TFT_HEIGHT 280

#define TFT_IPS 1
#define TFT_MOSI 10
#define TFT_SCLK 9
#define TFT_CS 14
#define TFT_DC 8
#define TFT_RST 18
#define TFT_BL 13
#define TFT_BACKLIGHT_ON HIGH
#define ROTATION 1
#define MINBRIGHT 10
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 6000000
#define SMOOTH_FONT 1
#define TOUCH_CS -1
#define HAS_TOUCH 0
#define SPI_SS_PIN 42
#define SPI_MOSI_PIN 48
#define SPI_MISO_PIN 41
#define SPI_SCK_PIN 47
static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK = SPI_SCK_PIN;
#define SDCARD_CS SPI_SS_PIN
#define SDCARD_MOSI SPI_MOSI_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_SCK SPI_SCK_PIN
#define USE_CC1101_VIA_SPI
#define CC1101_SS_PIN 2
#define CC1101_GDO0_PIN 1
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 4
#define NRF24_SS_PIN 21
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define USE_W5500_VIA_SPI
#define W5500_SS_PIN -1
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_INT_PIN -1
#define SERIAL_RX 2
#define SERIAL_TX 1
#define BAD_RX SERIAL_RX
#define BAD_TX SERIAL_TX
#define GROVE_SDA 5
#define GROVE_SCL 6
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;
#define BTN_ALIAS "\"OK\""
#define HAS_3_BUTTONS
#define BTN_ACT LOW
#define UP_BTN 39
#define DW_BTN 40
#define OK_BTN 0
#define ADC_PIN 17
#define CHARGE_PIN 38
#define RXLED 3
#define LED 12
#define LED_ON HIGH
#define LED_OFF LOW
#endif
/*Baoduscuvaomomnhi*/
