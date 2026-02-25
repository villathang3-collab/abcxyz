#pragma once

// Pin definitions for Freenove ESP32-S3-WROOM-N16R8

#define BOARD_NAME "Freenova S3"

// Standard Arduino pins
static const uint8_t TX = 43;
static const uint8_t RX = 44;

static const uint8_t SDA = 16;
static const uint8_t SCL = 15;

// SPI (Corrected for TFT)
static const uint8_t SS    = 10; // TFT_CS
static const uint8_t MOSI  = 11;
static const uint8_t MISO  = 13;
static const uint8_t SCK   = 12;

// TFT Display (Corrected)
#define TFT_MOSI 11
#define TFT_MISO 13
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   46
#define TFT_RST  -1 // Not specified, managed by board
#define TFT_BL   45

// Touch Screen (FT6336U)
#define TOUCH_SDA 16
#define TOUCH_SCL 15
#define TOUCH_RST 18
#define TOUCH_INT 17

// SD Card (SD_MMC 4-bit)
#define SD_MMC_CLK 38
#define SD_MMC_CMD 40
#define SD_MMC_D0  39
#define SD_MMC_D1  41
#define SD_MMC_D2  48
#define SD_MMC_D3  47

// RGB LED
#define RGB_LED_PIN 42

// USB
#define USB_D_N 19
#define USB_D_P 20
