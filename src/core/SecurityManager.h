#pragma once
#include "display.h"
#include "settings.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
// Include file bàn phím (đường dẫn tương đối)
#include "mykeyboard.h"

class SecurityManager {
public:
    // Trả về true nếu Key đúng, false nếu người dùng bấm thoát
    bool check() {
        // [FIX QUAN TRỌNG] Bật WiFi Station để đọc được MAC Address
        // Nếu không có dòng này, MAC sẽ là 00:00:00:00:00:00
        if (WiFi.getMode() == WIFI_MODE_NULL) {
            WiFi.mode(WIFI_STA);
            delay(50); // Đợi 50ms cho chip WiFi khởi động
        }

        if (!LittleFS.begin(true)) Serial.println("[SEC] LittleFS Error");

        String realKey = calculateKey();
        String savedKey = readSavedKey();

        // Nếu Key đã lưu khớp -> Cho qua luôn
        if (savedKey == realKey) return true;

        // Nếu chưa khớp -> Gọi bàn phím nhập liệu
        return showInputScreen(realKey);
    }

private:
    struct Digits {
        int d1 = -1;
        int d2 = -1;
        int count = 0;
    };

    // Hàm tách số từ Hex
    Digits extractDigits(uint8_t byte) {
        Digits ret;
        char hexStr[3];
        sprintf(hexStr, "%02X", byte);
        if (hexStr[0] >= '0' && hexStr[0] <= '9') {
            ret.d1 = hexStr[0] - '0';
            ret.count++;
        }
        if (hexStr[1] >= '0' && hexStr[1] <= '9') {
            if (ret.count == 0) ret.d1 = hexStr[1] - '0';
            else ret.d2 = hexStr[1] - '0';
            ret.count++;
        }
        return ret;
    }

    String calculateKey() {
        // Đảm bảo WiFi đã bật trước khi đọc MAC
        if (WiFi.getMode() == WIFI_MODE_NULL) WiFi.mode(WIFI_STA);

        uint8_t mac[6];
        WiFi.macAddress(mac); // Đọc MAC vào mảng

        long M[6];

        // M1: Tổng 2 số - 36
        Digits d0 = extractDigits(mac[0]);
        int sum0 = (d0.count == 1) ? d0.d1 : (d0.count == 2 ? d0.d1 + d0.d2 : 0);
        M[1] = sum0 - 36;

        // M2: Tổng 2 số + 12
        Digits d1 = extractDigits(mac[1]);
        int sum1 = (d1.count == 1) ? d1.d1 : (d1.count == 2 ? d1.d1 + d1.d2 : 0);
        M[2] = sum1 + 12;

        // M3: Hiệu 2 số - 2 (nếu âm +6)
        Digits d2 = extractDigits(mac[2]);
        int diff2 = (d2.count == 1) ? d2.d1 : (d2.count == 2 ? d2.d1 - d2.d2 : 0);
        long tempM3 = diff2 - 2;
        if (tempM3 < 0) tempM3 += 6;
        M[3] = tempM3;

        // M4: Hiệu 2 số - 3
        Digits d3 = extractDigits(mac[3]);
        int diff3 = (d3.count == 1) ? d3.d1 : (d3.count == 2 ? d3.d1 - d3.d2 : 0);
        M[4] = diff3 - 3;

        // M5: Tổng giá trị Decimal của 2 byte đầu - 3
        M[5] = ((long)mac[0] + (long)mac[1]) - 3;

        // KEY FINAL: Ghép chuỗi (M1-M2)*M4 và M3-M5
        long part1 = abs((M[1] - M[2]) * M[4]);
        long part2 = abs(M[3] - M[5]);

        String finalStr = String(part1) + String(part2);

        // Đệm thêm số 0 nếu ngắn hơn 6 ký tự
        while (finalStr.length() < 6) finalStr += "0";

        // Lấy đúng 6 ký tự đầu
        return finalStr.substring(0, 6);
    }

    String readSavedKey() {
        if (!LittleFS.exists("/license.key")) return "";
        File f = LittleFS.open("/license.key", "r");
        if (!f) return "";
        String k = f.readString();
        f.close();
        k.trim();
        return k;
    }

    void saveKey(String key) {
        File f = LittleFS.open("/license.key", "w");
        if (f) {
            f.print(key);
            f.close();
        }
    }

    bool showInputScreen(String correctKey) {
        // Đảm bảo WiFi bật để hiển thị MAC đúng trên tiêu đề bàn phím
        if (WiFi.getMode() == WIFI_MODE_NULL) WiFi.mode(WIFI_STA);

        while (true) {
            // Lấy MAC để hiện lên tiêu đề
            String msg = "MAC: " + WiFi.macAddress();

            // Gọi bàn phím SỐ
            String inputKey = num_keyboard("", 6, msg);
            inputKey.trim();

            // Nếu người dùng bấm thoát (trả về ESC hoặc rỗng)
            if (inputKey == "\x1B" || inputKey == "") { return false; }

            // Kiểm tra Key
            if (inputKey == correctKey) {
                saveKey(inputKey);

                // Báo thành công
                tft.fillScreen(TFT_BLACK);
                tft.setTextColor(TFT_GREEN);
                tft.setTextSize(2);
                tft.drawCentreString("KICH HOAT THANH CONG!", tftWidth / 2, tftHeight / 2 - 10, 1);
                delay(1000);
                return true;
            } else {
                // Báo sai
                tft.fillScreen(TFT_BLACK);
                tft.setTextColor(TFT_RED);
                tft.setTextSize(2);
                tft.drawCentreString("SAI KEY!", tftWidth / 2, tftHeight / 2 - 20, 1);
                tft.setTextSize(1);
                tft.drawCentreString("Vui long tinh lai...", tftWidth / 2, tftHeight / 2 + 10, 1);
                delay(1500);
            }
        }
    }
};

extern SecurityManager security;
