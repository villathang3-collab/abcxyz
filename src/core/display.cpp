#include "display.h"
#include "Translation.h"
#include "Unifont10.h"
#include "Unifont12.h"
#include "Unifont16.h" // Cho Status Bar
#include "Unifont20.h" // Cho Option / File List
#include "Unifont28.h" // Cho Menu Chính             // Font to
#include "Unifont8.h"
#include "core/wifi/webInterface.h" // for server
#include "core/wifi/wg.h"           //for isConnectedWireguard to print wireguard lock
#include "mykeyboard.h"
#include "settings.h" //for timeStr
#include "utils.h"
#include <JPEGDecoder.h>
#include <interface.h> //for charging ischarging to print charging indicator
#define MAX_MENU_SIZE (int)(tftHeight / 25)

// Send the ST7789 into or out of sleep mode
void panelSleep(bool on) {
#if defined(ST7789_2_DRIVER) || defined(ST7789_DRIVER)
    if (on) {
        tft.writecommand(0x10); // SLPIN: panel off
        delay(5);
    } else {
        tft.writecommand(0x11); // SLPOUT: panel on
        delay(120);
    }
#endif
}

bool __attribute__((weak)) isCharging() { return false; }
// Hàm nạp font thông minh
void setFont(int type) {
    // Luôn set màu trước cho đồng bộ
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

// ============================================================
// TRƯỜNG HỢP 1: MÀN HÌNH NHỎ (<= 135x240) -> Dùng Unifont Custom
// ============================================================
#if TFT_WIDTH <= 135 && TFT_HEIGHT <= 240

    tft.unloadFont(); // Bắt buộc xả font cũ trước khi load mới

    switch (type) {
        case FP: tft.loadFont(Font8); break;  // FP = Font 8px
        case FM: tft.loadFont(Font16); break; // FM = Font 16px
        case FG: tft.loadFont(Font20); break; // FG = Font 20px
    }

    tft.setTextSize(1);
#endif
}
void displayScrollingText(const String &text, Opt_Coord &coord, int fontSize) {
    // [SỬA 1] Thêm dấu chấm phẩy và dùng biến trText
    String trText = Translation::tr(text);

    int clearHeight;
    int charWidth;
    int clearTopPad = 0;

    // --- LOGIC CŨ GIỮ NGUYÊN ---
    if (fontSize == 16 || coord.size > 18) {
        tft.loadFont(Font16);
        clearHeight = 20;
        charWidth = 9;
        clearTopPad = 4;
    } else {
        tft.loadFont(Font20);
        clearHeight = 28;
        charWidth = 12;
        clearTopPad = 5;
    }

    tft.setTextSize(1);
    tft.setTextColor(coord.fgcolor, coord.bgcolor);

    // [SỬA 2] Tính độ dài của chữ TIẾNG VIỆT (trText)
    int len = trText.length();

    // Nếu chữ ngắn hơn khung -> Không chạy cuộn
    if (len < coord.size) {
        // [SỬA 3] Phải vẽ chữ ra trước khi return, nếu không sẽ bị mất chữ!
        // tft.drawString(trText, coord.x, coord.y);
        tft.unloadFont();
        return;
    }

    // [SỬA 4] Dùng trText để tạo chuỗi cuộn
    String displayText = trText + "        ";
    int scrollLen = len + 8;
    static int i = 0;
    static long _lastmillis = 0;

    if (millis() > _lastmillis + 200) {
        String scrollingPart = displayText.substring(i, i + (coord.size - 1));

        int clearWidth = (coord.size - 1) * charWidth;

        // Xóa nền (Logic cũ giữ nguyên)
        tft.fillRect(coord.x, coord.y - clearTopPad, clearWidth, clearHeight + clearTopPad, coord.bgcolor);

        // Vẽ chữ
        tft.setCursor(coord.x, coord.y);
        tft.print(scrollingPart);

        // Vòng lặp
        if (i >= scrollLen - coord.size) i = -1;
        _lastmillis = millis();
        i++;
        if (i == 1) _lastmillis = millis() + 1000;
    }

    tft.unloadFont();
}


// ==============================================================================
// LOGIC XỬ LÝ FOOTER (THÔNG MINH)
// ==============================================================================

// ==============================================================================
// XỬ LÝ FOOTER (Font 20 - Luôn hiển thị để tránh lỗi Linker)
// ==============================================================================

void TouchFooter(uint16_t color) {
    // 1. Nạp Font 20
    tft.loadFont(Font20);
    tft.setTextSize(1);

    // 2. Xóa nền cũ (QUAN TRỌNG)
    // Xóa một vùng cao 50px bắt đầu từ tftHeight để làm sạch chữ cũ
    tft.fillRect(0, tftHeight, tftWidth, 50, bruceConfig.bgColor);

    // 3. Vẽ khung (Giữ nguyên tọa độ gốc của bạn: tftHeight + 2)
    tft.drawRoundRect(5, tftHeight + 2, tftWidth - 10, 43, 5, color);

    // 4. Vẽ chữ
    tft.setTextColor(color, bruceConfig.bgColor);

    // Lưu ý: Đã BỎ tham số ', 1' ở cuối hàm drawCentreString để nó nhận Font 20
    tft.drawCentreString("PREV", tftWidth / 6, tftHeight + 4);
    tft.drawCentreString("SEL", tftWidth / 2, tftHeight + 4);
    tft.drawCentreString("NEXT", 5 * tftWidth / 6, tftHeight + 4);
}

void MegaFooter(uint16_t color) {
    // 1. Nạp Font 20
    tft.loadFont(Font20);
    tft.setTextSize(1);

    // 2. Xóa nền cũ
    tft.fillRect(0, tftHeight, tftWidth, 50, bruceConfig.bgColor);

    // 3. Vẽ khung
    tft.drawRoundRect(5, tftHeight + 2, tftWidth - 10, 43, 5, color);

    // 4. Vẽ chữ
    tft.setTextColor(color, bruceConfig.bgColor);

    tft.drawCentreString("Exit", tftWidth / 6, tftHeight + 4);
    tft.drawCentreString("UP", tftWidth / 2, tftHeight + 4);
    tft.drawCentreString("DOWN", 5 * tftWidth / 6, tftHeight + 4);

    tft.unloadFont();
}
//*****************************************************************************************Function name
//: TouchFooter **Description : Draw touch screen footer ***************************************************
// ************************************ /

/***************************************************************************************
** Function name: resetTftDisplay
** Description:   set cursor to 0,0, screen and text to default color
***************************************************************************************/
void resetTftDisplay(int x, int y, uint16_t fc, int size, uint16_t bg, uint16_t screen) {
    tft.setCursor(x, y);
    tft.fillScreen(screen);
    tft.setTextSize(size);
    tft.setTextColor(fc, bg);
}

/***************************************************************************************
** Function name: setTftDisplay
** Description:   set cursor, font color, size and bg font color
***************************************************************************************/
void setTftDisplay(int x, int y, uint16_t fc, int size, uint16_t bg) {
    if (x >= 0 && y < 0) tft.setCursor(x, tft.getCursorY());      // if -1 on x, sets only y
    else if (x < 0 && y >= 0) tft.setCursor(tft.getCursorX(), y); // if -1 on y, sets only x
    else if (x >= 0 && y >= 0) tft.setCursor(x, y);               // if x and y > 0, sets both
    tft.setTextSize(size);
    tft.setTextColor(fc, bg);
}

void turnOffDisplay() { setBrightness(0, false); }

bool wakeUpScreen() {
    previousMillis = millis();
    if (isScreenOff) {
        isScreenOff = false;
        dimmer = false;
        getBrightness();
        vTaskDelay(pdMS_TO_TICKS(200));
        return true;
    } else if (dimmer) {
        dimmer = false;
        getBrightness();
        vTaskDelay(pdMS_TO_TICKS(200));
        return true;
    }
    return false;
}

/***************************************************************************************
** Function name: displayRedStripe
** Description:   Display Red Stripe with information
***************************************************************************************/
void displayRedStripe(String text, uint16_t fgcolor, uint16_t bgcolor) {
    // 1. Dịch văn bản
    String trText = Translation::tr(text);

    // Xử lý trường hợp màu chữ trùng màu nền (vd cùng trắng)
    if (fgcolor == bgcolor && fgcolor == TFT_WHITE) fgcolor = TFT_BLACK;

    // 2. NẠP FONT 16
    tft.loadFont(Font16);
    tft.setTextSize(1);

    // 3. Tính toán hộp
    int boxHeight = 32;           // Chiều cao hộp (Font16)
    int boxWidth = tftWidth - 20; // Cách lề 10px mỗi bên
    int x = 10;
    int y = (tftHeight - boxHeight) / 2; // Căn giữa màn hình theo chiều dọc

    // 4. Vẽ nền hộp
    tft.fillRoundRect(x, y, boxWidth, boxHeight, 7, bgcolor);

    // [MẸO] Vẽ thêm viền mỏng để nổi bật nếu nền hộp trùng màu nền menu
    tft.drawRoundRect(x, y, boxWidth, boxHeight, 7, fgcolor);

    // 5. Vẽ chữ (DÙNG BIẾN ĐÃ DỊCH trText)
    tft.setTextColor(fgcolor, bgcolor);
    // Canh giữa text vào hộp: y + nửa hộp - nửa font (8px)
    tft.drawCentreString(trText, tftWidth / 2, y + (boxHeight / 2) - 8, 1);

    // 6. Xả font
    tft.unloadFont();
}
void drawButton(
    int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, const char *text, bool inverted = false
) {
    if (inverted) {
        tft.fillRoundRect(x, y, w, h, 5, color);
    } else {
        tft.fillRoundRect(x, y, w, h, 5, TFT_BLACK);
        tft.drawRoundRect(x, y, w, h, 5, color);
    }
    tft.setTextColor(inverted ? TFT_BLACK : color);
    tft.drawString(text, x + w / 2, y + h);
}
int8_t displayMessage(
    const char *message, const char *leftButton, const char *centerButton, const char *rightButton,
    uint16_t color
) {
#ifdef HAS_SCREEN
    uint8_t oldTextDatum = tft.getTextDatum();
#endif

    // [THAY ĐỔI] Load font Unifont8 thay vì dùng Font mặc định + Size FM
    tft.loadFont(Font8);
    tft.setTextColor(color);
    tft.setTextSize(1); // Với Smooth Font (loadFont), luôn set size về 1

    tft.setTextDatum(TC_DATUM);
    // Vẽ thông báo chính
    tft.drawString(message, tftWidth / 2, tftHeight / 2 - 20);

    tft.setTextDatum(BC_DATUM);
    int16_t buttonHeight = 20;
    int16_t buttonY = tftHeight - buttonHeight - 5;
    int16_t buttonWidth = tftWidth / 3 - 10;

    int8_t totalButtons = (leftButton ? 1 : 0) + (centerButton ? 1 : 0) + (rightButton ? 1 : 0);
    int8_t selected = 0;
    bool redraw = true;

    while (true) {
        if (check(PrevPress) || check(EscPress)) {
            selected = (selected - 1 + totalButtons) % totalButtons;
            redraw = true;
        }
        if (check(NextPress)) {
            selected = (selected + 1) % totalButtons;
            redraw = true;
        }
        if (check(SelPress)) { break; }

        if (redraw) {
            int8_t index = 0;
            // Các nút bấm cũng sẽ dùng Font8 vì font vẫn đang được load
            if (leftButton) {
                drawButton(5, buttonY, buttonWidth, buttonHeight, color, leftButton, selected == index);
                index++;
            }

            if (centerButton) {
                drawButton(
                    tftWidth / 3 + 5,
                    buttonY,
                    buttonWidth,
                    buttonHeight,
                    color,
                    centerButton,
                    selected == index
                );
                index++;
            }

            if (rightButton) {
                drawButton(
                    tftWidth * 2 / 3 + 5,
                    buttonY,
                    buttonWidth,
                    buttonHeight,
                    color,
                    rightButton,
                    selected == index
                );
            }
            redraw = false;
        }

        delay(10);
    }

#ifdef HAS_SCREEN
    tft.setTextDatum(oldTextDatum);
#endif

    // [QUAN TRỌNG] Phải xả font để giải phóng RAM trước khi thoát hàm
    tft.unloadFont();

    return selected;
}

