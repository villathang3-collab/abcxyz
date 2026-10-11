#include "core/powerSave.h"
#include "core/utils.h"
#include <globals.h>
#include <interface.h>
#include <Wire.h> 

#ifndef MINBRIGHT
#define MINBRIGHT 1
#endif

// --- KHAI BÁO CẢM ỨNG THỦ CÔNG ---
#ifdef HAS_TOUCH
#define CST816_ADDR 0x15

// Hàm tự viết để lấy cả Tọa độ X, Y và Cử chỉ (Gesture)
bool getTouchData(int &x, int &y, uint8_t &gesture) {
    Wire.beginTransmission(CST816_ADDR);
    Wire.write(0x01); // Thanh ghi 0x01 chứa Gesture ID, 0x02 chứa Fingers...
    if (Wire.endTransmission(false) != 0) return false; 

    // Đọc 6 byte liên tục: Gesture, Fingers, Xh, Xl, Yh, Yl
    Wire.requestFrom(CST816_ADDR, 6); 
    if (Wire.available() >= 6) {
        gesture = Wire.read();
        uint8_t fingers = Wire.read();
        uint8_t xh = Wire.read();
        uint8_t xl = Wire.read();
        uint8_t yh = Wire.read();
        uint8_t yl = Wire.read();

        if (fingers > 0) { 
            x = ((xh & 0x0F) << 8) | xl;
            y = ((yh & 0x0F) << 8) | yl;
            return true;
        }
    }
    return false; 
}
#endif
// --------------------------------

void _setup_gpio() {
    // 1. Khóa toàn bộ các chân CS lên HIGH
    pinMode(TFT_CS, OUTPUT);    digitalWrite(TFT_CS, HIGH);
    pinMode(SDCARD_CS, OUTPUT); digitalWrite(SDCARD_CS, HIGH);
    pinMode(CC1101_SS_PIN, OUTPUT); digitalWrite(CC1101_SS_PIN, HIGH);
    pinMode(NRF24_SS_PIN, OUTPUT); digitalWrite(NRF24_SS_PIN, HIGH);
    pinMode(W5500_SS_PIN, OUTPUT); digitalWrite(W5500_SS_PIN, HIGH);

    // 2. Setup màn hình
    pinMode(TFT_MOSI, OUTPUT);  digitalWrite(TFT_MOSI, HIGH);
    pinMode(TFT_SCLK, OUTPUT);
    pinMode(TFT_BL, OUTPUT);    digitalWrite(TFT_BL, HIGH);
    pinMode(TFT_RST, OUTPUT);
    pinMode(TFT_DC, OUTPUT);    digitalWrite(TFT_DC, HIGH);

#ifdef ILI9341_DRIVER
    bruceConfig.colorInverted = 0;
#endif

    // 3. Khởi tạo màn hình
    tft.init(); 
}

void _post_setup_gpio() {
#ifdef HAS_TOUCH
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(400000); // 400kHz cho ổn định

    // Gửi lệnh tắt chế độ Sleep tự động của CST816 (Theo code của bạn)
    Wire.beginTransmission(CST816_ADDR);
    Wire.write(0xFE); Wire.write(0x00); 
    Wire.endTransmission();

    Wire.beginTransmission(CST816_ADDR);
    Wire.write(0xEE); Wire.write(0x00); 
    Wire.endTransmission();

    Serial.println("Raw CST816 Touch Init OK!");
#endif
}

int getBattery() { return 0; }
bool isCharging() { return false; }

void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

#ifdef HAS_TOUCH
    checkPowerSaveTime();
    
    int x, y;
    uint8_t gesture;

    if (getTouchData(x, y, gesture)) {
        tm = millis();

        NextPress = false; PrevPress = false; 
        SelPress = false;  EscPress = false;
        UpPress = false;   DownPress = false;
        
        // Map mã Gesture của CST816 sang Bruce OS
        if (gesture == 0x01) {
            PrevPress = true; // Vuốt lên
        } else if (gesture == 0x02) {
            NextPress = true; // Vuốt xuống
        } else if (gesture == 0x05 || gesture == 0x0B) {
            SelPress = true;  // Nhấp 1 lần hoặc nhấp đúp
        } else if (gesture == 0x03 || gesture == 0x04) {
            EscPress = true;  // Vuốt trái/phải
        }

        if (!wakeUpScreen()) {
            AnyKeyPress = true; 
            return; 
        }

        int mapped_x = tftWidth - x; 
        int mapped_y = (tftHeight + 20) - y;

        touchPoint.x = mapped_x;
        touchPoint.y = mapped_y;
        touchPoint.pressed = true;
        touchHeatMap(touchPoint);
        
    } else {
        touchPoint.pressed = false;
    }
#endif
}

void powerOff() { digitalWrite(TFT_BL, LOW); }
void checkReboot() {}