// 1. Hàm báo lỗi (Mặc định màu Đỏ)
void displayError(String txt, bool waitKeyPress) {
    // Dịch sang tiếng Việt trước khi hiển thị
    displayRedStripe(Translation::tr(txt));

#ifndef HAS_SCREEN
    Serial.println("ERR: " + txt); // Log giữ nguyên
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

// 2. Hàm cảnh báo (Màu Vàng)
void displayWarning(String txt, bool waitKeyPress) {
    // Dịch sang tiếng Việt, truyền màu Vàng/Đen
    displayRedStripe(Translation::tr(txt), TFT_BLACK, TFT_YELLOW);

#ifndef HAS_SCREEN
    Serial.println("WARN: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

// 3. Hàm thông tin (Màu Xanh Dương/Trắng)
void displayInfo(String txt, bool waitKeyPress) {
    // Đã xóa sạch các lệnh loadFont thừa, chỉ gọi hàm vẽ chuẩn
    displayRedStripe(Translation::tr(txt), TFT_WHITE, TFT_BLUE);

#ifndef HAS_SCREEN
    Serial.println("INFO: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

// 4. Hàm thành công (Màu Xanh Lá)
void displaySuccess(String txt, bool waitKeyPress) {
    displayRedStripe(Translation::tr(txt), TFT_WHITE, TFT_DARKGREEN);

#ifndef HAS_SCREEN
    Serial.println("SUCCESS: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

// 5. Hàm thông báo text theo Theme (Màu Tím/Màu chủ đạo)
void displayTextLine(String txt, bool waitKeyPress) {
    displayRedStripe(
        Translation::tr(txt), // <--- Quan trọng nhất
        getComplementaryColor2(bruceConfig.priColor),
        bruceConfig.priColor
    );

#ifndef HAS_SCREEN
    Serial.println("MESSAGE: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}
void setPadCursor(int16_t padx, int16_t pady) {
    for (int y = 0; y < pady; y++) tft.println();
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
}

// 1. Hàm in có định dạng (Có tham số padx)
void padprintf(int16_t padx, const char *format, ...) {
    char buffer[64];
    va_list args;
    va_start(args, format);

    // [THÊM] Dịch chuỗi định dạng trước khi điền số
    // Ví dụ: Code gọi padprintf(0, "Found %d networks", 5);
    // Nó sẽ tìm key "Found %d networks" trong lang_vi.h để dịch thành "Tìm thấy %d mạng"
    String trFormat = Translation::tr(format);

    // Dùng trFormat.c_str() thay vì format gốc
    vsnprintf(buffer, sizeof(buffer), trFormat.c_str(), args);
    va_end(args);

    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.printf("%s", buffer);
}

// 2. Hàm in có định dạng (Mặc định padx)
void padprintf(const char *format, ...) {
    char buffer[64];
    va_list args;
    va_start(args, format);

    // [THÊM] Dịch tương tự như trên
    String trFormat = Translation::tr(format);

    vsnprintf(buffer, sizeof(buffer), trFormat.c_str(), args);
    va_end(args);

    tft.setCursor(BORDER_PAD_X, tft.getCursorY());
    tft.printf("%s", buffer);
}

// 3. Hàm in chuỗi String
void padprint(const String &s, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    // [THÊM] Dịch chuỗi
    tft.print(Translation::tr(s));
}

// 4. Hàm in mảng ký tự (char array)
void padprint(const char str[], int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    // [THÊM] Dịch chuỗi
    tft.print(Translation::tr(str));
}

// ============================================================
// CÁC HÀM IN SỐ VÀ KÝ TỰ LẺ (GIỮ NGUYÊN - KHÔNG DỊCH)
// ============================================================

void padprint(char c, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(c);
}
void padprint(unsigned char b, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(b, base);
}
void padprint(int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(unsigned int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(unsigned long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(unsigned long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(double n, int digits, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, digits);
}
void padprintln(const String &s, int16_t padx) {
    // [BƯỚC 1] Dịch chuỗi sang Tiếng Việt ngay từ đầu
    // Tạo biến 'text' mới chứa nội dung đã dịch
    String text = Translation::tr(s);

    // [BƯỚC 2] Dùng biến 'text' để xử lý (Thay vì dùng 's' như cũ)
    if (text.isEmpty()) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(text);
        return;
    }

    String buff;
    size_t start = 0;
    int _maxCharsInLine = (tftWidth - (padx + 1) * BORDER_PAD_X) / (FP * LW);

    // [BƯỚC 3] Tự động cắt dòng dựa trên độ dài của TIẾNG VIỆT
    // Lưu ý: Đã đổi 's.substring' thành 'text.substring'
    while (!(buff = text.substring(start, start + _maxCharsInLine)).isEmpty()) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(buff);
        start += buff.length();
    }
}
void padprintln(const char str[], int16_t padx) {
    if (strcmp(str, "") == 0) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(str);
        return;
    }

    String buff;
    size_t start = 0;
    int _maxCharsInLine = (tftWidth - (padx + 1) * BORDER_PAD_X) / (FP * LW);

    // automatically split into multiple lines
    while (!(buff = String(str).substring(start, start + _maxCharsInLine)).isEmpty()) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(buff);
        start += buff.length();
    }
}
void padprintln(char c, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(c);
}
void padprintln(unsigned char b, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(b, base);
}
void padprintln(int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(unsigned int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(unsigned long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(unsigned long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(double n, int digits, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, digits);
}

// Hàm vẽ căn giữa (Nếu có rồi thì không cần copy lại)
// void printCentered(...) { ... }

void printCentered(const String &text, int y, uint16_t color) {
    tft.setTextColor(color);
    int w = tft.textWidth(text);
    tft.drawString(text, (tftWidth - w) / 2, y);
}

// Hàm này nằm trong display.cpp
// Hàm này nằm trong display.cpp
void drawStatusBar() {
    // ============================================================
    // [FIX LỖI ĐÈ FONT]
    // Bắt buộc tắt Font Custom và chuyển về Font hệ thống ngay đầu hàm
    // ============================================================
    tft.unloadFont();   // Gỡ bỏ Font20/Font16 nếu đang nạp
    tft.setTextFont(1); // Chọn Font hệ thống mặc định (GLCD)
    tft.setTextSize(1); // Reset kích thước về chuẩn
    // ============================================================

    int i = 0;
    uint8_t bat = getBattery();
    uint8_t bat_margin = 85;

    // Logic vẽ pin (Code cũ của bạn)
    if (bat > 0) {
        drawBatteryStatus(bat);
    } else bat_margin = 26;

    // Các icon khác
    if (sdcardMounted) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        // tft.setTextSize(FP); // <-- BỎ dòng này vì FP có thể là tham số font custom
        tft.setTextSize(1); // Dùng size 1 cho font hệ thống
        tft.drawString("SD", tftWidth - (bat_margin), 12);
        i++;
    }
    if (gpsConnected) {
        drawGpsSmall(tftWidth - (bat_margin + 23 * i), 7);
        i++;
    }
    if (WiFi.getMode()) {
        drawWifiSmall(tftWidth - (bat_margin + 23 * i), 7);
        i++;
    }
    if (isWebUIActive) {
        drawWebUISmall(tftWidth - (bat_margin + 23 * i), 7);
        i++;
    }
    if (BLEConnected) {
        drawBLESmall(tftWidth - (bat_margin + 23 * i), 7);
        i++;
    }
    if (isConnectedWireguard) {
        drawWireguardStatus(tftWidth - (bat_margin + 24 * i), 7);
        i++;
    }

    // Vẽ khung viền
    if (bruceConfig.theme.border) {
        tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
        tft.drawLine(5, 25, tftWidth - 6, 25, bruceConfig.priColor);
    }

    // Vẽ đồng hồ / Tiêu đề
    if (clock_set) {
        int clock_fontsize = 1;

        // Cấu hình hiển thị cho Font hệ thống
        setTftDisplay(12, 12, bruceConfig.priColor, clock_fontsize, bruceConfig.bgColor);

#if defined(HAS_RTC)
        _rtc.GetTime(&_time);
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", _time.Hours, _time.Minutes);

        // Xóa nền kỹ trước khi vẽ giờ để tránh đè số
        // (Dùng font hệ thống size 1 thì chiều cao khoảng 8px, ta xóa 12px cho chắc)
        tft.fillRect(12, 12, 60, 12, bruceConfig.bgColor);
        tft.print(timeStr);
#else
        updateTimeStr(rtc.getTimeStruct());
        // LH là chiều cao dòng, với font hệ thống thì ta tự ước lượng hoặc dùng 15px
        tft.fillRect(12, 12, 100, 15, bruceConfig.bgColor);
        tft.print(timeStr);
#endif
    } else {
        setTftDisplay(12, 12, bruceConfig.priColor, 1, bruceConfig.bgColor);
        tft.print("BRUCE " + String(BRUCE_VERSION));
    }
}

/*********************************************************************
** Function: loopOptions
** Where you choose among the options in menu
**********************************************************************/
int loopOptions(
    std::vector<Option> &options, uint8_t menuType, const char *subText, int index, bool interpreter
) {
    // [THÊM] Dịch tiêu đề menu ngay khi vào hàm
    String trSubText = Translation::tr(String(subText));

    Opt_Coord coord;
    bool redraw = true;
    bool exit = false;
    int menuSize = options.size();
    int devModeCounter = 0;
    static unsigned long _clock_bat_timer = millis();
    if (options.size() > MAX_MENU_SIZE) { menuSize = MAX_MENU_SIZE; }

    // Chỉ vẽ nền xám cho menu danh sách (Submenu)
    if (index > 0 && menuType != MENU_TYPE_MAIN)
        tft.fillRoundRect(
            tftWidth * 0.10,
            tftHeight / 2 - menuSize * (FM * 8 + 4) / 2 - 5,
            tftWidth * 0.8,
            (FM * 8 + 4) * menuSize + 10,
            5,
            bruceConfig.bgColor
        );

    if (index >= options.size()) index = 0;
    bool firstRender = true;

    if (menuType == MENU_TYPE_MAIN) drawMainBorder(true);
    else drawMainBorder();

    while (1) {
        if (exit) break;
        if (menuType == MENU_TYPE_MAIN) {
            checkReboot();
            if (devModeCounter >= 5 && !bruceConfig.devMode) {
                bruceConfig.setDevMode(true);
                displayInfo("Dev Mode Enabled", true);
            }
            if (millis() - _clock_bat_timer > 30000) {
                _clock_bat_timer = millis();
                drawStatusBar();
            }
        }

        if (redraw) {
            menuOptionType = menuType;
            menuOptionLabel = subText;
            for (auto &opt : options) opt.hovered = false;
            options[index].hovered = true;

            // =========================================================
            // [FIX 13.0 - MAX DEPTH CLEAR - XÓA SÁT SÀN]
            // =========================================================
            if (menuType == MENU_TYPE_MAIN) {
                // 1. Điểm bắt đầu: Từ dưới chân Icon
                int startY = (tftHeight / 2) + 30;

                // 2. Điểm kết thúc: SÁT VIỀN KHUNG (Bottom Border)
                // Viền khung nằm ở (tftHeight - 5). Ta xóa đến (tftHeight - 6).
                // Đây là mức sâu nhất có thể. Nếu vẫn còn sót thì chữ đang vẽ đè lên cả viền.
                int endY = tftHeight - 6;

                int h = endY - startY;

                if (h > 0) {
                    // Xóa sạch nền
                    // Chừa lề trái phải 20px
                    tft.fillRect(20, startY, tftWidth - 40, h, bruceConfig.bgColor);
                }

                // [NỖ LỰC ĐỔI MÀU CHỮ GỐC]
                // Ép màu tím trước khi gọi hàm vẽ của hệ thống
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            }
            // =========================================================

            // GỌI HÀM VẼ GỐC
            bool renderedByLambda = false;
            if (options[index].hover)
                renderedByLambda = options[index].hover(options[index].hoverPointer, true);

            if (!renderedByLambda) {
                // [SỬA] Truyền biến trSubText (đã dịch) thay vì subText gốc
                if (menuType == MENU_TYPE_SUBMENU) drawSubmenu(index, options, trSubText.c_str());
                else
                    coord = drawOptions(
                        index,
                        options,
                        bruceConfig.priColor,
                        bruceConfig.secColor,
                        bruceConfig.bgColor,
                        firstRender
                    );
            }
            firstRender = false;
            redraw = false;
        }

#ifdef HAS_KEYBOARD
        checkShortcutPress();
#endif

        if (menuType == MENU_TYPE_REGULAR) {
            // [SỬA] Dịch nội dung hiển thị cho menu Regular
            String txt = Translation::tr(options[index].label);
            displayScrollingText(txt, coord);
        }

        if (PrevPress || check(UpPress)) {
            devModeCounter = 0;
#ifdef HAS_KEYBOARD
            check(PrevPress);
            if (index == 0) index = options.size() - 1;
            else if (index > 0) index--;
            redraw = true;
#else
            long _tmp = millis();
#ifndef HAS_ENCODER
            LongPress = true;
            while (PrevPress && menuType != MENU_TYPE_MAIN) {
                if (millis() - _tmp > 200)
                    tft.drawArc(
                        tftWidth / 2,
                        tftHeight / 2,
                        25,
                        15,
                        0,
                        360 * (millis() - (_tmp + 200)) / 500,
                        getColorVariation(bruceConfig.priColor),
                        bruceConfig.bgColor
                    );
                vTaskDelay(10 / portTICK_RATE_MS);
            }
            if (menuType != MENU_TYPE_MAIN)
                tft.drawArc(
                    tftWidth / 2, tftHeight / 2, 25, 15, 0, 360, bruceConfig.bgColor, bruceConfig.bgColor
                );

            LongPress = false;
#endif
            if (millis() - _tmp > 700) {
                index = -1;
                break;
            } else {
                check(PrevPress);
                if (index == 0) index = options.size() - 1;
                else if (index > 0) index--;
                redraw = true;
            }
#endif
        }

        if (check(NextPress) || check(DownPress)) {
            index++;
            if ((index + 1) > options.size()) {
                if (!bruceConfig.devMode) devModeCounter++;
                index = 0;
            }
            redraw = true;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);

        if (check(SelPress) || forceMenuOption >= 0) {
            uint16_t chosen = index;
            if (forceMenuOption >= 0) {
                chosen = forceMenuOption;
                forceMenuOption = -1;
                Serial.print("Forcely ");
            }
            Serial.println("Selected: " + String(options[chosen].label));
            options[chosen].operation();
            break;
        }

        if (interpreter_start && !interpreter) { break; }

#ifdef HAS_KEYBOARD
        if (check(EscPress)) {
            index = -1;
            break;
        }
#elif defined(T_EMBED) || defined(HAS_TOUCH) || !defined(HAS_SCREEN)
        if (menuType != MENU_TYPE_MAIN && check(EscPress)) break;
#endif
    }
    return index;
}
/********************************************************************************
** Function name: progressHandler
** Description:   Função para manipular o progresso da atualização
** Dependencia: prog_handler =>>    0 - Flash, 1 - LittleFS
***************************************************************************************/
void progressHandler(int progress, size_t total, String message) {
    int barWidth = map(progress, 0, total, 0, tftWidth - 40);
    if (barWidth < 3) {
        tft.fillRect(6, 27, tftWidth - 12, tftHeight - 33, bruceConfig.bgColor);
        tft.drawRect(18, tftHeight - 47, tftWidth - 36, 17, bruceConfig.priColor);
        displayRedStripe(message, TFT_WHITE, bruceConfig.priColor);
    }
    tft.fillRect(20, tftHeight - 45, barWidth, 13, bruceConfig.priColor);
}

/***************************************************************************************
** Function name: drawOptions
** Description:   Função para desenhar e mostrar as opçoes de contexto
***************************************************************************************/
// Hàm vẽ căn giữa (nếu chưa có thì để nguyên, nếu có rồi thì dùng lại hàm cũ)
// (Lưu ý: Nếu code báo lỗi 'printCentered' undefined thì copy hàm đó đặt lên trên cùng file)

Opt_Coord drawOptions(
    int index, std::vector<Option> &options, uint16_t fgcolor, uint16_t selcolor, uint16_t bgcolor,
    bool firstRender
) {
    // 1. NẠP FONT 20
    tft.loadFont(Font20);
    tft.setTextSize(1);

    Opt_Coord coord;
    int menuSize = options.size();
    if (options.size() > MAX_MENU_SIZE) { menuSize = MAX_MENU_SIZE; }

    // ================== CẤU HÌNH CHO FONT 20 ==================
    int ROW_HEIGHT = 28;
    int CHAR_WIDTH = 12;
    // ==========================================================

    // Tính toán vị trí Y bắt đầu của Menu
    int32_t optionsTopY = tftHeight / 2 - menuSize * ROW_HEIGHT / 2 - 5;

    tft.drawPixel(0, 0, bruceConfig.bgColor);

    if (firstRender) {
        // Vẽ nền khung bao quanh
        tft.fillRoundRect(
            tftWidth * 0.10, optionsTopY, tftWidth * 0.8, ROW_HEIGHT * menuSize + 10, 5, bgcolor
        );
        // Vẽ viền khung
        tft.drawRoundRect(
            tftWidth * 0.10, optionsTopY, tftWidth * 0.8, ROW_HEIGHT * menuSize + 10, 5, fgcolor
        );
    }

    tft.setTextColor(fgcolor, bgcolor);

    int i = 0;
    int init = 0;
    int cont = 1;

    // Logic cuộn menu (Pagination)
    if (index >= MAX_MENU_SIZE) init = index - MAX_MENU_SIZE + 1;

    for (i = 0; i < (int)options.size(); i++) {
        if (i >= init) {
            // Tính tọa độ Y của dòng hiện tại
            int currentY = optionsTopY + 5 + (cont - 1) * ROW_HEIGHT;

            // --- [GIỮ NGUYÊN] MỞ RỘNG VÙNG XÓA NỀN ---
            // Dịch lên 2px (currentY - 2)
            // Tăng chiều cao thêm 5px (ROW_HEIGHT + 5) để bù cho 2px trên và thêm 3px dưới
            tft.fillRect(tftWidth * 0.10 + 2, currentY - 2, tftWidth * 0.8 - 4, ROW_HEIGHT + 5, bgcolor);

            if (options[i].selected) tft.setTextColor(selcolor, bgcolor);
            else tft.setTextColor(fgcolor, bgcolor);

            String text = "";
            if (i == index) {
                text += ">";
                // Cập nhật tọa độ cho hiệu ứng chữ chạy
                coord.x = tftWidth * 0.10 + 5 + CHAR_WIDTH;
                coord.y = currentY + 4; // Căn chỉnh Y
                coord.size = (tftWidth * 0.8 - 10) / CHAR_WIDTH - 1;
                coord.fgcolor = fgcolor;
                coord.bgcolor = bgcolor;
            } else text += " ";

            // [SỬA QUAN TRỌNG] Dịch tên menu trước khi hiển thị
            String translatedLabel = Translation::tr(options[i].label);

            // Thêm khoảng trắng để xóa dư thừa
            text += translatedLabel + "              ";

            // Đặt con trỏ và vẽ chữ
            tft.setCursor(tftWidth * 0.10 + 5, currentY + 4);
            tft.println(text.substring(0, (tftWidth * 0.8 - 10) / CHAR_WIDTH - 1));

            cont++;
        }
        if (cont > MAX_MENU_SIZE) goto Exit;
    }
Exit:
    if (options.size() > MAX_MENU_SIZE) menuSize = MAX_MENU_SIZE;

#if defined(HAS_TOUCH)
    TouchFooter(bruceConfig.secColor);
#endif

    tft.unloadFont();
    return coord;
}
// Đảm bảo bạn đã include font
// #include "Unifont16.h"
// #include "Unifont20.h" (hoặc tên file font của bạn)

void drawSubmenu(int index, std::vector<Option> &options, const char *title) {
    drawStatusBar(); // Vẽ status bar

    // ================== CẤU HÌNH AN TOÀN ==================
    int SAFE_X = 6;
    int SAFE_WIDTH = tftWidth - 12;
    int LINE_HEIGHT = 30;
    int menuSize = options.size();

    // 2. TÍNH TOÁN TỌA ĐỘ
    int middle = 25 + (tftHeight - 30) / 2;
    int step = (tftHeight - 42) / 3;

    int middle_up = middle - step + 4;
    int middle_down = middle + step;

    // ================== BƯỚC 1: VẼ CÁC MỤC MENU (LAYER DƯỚI) ==================
    tft.loadFont(Font20);
    tft.setTextSize(1);
    tft.setTextDatum(TC_DATUM); // Căn Giữa - Trên

    // --- MỤC TRÊN (Previous) ---
    String prevLabel = index - 1 >= 0 ? options[index - 1].label : options[menuSize - 1].label;
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);

    // [MỤC TRÊN] Giữ nguyên fix ghosting 4px
    tft.fillRect(SAFE_X, middle_up - 15 - 4, SAFE_WIDTH, LINE_HEIGHT + 14, bruceConfig.bgColor);
    tft.drawString(Translation::tr(prevLabel), tftWidth / 2, middle_up - 10);

    // --- MỤC ĐANG CHỌN (Selected) ---
    String trCurrent = Translation::tr(options[index].label);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    // [FIX MỤC GIỮA CẮT TRÊN]
    // Y cũ: middle - LINE_HEIGHT/2 - 5 - 4
    // Y mới: Cộng thêm 5px vào Y (dịch xuống) -> Bỏ số 5 đi
    // Công thức mới: middle - LINE_HEIGHT/2 - 4
    tft.fillRect(SAFE_X, middle - LINE_HEIGHT / 2 - 4, SAFE_WIDTH, LINE_HEIGHT + 14, bruceConfig.bgColor);

    tft.drawString(trCurrent, tftWidth / 2, middle - 12);

    // [GẠCH CHÂN]
    int textWidth = tft.textWidth(trCurrent);
    tft.drawFastHLine(tftWidth / 2 - textWidth / 2, middle + 15, textWidth, bruceConfig.priColor);

    // --- MỤC DƯỚI (Next) ---
    String nextLabel = index + 1 < menuSize ? options[index + 1].label : options[0].label;
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);

    // [FIX MỤC CUỐI CẮT LỀ]
    // Chiều cao cũ: LINE_HEIGHT + 10 + 4 (= LINE_HEIGHT + 14)
    // Chiều cao mới: Giảm 10px -> Còn LINE_HEIGHT + 4
    tft.fillRect(SAFE_X, middle_down - 15 - 4, SAFE_WIDTH, LINE_HEIGHT + 4, bruceConfig.bgColor);

    tft.drawString(Translation::tr(nextLabel), tftWidth / 2, middle_down - 10);

    tft.unloadFont();

    // ================== BƯỚC 2: VẼ TIÊU ĐỀ (LAYER TRÊN CÙNG) ==================
    tft.loadFont(Font16);
    int fontHeight = tft.fontHeight();

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM); // Căn Trái - Trên

    // [FIX TIÊU ĐỀ] (Giữ nguyên cấu hình cũ bạn đã ok)
    // Thu hẹp Y trên dưới 4px, giữ lề phải 6px
    tft.fillRect(12, 30 + 4, tftWidth - 18, fontHeight - 8, bruceConfig.bgColor);
    tft.drawString(Translation::tr(title), 12, 30);

    tft.unloadFont();

    // ================== BƯỚC 3: THANH CUỘN & FOOTER ==================
    tft.fillRect(tftWidth - 5, 0, 5, tftHeight, bruceConfig.bgColor);
    tft.fillRect(tftWidth - 5, index * tftHeight / menuSize, 5, tftHeight / menuSize, bruceConfig.priColor);

#if defined(HAS_TOUCH)
    tft.loadFont(Font8);
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("[ x ]", 7, 7, 1);
    TouchFooter(bruceConfig.secColor);
    tft.unloadFont();
#endif

    tft.setTextDatum(TL_DATUM);
}
void drawMainBorder(bool clear) {
    if (clear) {
        tft.drawPixel(0, 0, 0);
        tft.fillScreen(bruceConfig.bgColor);
    }
    setTftDisplay(12, 12, bruceConfig.priColor, 1, bruceConfig.bgColor);
    tft.setTextDatum(TL_DATUM); // TL_DATUM tương ứng với 0

    // if(wifiConnected) {tft.print(timeStr);} else {tft.print("BRUCE 1.0b");}

    drawStatusBar();

#if defined(HAS_TOUCH)
    // Nếu code gốc của bạn là TouchFooter() không có tham số thì xóa phần trong ngoặc đi
    // Nhưng dựa vào context màu sắc, có thể nó là:
    TouchFooter(bruceConfig.secColor);
#endif
}
void drawMainBorderWithTitle(String title, bool clear) {
    drawMainBorder(clear);

    // [CẬP NHẬT] Nạp Font 20 để đồng bộ với Menu/Submenu
    tft.loadFont(Font20);
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    // Dịch tiêu đề sang Tiếng Việt và viết hoa
    String trTitle = Translation::tr(title);
    trTitle.toUpperCase();

    // Vẽ căn giữa màn hình
    // Y = 30 là vị trí an toàn nằm dưới thanh trạng thái (Status Bar)
    tft.drawCentreString(trTitle, tftWidth / 2, BORDER_PAD_Y);

    // Xả font để tránh lỗi các hàm sau
    tft.unloadFont();
}

void printTitle(String title) {
    tft.setCursor((tftWidth - (title.length() * FM * LW)) / 2, BORDER_PAD_Y);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FM);

    title.toUpperCase();
    tft.println(title);

    tft.setTextSize(FP);
}

void printSubtitle(String subtitle, bool withLine) {
    int16_t cursorX = (tftWidth - (subtitle.length() * FP * LW)) / 2;
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);

    tft.setCursor(cursorX, BORDER_PAD_Y + FM * LH);
    tft.println(subtitle);

    if (withLine) {
        String line = "";
        for (byte i = 0; i < subtitle.length(); i++) line += "-";

        tft.setCursor(cursorX, tft.getCursorY());
        tft.println(line);
    }
}

void printFootnote(String text) {
    tft.setTextSize(FP);
    tft.drawRightString(text, tftWidth - BORDER_PAD_X, tftHeight - BORDER_PAD_X - FP * LH, SMOOTH_FONT);
}

void printCenterFootnote(String text) {
    tft.fillRect(10, tftHeight - BORDER_PAD_X - FP * LH, tftWidth - 20, FP * LH, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawCentreString(text, tftWidth / 2, tftHeight - BORDER_PAD_X - FP * LH, SMOOTH_FONT);
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int percent = 0;

    return (percent < 0) ? 0 : (percent >= 100) ? 100 : percent;
}

/***************************************************************************************
** Function name: drawBatteryStatus()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
void drawBatteryStatus(uint8_t bat) {
    // --- QUAY VỀ FONT GỐC ---
    tft.unloadFont();
    // ------------------------

    if (bat == 0) return;

    bool charging = isCharging();

    uint16_t color = bruceConfig.priColor;
    uint16_t barcolor = bruceConfig.priColor;
    if (bat < 16) barcolor = color = TFT_RED;
    else if (bat < 34) barcolor = color = TFT_YELLOW;
    if (charging) color = TFT_GREEN;

    tft.drawRoundRect(tftWidth - 43, 6, 36, 19, 2, charging ? color : bruceConfig.bgColor);
    tft.drawRoundRect(tftWidth - 42, 7, 34, 17, 2, color);

    tft.setTextSize(1); // Font gốc size 1 là chuẩn đẹp
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawRightString((bat == 100 ? "" : " ") + String(bat) + "%", tftWidth - 45, 12, 1);
    tft.fillRoundRect(tftWidth - 40, 9, 30 * bat / 100, 13, 2, barcolor);
    tft.drawLine(tftWidth - 30, 9, tftWidth - 30, 9 + 13, bruceConfig.bgColor);
    tft.drawLine(tftWidth - 20, 9, tftWidth - 20, 9 + 13, bruceConfig.bgColor);
}
/***************************************************************************************
** Function name: drawWireguardStatus()
** Description:   Draws a padlock when connected
***************************************************************************************/
void drawWireguardStatus(int x, int y) {
    tft.fillRect(x, y, 20, 17, bruceConfig.bgColor);
    if (isConnectedWireguard) {
        tft.drawRoundRect(11 + x, 0 + y, 8, 12, 5, TFT_GREEN);
        tft.fillRoundRect(10 + x, 8 + y, 10, 8, 0, TFT_GREEN);
    } else {
        tft.drawRoundRect(1 + x, 0 + y, 8, 12, 5, bruceConfig.priColor);
        tft.fillRoundRect(0 + x, 8 + y, 10, 8, 0, bruceConfig.bgColor);
        tft.fillRoundRect(6 + x, 8 + y, 10, 10, 0, bruceConfig.priColor);
    }
}

/***************************************************************************************
** Function name: listFiles
** Description:   Função para desenhar e mostrar o menu principal
***************************************************************************************/
#define MAX_ITEMS (int)(tftHeight - 20) / (LH * FM)
/***************************************************************************************
** Function name: listFiles
** Description:   Vẽ danh sách file (Đã nâng cấp Font 20px + Fix lỗi rách chữ)
***************************************************************************************/
// Hàm này nằm trong display.cpp
Opt_Coord listFiles(int index, std::vector<FileList> fileList) {
    // 1. VẼ STATUS BAR
    drawStatusBar();

    tft.loadFont(Font16);
    tft.setTextSize(1);

    // --- CẤU HÌNH ---
    int ROW_HEIGHT = 25;
    int HEADER_OFFSET = 35;
    int CHAR_WIDTH = 9;

    Opt_Coord coord;

    if (index == 0) {
        tft.fillRect(0, 20, tftWidth, tftHeight - 20, bruceConfig.bgColor);
        tft.drawRoundRect(5, 25, tftWidth - 10, tftHeight - 30, 5, bruceConfig.priColor);
    }

    int maxItemsPerScreen = (tftHeight - HEADER_OFFSET - 5) / ROW_HEIGHT;
    int i = 0;
    int arraySize = fileList.size();

    int start = 0;
    if (index >= maxItemsPerScreen) {
        start = index - maxItemsPerScreen + 1;
        if (start < 0) start = 0;
    }

    int nchars = (tftWidth - 25) / CHAR_WIDTH;

    int cont = 0;
    while (i < arraySize) {
        if (i >= start) {
            int currentY = HEADER_OFFSET + (cont * ROW_HEIGHT);

            if (currentY + ROW_HEIGHT > tftHeight - 5) break;

            // BƯỚC 1: Xóa nền cơ bản cho toàn bộ dòng (Quan trọng)
            // Xóa từ x=10 đến hết chiều rộng, cao đúng bằng ROW_HEIGHT
            tft.fillRect(10, currentY, tftWidth - 20, ROW_HEIGHT, bruceConfig.bgColor);

            // Thiết lập màu sắc
            if (fileList[i].folder == true)
                tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
            else if (fileList[i].operation == true) tft.setTextColor(ALCOLOR, bruceConfig.bgColor);
            else tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

            // Vẽ con trỏ >
            if (index == i) {
                tft.drawString(">", 12, currentY + 4);
                coord.x = 25;
                coord.y = currentY + 4;
                coord.size = nchars;
                coord.fgcolor =
                    fileList[i].folder ? getColorVariation(bruceConfig.priColor) : bruceConfig.priColor;
                coord.bgcolor = bruceConfig.bgColor;
            }

            String displayName = fileList[i].filename;

            // [FIX LỖI NÚT BACK KHÔNG XÓA ĐƯỢC]
            if (displayName == "Back") {
                // Biện pháp mạnh: Vẽ đè một hình chữ nhật màu nền lên vị trí chữ sắp hiện
                // Để đảm bảo không còn bóng dáng chữ "Back" hay chữ cũ nào sót lại.

                // X = 24 (Sau dấu mũi tên > một chút)
                // Y = currentY (Bằng đúng dòng hiện tại)
                // W = tftWidth - 30 (Rộng hết phần còn lại màn hình)
                // H = ROW_HEIGHT (Cao bằng dòng)
                tft.fillRect(24, currentY, tftWidth - 30, ROW_HEIGHT, bruceConfig.bgColor);

                // Sau khi xóa sạch sẽ thì mới lấy chữ dịch
                displayName = Translation::tr("Back");
            }

            // Cắt chuỗi nếu dài quá
            if (displayName.length() > nchars) { displayName = displayName.substring(0, nchars); }

            // Vẽ chữ mới lên nền đã xóa sạch
            tft.drawString(displayName, 25, currentY + 4);
            cont++;
        }
        i++;
    }

    tft.unloadFont();
    return coord;
}
void drawWifiSmall(int x, int y) {
    tft.fillRect(x, y, 16, 16, bruceConfig.bgColor);
    tft.fillCircle(9 + x, 14 + y, 1, bruceConfig.priColor);
    tft.drawArc(9 + x, 14 + y, 4, 6, 130, 230, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawArc(9 + x, 14 + y, 10, 12, 130, 230, bruceConfig.priColor, bruceConfig.bgColor);
}

void drawWebUISmall(int x, int y) {
    tft.fillRect(x, y, 16, 16, bruceConfig.bgColor);

    tft.drawCircle(8 + x, 8 + y, 7, bruceConfig.priColor);

    tft.drawLine(3 + x, 4 + y, 14 + x, 4 + y, bruceConfig.priColor);
    tft.drawLine(2 + x, 8 + y, 15 + x, 8 + y, bruceConfig.priColor);
    tft.drawLine(3 + x, 12 + y, 14 + x, 12 + y, bruceConfig.priColor);
}

void drawBLESmall(int x, int y) {
    tft.fillRect(x, 2 + y, 17, 13, bruceConfig.bgColor);
    tft.drawWideLine(8 + x, 8 + y, 4 + x, 5 + y, 2, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawWideLine(8 + x, 8 + y, 4 + x, 13 + y, 2, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawTriangle(8 + x, 8 + y, 8 + x, 2 + y, 13 + x, 5 + y, bruceConfig.priColor);
    tft.drawTriangle(8 + x, 8 + y, 8 + x, 14 + y, 13 + x, 11 + y, bruceConfig.priColor);
}

void drawBLE_beacon(int x, int y, uint16_t color) {
    tft.fillRect(x, y, 40, 80, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 53 + y, 2 + x, 26 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 26 + y, 2 + x, 53 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 53 + y, 20 + x, 68 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 26 + y, 20 + x, 12 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(20 + x, 12 + y, 20 + x, 68 + y, 5, color, bruceConfig.bgColor);
    tft.fillTriangle(40 + x, 26 + y, 20 + x, 40 + y, 20 + x, 12 + y, color);
    tft.fillTriangle(40 + x, 53 + y, 20 + x, 40 + y, 20 + x, 68 + y, color);
}

void drawGPS(int x, int y) {
    tft.fillRect(x, y, 80, 80, bruceConfig.bgColor);
    tft.drawEllipse(40 + x, 70 + y, 15, 8, bruceConfig.priColor);
    tft.drawArc(40 + x, 25 + y, 23, 7, 0, 340, bruceConfig.priColor, bruceConfig.bgColor);
    tft.fillTriangle(40 + x, 70 + y, 20 + x, 64 + y, 60 + x, 64 + y, bruceConfig.priColor);
}

void drawGpsSmall(int x, int y) {
    tft.fillRect(x, y, 17, 17, bruceConfig.bgColor);
    tft.drawEllipse(9 + x, 14 + y, 4, 3, bruceConfig.priColor);
    tft.drawArc(9 + x, 6 + y, 5, 2, 0, 340, bruceConfig.priColor, bruceConfig.bgColor);
    tft.fillTriangle(9 + x, 15 + y, 5 + x, 9 + y, 13 + x, 9 + y, bruceConfig.priColor);
}

void drawCreditCard(int x, int y) {
    tft.fillRect(x, y, 70, 50, bruceConfig.bgColor);
    tft.fillRoundRect(x + 5, y + 5, 60, 40, 5, bruceConfig.priColor);
    tft.fillRect(x + 5, y + 15, 60, 10, getColorVariation(bruceConfig.priColor, 3, -1));
    tft.fillRect(x + 10, y + 30, 12, 10, getColorVariation(bruceConfig.priColor, 3, 1));
    tft.drawRect(x + 10, y + 30, 12, 10, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.drawRect(x + 10 + 4, y + 30, 4, 10, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.drawRect(x + 10, y + 33, 5, 4, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.drawRect(x + 17, y + 33, 5, 4, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.fillRect(x + 30, y + 35, 30, 5, getColorVariation(bruceConfig.priColor, 5, 1));
}

void drawMfkey32Icon(int x, int y) {
    tft.drawRect(x + 2, y + 15, 24, 40, bruceConfig.priColor);
    tft.drawRect(x + 5, y + 18, 18, 12, bruceConfig.priColor);
    tft.drawRect(x + 5, y + 34, 18, 18, bruceConfig.priColor);
    tft.drawLine(x + 5, y + 40, x + 22, y + 40, bruceConfig.priColor);
    tft.drawLine(x + 5, y + 46, x + 22, y + 46, bruceConfig.priColor);
    tft.drawLine(x + 11, y + 34, x + 11, y + 51, bruceConfig.priColor);
    tft.drawLine(x + 17, y + 34, x + 17, y + 51, bruceConfig.priColor);
    tft.drawRect(x + 30, y + 10, 25, 35, bruceConfig.priColor);
    int startX = x + 32;
    int startY = y + 12;
    int endX = x + 52;
    int endY = y + 32;
    int step = 2;
    int turns = 0;

    while (startX <= endX && startY <= endY && turns < 3) {
        for (int i = startX; i <= endX; i++) { tft.drawPixel(i, startY, bruceConfig.priColor); }
        startY += step;
        for (int i = startY; i <= endY; i++) { tft.drawPixel(endX, i, bruceConfig.priColor); }
        endX -= step;
        for (int i = endX; i >= startX; i--) { tft.drawPixel(i, endY, bruceConfig.priColor); }
        endY -= step;
        for (int i = endY; i >= startY; i--) { tft.drawPixel(startX, i, bruceConfig.priColor); }
        startX += step;
        turns++;
    }
    tft.fillRect(x + 40, y + 36, 6, 6, getColorVariation(bruceConfig.priColor, 3, 1));
}

void drawMfkey64Icon(int x, int y) {
    drawMfkey32Icon(x, y);
    tft.fillRoundRect(x + 40, y + 6, 24, 14, 4, bruceConfig.bgColor);
    tft.drawRoundRect(x + 40, y + 6, 24, 14, 4, getColorVariation(bruceConfig.priColor, 3, -1));
    tft.drawCircle(x + 48, y + 12, 4, getColorVariation(bruceConfig.priColor, 3, -1));
}

// ####################################################################################################
//  Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
// ####################################################################################################
//  from:
//  https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Generic/ESP32_SDcard_jpeg/ESP32_SDcard_jpeg.ino
//  This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
//  fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {

    // jpegInfo(); // Print information from the JPEG file (could comment this line out)

    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    bool swapBytes = tft.getSwapBytes();
    tft.setSwapBytes(true);

    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
    uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

    // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;

    // Fetch data from the file, decode and display
    tft.fillRect(xpos, ypos, JpegDec.width, JpegDec.height, TFT_BLACK);
    while (JpegDec.read()) {   // While there is more data in the file
        pImg = JpegDec.pImage; // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

        // Calculate coordinates of top left corner of current MCU
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // check if the image block size needs to be changed for the right edge
        if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
        else win_w = min_w;

        // check if the image block size needs to be changed for the bottom edge
        if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
        else win_h = min_h;

        // copy pixels into a contiguous block
        if (win_w != mcu_w) {
            uint16_t *cImg;
            int p = 0;
            cImg = pImg + win_w;
            for (int h = 1; h < win_h; h++) {
                p += mcu_w;
                for (int w = 0; w < win_w; w++) {
                    *cImg = *(pImg + w + p);
                    cImg++;
                }
            }
        }

        // calculate how many pixels must be drawn
        uint32_t mcu_pixels = win_w * win_h;

        // draw image MCU block only if it will fit on the screen
        if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
            tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
        else if ((mcu_y + win_h) > tft.height())
            JpegDec.abort(); // Image has run off bottom of screen so abort decoding
    }

    tft.setSwapBytes(swapBytes);
}

bool showJpeg(FS &fs, String filename, int x, int y, bool center) {
    // record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();
    File picture;
    if (fs.exists(filename)) picture = fs.open(filename, FILE_READ);
    else return false;

    const size_t data_size = picture.size();

    // Alloc memory into heap
    uint8_t *data_array = new uint8_t[data_size];
    if (data_array == nullptr) {
        // Fail allocating memory
        picture.close();
        delete[] data_array;
        return false;
    }

    uint8_t data;
    int i = 0;
    byte line_len = 0;

    while (picture.available()) {
        data = picture.read();
        data_array[i] = data;
        i++;

        // print array on Serial
        /*
        Serial.print("0x");
        if (abs(data) < 16) {
          Serial.print("0");
        }

        Serial.print(data, HEX);
        Serial.print(","); // Add value and comma
        line_len++;
        if (line_len >= 32) {
          line_len = 0;
          Serial.println();
        }
        */
    }

    picture.close();

    bool decoded = false;
    if (data_array) {
        decoded = JpegDec.decodeArray(data_array, data_size);
    } else {
        displayError(filename + " Fail");
        delay(2500);
        delete[] data_array; // free heap before leaving
        return false;
    }

    if (decoded) {
        if (center) {
            x = x + (tftWidth - JpegDec.width) / 2;
            y = y + (tftHeight - JpegDec.height) / 2;
        }
        jpegRender(x, y);
    }
    // calculate how long it took to draw the image
    drawTime = millis() - drawTime; // Calculate the time it took

    // print the results to the serial port
    Serial.print("Total render time was    : ");
    Serial.print(drawTime);
    Serial.println(" ms");
    Serial.println("=====================================");

    delete[] data_array; // free heap before leaving
    return true;
}

#if !defined(LITE_VERSION)
// ####################################################################################################
//  Draw a GIF on the TFT
//  derived from
//  https://github.com/bitbank2/AnimatedGIF/blob/master/examples/TFT_eSPI_memory/TFT_eSPI_memory.ino and
//  https://github.com/bitbank2/AnimatedGIF/blob/master/examples/best_practices_example/best_practices_example.ino
// ####################################################################################################

Gif::Gif() : gifPosition(0, 0) {}

Gif::~Gif() {
    gif->close();
    delete gif;
}

FS *Gif::GifFs = NULL;

void *Gif::openFile(const char *fname, int32_t *pSize) {
    File GifFile;

    if (GifFs != NULL) {
        GifFile = GifFs->open(fname);
    } else {
        if (SD.exists(fname)) GifFile = SD.open(fname);
        else if (LittleFS.exists(fname)) GifFile = LittleFS.open(fname);
    }

    File *FSGifFile = new File(GifFile);

    if (FSGifFile) {
        *pSize = FSGifFile->size();
        return (void *)FSGifFile;
    }
    return NULL;
}

void Gif::closeFile(void *pHandle) {
    File *f = static_cast<File *>(pHandle);
    if (f != NULL) { f->close(); }
    delete f;
}

int32_t Gif::readFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen)
        iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    if (iBytesRead <= 0) return 0;
    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();
    return iBytesRead;
}

int32_t Gif::seekFile(GIFFILE *pFile, int32_t iPosition) {
    int i = micros();
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();
    i = micros() - i;
    return pFile->iPos;
}

void Gif::GIFDraw(GIFDRAW *pDraw) {
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[tftWidth];
    int x, y, iWidth;

    GifPosition *position = (GifPosition *)(pDraw->pUser);

    iWidth = pDraw->iWidth;
    if (iWidth > tftWidth) iWidth = tftWidth;
    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line

    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) { // restore to background color
        for (x = 0; x < iWidth; x++) {
            if (s[x] == pDraw->ucTransparent) s[x] = pDraw->ucBackground;
        }
        pDraw->ucHasTransparency = 0;
    }
    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) { // if transparency used
        uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
        int x, iCount;
        pEnd = s + iWidth;
        x = 0;
        iCount = 0; // count non-transparent pixels
        while (x < iWidth) {
            c = ucTransparent - 1;
            d = usTemp;
            while (c != ucTransparent && s < pEnd) {
                c = *s++;
                if (c == ucTransparent) { // done, stop
                    s--;                  // back up to treat it like transparent
                } else {                  // opaque
                    *d++ = usPalette[c];
                    iCount++;
                }
            } // while looking for opaque pixels
            if (iCount) { // any opaque pixels?
                tft.drawPixel(0, 0, 0);
                tft.pushImage(pDraw->iX + x + position->x, y + position->y, iCount, 1, (uint16_t *)usTemp);
                x += iCount;
                iCount = 0;
            }
            // no, look for a run of transparent pixels
            c = ucTransparent;
            while (c == ucTransparent && s < pEnd) {
                c = *s++;
                if (c == ucTransparent) iCount++;
                else s--;
            }
            if (iCount) {
                x += iCount; // skip these
                iCount = 0;
            }
        }
    } else {
        s = pDraw->pPixels;
        // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
        for (x = 0; x < iWidth; x++) usTemp[x] = usPalette[*s++];
        tft.drawPixel(0, 0, 0);
        tft.pushImage(pDraw->iX + position->x, y + position->y, iWidth, 1, (uint16_t *)usTemp);
    }
} /* GIFDraw() */

bool Gif::openGIF(FS *fs, const char *filename) {
    if (fs != NULL) {
        GifFs = fs;
        if (!fs->exists(filename)) return false;
    } else {
        GifFs = NULL;
    }

    gif = new AnimatedGIF();
    gif->begin(BIG_ENDIAN_PIXELS);
    if (gif->open(filename, openFile, closeFile, readFile, seekFile, GIFDraw)) { return true; }

    log_e("GIF opening error: %d\n", gif->getLastError());
    return false;
}

// Play a single frame
// returns:
// 2 = skipped waiting for another frame
// 1 = good result and more frames exist
// 0 = no more frames exist, a frame may or may not have been played: use getLastError() and look for
// GIF_SUCCESS to know if a frame was played -1 = error
int Gif::playFrame(int x, int y, bool bSync) {
    if (bSync && ((millis() - lTime) >= *delayMilliseconds)) {
        lTime = millis();
        gifPosition.x = x;
        gifPosition.y = y;
        return gif->playFrame(false, delayMilliseconds, &gifPosition);
    }

    return 2;
}

int Gif::getLastError() { return gif->getLastError(); }

/*
 * playDurationMs:
 *  -1 : Play the GIF in an infinite loop
 *  0  : Play the GIF once
 * >0  : Play the GIF for the specified duration in milliseconds
 *       (e.g., 1000 = play for 1 second)
 */
bool showGif(FS *fs, const char *filename, int x, int y, bool center, int playDurationMs) {
    if (!fs->exists(filename)) return false;

    Gif gif;
    bool success = gif.openGIF(fs, filename);
    if (!success) { return false; }

    if (center) {
        x = x + (tftWidth - gif.getCanvasWidth()) / 2;
        y = y + (tftHeight - gif.getCanvasHeight()) / 2;
    }

    int result = 0;
    long timeStart = millis();
    do {
        result = gif.playFrame(x, y);
        if (result == -1) log_e("GIF playFrame error: %d\n", gif.getLastError());

        if (check(AnyKeyPress)) break;

        if (playDurationMs > 0 && (millis() - timeStart) > playDurationMs) break;
        if (playDurationMs == 0 && result == 0) break;
    } while (result >= 0);

    return true;
}
#endif
/***************************************************************************************
** Function name: getComplementaryColor2
** Description:   Get simple complementary color in RGB565 format
***************************************************************************************/
uint16_t getComplementaryColor2(uint16_t color) {
    int r = 31 - ((color >> 11) & 0x1F);
    int g = 63 - ((color >> 5) & 0x3F);
    int b = 31 - (color & 0x1F);
    return (r << 11) | (g << 5) | b;
}
/***************************************************************************************
** Function name: getComplementaryColor
** Description:   Get complementary color in RGB565 format
***************************************************************************************/
uint16_t getComplementaryColor(uint16_t color) {
    double r = ((color >> 11) & 0x1F) / 31.0;
    double g = ((color >> 5) & 0x3F) / 63.0;
    double b = (color & 0x1F) / 31.0;

    double cmax = fmax(r, fmax(g, b));
    double cmin = fmin(r, fmin(g, b));
    double delta = cmax - cmin;

    double hue = 0.0;
    if (delta == 0) hue = 0.0;
    else if (cmax == r) hue = 60 * fmod((g - b) / delta, 6);
    else if (cmax == g) hue = 60 * ((b - r) / delta + 2);
    else hue = 60 * ((r - g) / delta + 4);

    if (hue < 0) hue += 360;

    double lightness = (cmax + cmin) / 2;
    double saturation = (delta == 0) ? 0 : delta / (1 - std::abs(2 * lightness - 1));

    double compHue = fmod(hue + 180, 360);

    double c = (1 - std::abs(2 * lightness - 1)) * saturation;
    double x = c * (1 - std::abs(fmod(compHue / 60, 2) - 1));
    double m = lightness - c / 2;

    double compR = 0, compG = 0, compB = 0;
    if (compHue >= 0 && compHue < 60) {
        compR = c;
        compG = x;
    } else if (compHue >= 60 && compHue < 120) {
        compR = x;
        compG = c;
    } else if (compHue >= 120 && compHue < 180) {
        compG = c;
        compB = x;
    } else if (compHue >= 180 && compHue < 240) {
        compG = x;
        compB = c;
    } else if (compHue >= 240 && compHue < 300) {
        compB = c;
        compR = x;
    } else {
        compB = x;
        compR = c;
    }

    uint16_t compl_color = uint8_t(compR * 31) << 11 | uint8_t(compG * 63) << 5 | uint8_t(compB * 31);

    // change black color
    if (compl_color == 0) compl_color = color - 0x1111;

    return compl_color;
}

/***************************************************************************************
** Function name: getColorVariation
** Description:   Get a variation of color in RGB565 format
***************************************************************************************/
uint16_t getColorVariation(uint16_t color, int delta, int direction) {
    uint8_t r = ((color >> 11) & 0x1F);
    uint8_t g = ((color >> 5) & 0x3F);
    uint8_t b = (color & 0x1F);

    float brightness = 0.299 * r / 31 + 0.587 * g / 63 + 0.114 * b / 31;

    if (direction < 0 || (direction == 0 && brightness >= 0.5)) {
        r = max(0, r - delta);
        g = max(0, g - 2 * delta);
        b = max(0, b - delta);
    } else {
        r = min(31, r + delta);
        g = min(63, g + 2 * delta);
        b = min(31, b + delta);
    }

    uint16_t compl_color = r << 11 | g << 5 | b;

    return compl_color;
}

// Draw BITMAP files
// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) {
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read(); // MSB
    return result;
}

uint32_t read32(fs::File &f) {
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read(); // MSB
    return result;
}
bool drawBmp(FS &fs, String filename, int x, int y, bool center) {
    if ((x >= tft.width()) || (y >= tft.height())) return false;
    uint32_t startTime = millis();

    File bmpFS;

    // Open requested file on SD card
    bmpFS = fs.open(filename, "r");

    if (!bmpFS) {
        Serial.print("File not found");
        goto ERROR;
    }

    uint32_t seekOffset;
    uint16_t w, h, row, col;
    uint8_t r, g, b;

    if (read16(bmpFS) == 0x4D42) {
        read32(bmpFS);
        read32(bmpFS);
        seekOffset = read32(bmpFS);
        read32(bmpFS);
        w = read32(bmpFS);
        h = read32(bmpFS);
        if (center) {
            x = x + (tftWidth - w) / 2;
            y = y + (tftHeight - h) / 2;
        }

        if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0)) {
            y += h - 1;

            bool oldSwapBytes = tft.getSwapBytes();
            tft.setSwapBytes(true);
            bmpFS.seek(seekOffset);

            uint16_t padding = (4 - ((w * 3) & 3)) & 3;
            uint8_t lineBuffer[w * 3 + padding];

            for (row = 0; row < h; row++) {

                bmpFS.read(lineBuffer, sizeof(lineBuffer));
                uint8_t *bptr = lineBuffer;
                uint16_t *tptr = (uint16_t *)lineBuffer;
                // Convert 24 to 16-bit colours
                for (uint16_t col = 0; col < w; col++) {
                    b = *bptr++;
                    g = *bptr++;
                    r = *bptr++;
                    *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                }

                // Push the pixel row to screen, pushImage will crop the line if needed
                // y is decremented as the BMP image is drawn bottom up
                tft.drawPixel(
                    0, 0, 0
                ); // shared TFT_Spi devices struggle to work, need call a line first sometimes
                tft.pushImage(x, y--, w, 1, (uint16_t *)lineBuffer);
            }
            tft.setSwapBytes(oldSwapBytes);
            Serial.print("BMP Loaded in ");
            Serial.print(millis() - startTime);
            Serial.println(" ms");
        } else {
            goto ERROR;
        }
    } else {
    ERROR:
        Serial.println("BMP format not recognized.");
        bmpFS.close();
        return false;
    }
    bmpFS.close();
    return true;
}

bool drawImg(FS &fs, String filename, int x, int y, bool center, int playDurationMs) {
    String ext = filename.substring(filename.lastIndexOf('.'));
    ext.toLowerCase();
    uint8_t fls = 2;         // 2 for Little FS
    if (&fs == &SD) fls = 0; // 0 for SD
    tft.imageToBin(fls, filename, x, y, center, playDurationMs);
    if (ext.endsWith("jpg")) return showJpeg(fs, filename, x, y, center);
    else if (ext.endsWith("bmp")) return drawBmp(fs, filename, x, y, center);
    else if (ext.endsWith("png")) return drawPNG(fs, filename, x, y, center);

#if !defined(LITE_VERSION)

    else if (ext.endsWith("gif")) return showGif(&fs, filename.c_str(), x, y, center, playDurationMs);
#endif
    else log_e("Image not supported");

    return false;
}
// ==============================================================================
// PHẦN NÀY DÁN VÀO CUỐI FILE display.cpp
// (Sau khi đã xóa các hàm PNGDraw/drawPNG cũ để tránh lỗi)
// ==============================================================================

#if !defined(LITE_VERSION)

// [FIX 1] Xử lý xung đột tên biến giữa AnimatedGIF và PNGdec
#undef INTELSHORT
#undef INTELLONG
#include <PNGdec.h>

PNG *png;
File myfile;
FS *_fs;

// --- Helper Functions cho PNG ---
void *myOpen(const char *filename, int32_t *size) {
    myfile = _fs->open(filename);
    *size = myfile.size();
    return &myfile;
}
void myClose(void *handle) { if (myfile) myfile.close(); }
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
    if (!myfile) return 0;
    return myfile.read(buffer, length);
}
int32_t mySeek(PNGFILE *handle, int32_t position) {
    if (!myfile) return 0;
    return myfile.seek(position);
}

// [FIX 2] Biến toàn cục để tính toán Scale
int _png_x = 0;
int _png_y = 0;
float _png_scale = 1.0;

// [FIX 3] Hàm vẽ Pixel: Sửa lỗi kiểu dữ liệu + Thêm Scale + Trong suốt
int PNGDraw(PNGDRAW *pDraw) {
    uint16_t *pixels = (uint16_t*)pDraw->pPixels;
    int y = pDraw->y;
    uint16_t transparentColor = bruceConfig.bgColor; 

    // Tính tọa độ Y sau khi scale
    int drawY = _png_y + (y * _png_scale);
    int pixelSize = ceil(_png_scale);
    if(pixelSize < 1) pixelSize = 1;

    for (int x = 0; x < pDraw->iWidth; x++) {
        uint16_t color = pixels[x];

        // Hack trong suốt: Màu đen (0x0000) -> Màu nền
        if (color == 0x0000) color = transparentColor;

        // Tính tọa độ X sau khi scale
        int drawX = _png_x + (x * _png_scale);

        if (_png_scale >= 1.0) {
            tft.fillRect(drawX, drawY, pixelSize, pixelSize, color);
        } else {
            tft.drawPixel(drawX, drawY, color);
        }
    }
    return 1; // Bắt buộc trả về 1
}

// [FIX 4] Hàm drawPNG mới: Hỗ trợ targetW, targetH để tự co giãn ảnh
bool drawPNG(FS &fs, String filename, int x, int y, bool center, int targetW = 0, int targetH = 0) {
    if ((x >= tft.width()) || (y >= tft.height())) return false;
    _fs = &fs;

    // Cấp phát bộ nhớ
    void *mem = psramFound() ? ps_malloc(sizeof(PNG)) : malloc(sizeof(PNG));
    if (!mem) return false;

    png = new (mem) PNG();
    int16_t rc = png->open(filename.c_str(), myOpen, myClose, myRead, mySeek, PNGDraw);
    
    if (rc == PNG_SUCCESS) {
        int imgW = png->getWidth();
        int imgH = png->getHeight();
        _png_scale = 1.0; 

        // Tính toán tỷ lệ Scale
        if (targetW > 0 && targetH > 0) {
            float scaleW = (float)targetW / (float)imgW;
            float scaleH = (float)targetH / (float)imgH;
            _png_scale = (scaleW < scaleH) ? scaleW : scaleH;
        }

        int finalW = imgW * _png_scale;
        int finalH = imgH * _png_scale;

        // Căn giữa
        if (center) {
            _png_x = x + (tftWidth - finalW) / 2;
            _png_y = y + (tftHeight - finalH) / 2;
        } else {
            _png_x = x + (targetW - finalW) / 2;
            _png_y = y + (targetH - finalH) / 2;
        }

        rc = png->decode(NULL, 0);
        png->close();
    }
    
    delete png;
    return (rc == PNG_SUCCESS);
}

// Wrapper cho code cũ
bool drawPNG(FS &fs, String filename, int x, int y, bool center) {
    return drawPNG(fs, filename, x, y, center, 0, 0);
}

#else
bool drawPNG(FS &fs, String filename, int x, int y, bool center) {
    return false;
}
#endif

// ==============================================================================
// UTILS
// ==============================================================================

void preparePngBin(FS &fs, String filename) {
    uint8_t fls = 2;         
    if (&fs == &SD) fls = 0; 
    tft.imageToBin(fls, filename, 0, 0, false, 0);
}

#if !defined(LITE_VERSION)
// ==============================================================================
// [THENG UI] LOGIC PHÓNG TO ICON 10x10 (PIXEL ART)
// Dán vào CUỐI file src/core/display.cpp
// ==============================================================================

// Hàm vẽ icon phóng to từ mảng PROGMEM (Pixel Art Scaling)
// data: Mảng hex của icon 10x10
// x, y: Vị trí vẽ trên màn hình
// srcW, srcH: Kích thước gốc (10, 10)
// targetW, targetH: Kích thước ô chứa (ví dụ 60, 60)
// ==============================================================================
// [THENG UI] LOGIC VẼ ICON & GIAO DIỆN (ĐÃ ĐỒNG BỘ MÀU THEME)
// ==============================================================================

// Hàm vẽ icon phóng to (ĐÃ SỬA: Icon sẽ hiển thị theo màu Theme)
void drawIconScaled(const uint16_t* data, int x, int y, int srcW, int srcH, int targetW, int targetH) {
    if (!data) return;

    // 1. Tính toán tỷ lệ phóng to
    float scaleX = (float)targetW / srcW;
    float scaleY = (float)targetH / srcH;
    float scale = (scaleX < scaleY) ? scaleX : scaleY; 
    
    int pixelSize = (int)scale; 
    if (pixelSize < 1) pixelSize = 1;

    // 2. Tính kích thước thực tế
    int finalW = srcW * pixelSize;
    int finalH = srcH * pixelSize;

    // 3. Căn giữa
    int offX = x + (targetW - finalW) / 2;
    int offY = y + (targetH - finalH) / 2;

    // 4. Vẽ Pixel Art
    for (int j = 0; j < srcH; j++) {
        for (int i = 0; i < srcW; i++) {
            // Đọc màu gốc từ Flash
            uint16_t color = pgm_read_word(&data[j * srcW + i]);
            
            // Xử lý trong suốt (Màu đen 0x0000 -> Bỏ qua)
            if (color != 0x0000) {
                // [THAY ĐỔI QUAN TRỌNG] 
                // Thay vì vẽ 'color' gốc, ta vẽ 'bruceConfig.priColor'
                // Để icon đồng bộ màu với giao diện chính
                tft.fillRect(offX + i * pixelSize, offY + j * pixelSize, pixelSize, pixelSize, bruceConfig.priColor);
            }
        }
    }
}

// Hàm vẽ khung giao diện
void _drawThengFrame(String title, const std::vector<ThengItem>& items, int selIndex) {
    int barH = 22;  
    int footH = 20; 
    int topM = barH + 2; 
    int textH = 20; 
    
    // Xóa màn hình & Vẽ Bar
    tft.fillRect(0, 0, tftWidth, tftHeight, bruceConfig.bgColor);
    tft.fillRect(0, 0, tftWidth, barH, bruceConfig.priColor);
    tft.setTextColor(bruceConfig.bgColor);
    setFont(16); tft.setCursor(5, (barH - 16)/2 + 2); tft.print(title);
    
    // Pin
    int batW = 18, batH = 9;
    tft.drawRect(tftWidth - batW - 5, (barH-batH)/2, batW, batH, bruceConfig.bgColor);
    tft.fillRect(tftWidth - batW - 3, (barH-batH)/2 + 2, batW-4, batH-4, bruceConfig.bgColor);

    // Grid System
    int cols = 3; int rows = 2;
    int cellW = tftWidth / cols;
    int cellH = (tftHeight - topM - footH) / rows;

    for(size_t i=0; i<items.size(); i++) {
        if(i >= 6) break;

        int c = i % cols; int r = i / cols;
        int x = c * cellW; int y = topM + (r * cellH);
        bool isSel = (i == selIndex);

        // Vẽ khung chọn
        if(isSel) {
            tft.drawRoundRect(x+2, y+2, cellW-4, cellH-4, 6, bruceConfig.priColor);
            tft.drawRoundRect(x+3, y+3, cellW-6, cellH-6, 6, bruceConfig.priColor);
        }

        // Layout
        int textAreaY = y + cellH - textH - 2; 
        int iconAreaH = cellH - textH - 6; 
        int iconAreaW = cellW - 4;
        int iconDrawX = x + 2; int iconDrawY = y + 4; 

        bool hasImg = false;

        // Vẽ Icon (Sử dụng hàm drawIconScaled đã sửa màu ở trên)
        if (items[i].iconData != nullptr) {
            drawIconScaled(items[i].iconData, iconDrawX, iconDrawY, items[i].iconW, items[i].iconH, iconAreaW, iconAreaH);
            hasImg = true;
        } else if(SD.exists(items[i].iconPath)) {
            drawPNG(SD, (char*)items[i].iconPath.c_str(), iconDrawX, iconDrawY, false, iconAreaW, iconAreaH);
            hasImg = true;
            #endif
        }

        // Fallback nếu không có ảnh
        if(!hasImg) {
            int s = (iconAreaW < iconAreaH) ? iconAreaW : iconAreaH; if (s > 30) s = 30; 
            int cx = x + (cellW - s)/2; int cy = y + (iconAreaH - s)/2 + 4;
            // Vẽ hộp fallback cũng theo màu Theme
            tft.fillRoundRect(cx, cy, s, s, 6, bruceConfig.priColor);
            tft.setTextColor(TFT_WHITE); setFont(20);
            tft.setCursor(cx + s/2 - 6, cy + s/2 - 8); tft.print(items[i].title.charAt(0));
        }

        // --- VẼ CHỮ TĨNH ---
        setFont(16);
        // Nếu đang chọn thì dùng màu chính (Primary), không chọn thì màu xám
        tft.setTextColor(isSel ? bruceConfig.priColor : TFT_DARKGREY);
        
        // Vẽ chữ căn giữa
        tft.drawCentreString(items[i].title, x + cellW/2, textAreaY, 1);
    }

    // Footer
    tft.drawLine(0, tftHeight - footH, tftWidth, tftHeight - footH, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    setFont(16);
    tft.drawCentreString("SEL: Open   ESC: Exit", tftWidth/2, tftHeight - 15, 1);
}

// Hàm vòng lặp chính (ĐÃ FIX LỖI THOÁT & INVERTED)
void runThengGUI(String title, std::vector<ThengItem> items) {
    if(items.empty()) return;
    int currentSel = 0; 
    bool needRedraw = true; 
    bool running = true;

    // Xóa bộ đệm nút bấm cũ
    delay(100); 

    while(running) {
        if(needRedraw) {
            // Xóa sạch màn hình trước khi vẽ lại để tránh đè đồ họa cũ
            tft.fillScreen(bruceConfig.bgColor);
            _drawThengFrame(title, items, currentSel);
            needRedraw = false;
        } 

        // --- 1. ĐỌC TOÀN BỘ TÍN HIỆU VÀO BIẾN TẠM (Chống nuốt phím) ---
        bool isNext = check(NextPress);
        bool isPrev = check(PrevPress);
        bool isDown = check(DownPress);
        bool isUp   = check(UpPress);
        bool isSel  = check(SelPress);

        // --- 2. KIỂM TRA NÚT THOÁT VẬT LÝ ---
        bool exitRequest = false;

        #if defined(EscPress)
            if (check(EscPress)) exitRequest = true;
        #endif
        #if defined(BackPress)
            if (check(BackPress)) exitRequest = true;
        #endif
        #if defined(BtnSide)
             if (check(BtnSide)) exitRequest = true;
        #endif
        #if defined(BtnB)
             if (check(BtnB)) exitRequest = true;
        #endif

        // THỰC HIỆN THOÁT NẾU BẤM NÚT CỨNG
        if (exitRequest) { 
            returnToMenu = true;
            running = false; 
            break; 
        }

        // --- 3. ĐIỀU HƯỚNG BẰNG CÁC BIẾN TẠM ---
        if(isNext) { 
            currentSel++; 
            if(currentSel >= items.size()) currentSel = 0; 
            needRedraw = true; 
        }
        else if(isPrev) { 
            currentSel--; 
            if(currentSel < 0) currentSel = items.size() - 1; 
            needRedraw = true; 
        }
        else if(isDown) { 
            if(currentSel + 3 < items.size()) currentSel += 3; 
            needRedraw = true; 
        }
        else if(isUp) { 
            if(currentSel - 3 >= 0) currentSel -= 3; 
            needRedraw = true; 
        }
        
        // --- 4. XỬ LÝ LỆNH CHỌN (NÚT SEL) ---
        else if(isSel) {
            // Trường hợp 1: Nếu chọn icon EXIT hoặc Back -> Thoát Menu
            if (items[currentSel].title == "EXIT" || items[currentSel].title == "Back") {
                if(items[currentSel].callback) items[currentSel].callback(); // Chạy lệnh STOP (nếu có)
                
                returnToMenu = true; // Đặt cờ để thoát hoàn toàn về menu gốc
                running = false;
                break; 
            }

            // Trường hợp 2: Chạy lệnh của các icon chức năng bình thường
            if(items[currentSel].callback) {
                items[currentSel].callback();
            }
            
            // Yêu cầu vẽ lại toàn bộ GUI sau khi chức năng (ví dụ quét Wifi) chạy xong
            needRedraw = true; 
        }
        
        yield(); 
        delay(50); 
    }
}