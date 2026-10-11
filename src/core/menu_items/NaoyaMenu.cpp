#include "NaoyaMenu.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/wifi/wifi_common.h"
#include "core/utils.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const unsigned char epd_bitmap_z7690173847197_e46228d2b92f93fc13e7b5ebaecd3d0a [] PROGMEM = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xf0, 0xff, 0xfc, 
    0xc3, 0x7f, 0xff, 0xf0, 0xff, 0xf3, 0xfc, 0x3f, 0xff, 0xf0, 0xff, 0xc7, 0xff, 0x1f, 0xff, 0xf0, 
    0xff, 0x9f, 0xff, 0xbf, 0xbf, 0xf0, 0xff, 0xff, 0xff, 0xe1, 0x9f, 0xf0, 0xff, 0xff, 0xff, 0xf3, 
    0xdf, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xf0, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xef, 0xff, 0xff, 0xff, 
    0xff, 0xf0, 0xe7, 0xff, 0xff, 0xff, 0xff, 0x70, 0xff, 0xff, 0xff, 0xff, 0xff, 0x70, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0x70, 0xdf, 0xff, 0xbf, 0xff, 0xff, 0x70, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xf0, 
    0xcf, 0xff, 0xfd, 0x6f, 0xe6, 0x70, 0x9f, 0xfe, 0xcd, 0x0f, 0xe6, 0x30, 0x8b, 0xdf, 0xc5, 0x0f, 
    0x64, 0x70, 0x83, 0xbd, 0xc3, 0x0f, 0x04, 0x70, 0x83, 0xbb, 0xc3, 0x0f, 0x04, 0x70, 0x9f, 0xfb, 
    0xc0, 0x1f, 0x80, 0x70, 0x9d, 0xd7, 0xe0, 0x1f, 0xc1, 0x70, 0xc8, 0x6f, 0xe0, 0xdf, 0xc1, 0x70, 
    0xc4, 0x0f, 0xe0, 0xff, 0xe1, 0x70, 0xee, 0x1f, 0x80, 0xc7, 0xe3, 0xf0, 0xf6, 0x1f, 0xe1, 0xff, 
    0xd1, 0xf0, 0xf2, 0x1f, 0xf3, 0xfc, 0x33, 0xf0, 0xf8, 0x07, 0xf3, 0xf1, 0xf1, 0xf0, 0xfc, 0x10, 
    0x1b, 0xe7, 0xfb, 0xf0, 0xfe, 0x1f, 0xff, 0xfe, 0x79, 0xf0, 0xfe, 0x1b, 0xff, 0xff, 0xf9, 0xf0, 
    0xff, 0x1e, 0xff, 0xff, 0xfb, 0xf0, 0xff, 0xbf, 0xff, 0xff, 0xfb, 0xf0, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xe0, 0xff, 0xf0, 
    0xff, 0xff, 0xfd, 0xf9, 0xff, 0xf0, 0xff, 0xff, 0xe7, 0xe1, 0xff, 0xf0, 0xff, 0xff, 0xc0, 0x03, 
    0xff, 0xf0, 0xff, 0xff, 0xe1, 0xfb, 0xff, 0xf0, 0xff, 0xff, 0xf7, 0xfb, 0xff, 0xf0, 0xff, 0xff, 
    0xf3, 0xf7, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xef, 0xff, 0xf0, 0xff, 0xff, 0xfe, 0x0f, 0xff, 0xf0, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0
};

#if defined(BOARD_HAS_PSRAM)
#include "core/Unifont16.h" 
#endif

#define NAOYA_ICON_W 48
#define NAOYA_ICON_H 64

const char* naoya_api_keys[] = {
    "AIzaSyCSpWGUJilqUSn3p1wigDdLsVX5xr6dpwU", 
    "AIzaSyBfdsKFnKdxVm9yQnZtVuJCGueNxXhRHCY",
    "AIzaSyAFFjDb58oa2BZT-OdFz0r2ZwDm01nZ02M",
    "AIzaSyANCbINaqgrnZmtpfjm2ADcgKdusXRXEoE",
    "AIzaSyCurZ90bn0QRqKJbkiJ5HsT3OTGRIuK4sI",
    "AIzaSyCvEenOzXVYBG1Vdpsogczu_6uIFJ-P0cg",
    "AIzaSyC3Mu0MR4CYTAPfj4GrkOBlkTUf2qccrJE"
};
const int naoya_total_keys = 7;
static int current_key_idx = 0;

// Đặt toàn cục để không bao giờ bị báo "not declared"
String removeAccents(String text) {
    String res = text;
    const char* a_arr[] = {"á","à","ả","ã","ạ","â","ấ","ầ","ẩ","ẫ","ậ","ă","ắ","ằ","ẳ","ẵ","ặ","Á","À","Ả","Ã","Ạ","Â","Ấ","Ầ","Ẩ","Ẫ","Ậ","Ă","Ắ","Ằ","Ẳ","Ẵ","Ặ"};
    for(const char* c : a_arr) res.replace(c, "a");
    const char* e_arr[] = {"é","è","ẻ","ẽ","ẹ","ê","ế","ề","ể","ễ","ệ","É","È","Ẻ","Ẽ","Ẹ","Ê","Ế","Ề","Ể","Ễ","Ệ"};
    for(const char* c : e_arr) res.replace(c, "e");
    const char* i_arr[] = {"í","ì","ỉ","ĩ","ị","Í","Ì","Ỉ","Ĩ","Ị"};
    for(const char* c : i_arr) res.replace(c, "i");
    const char* o_arr[] = {"ó","ò","ỏ","õ","ọ","ô","ố","ồ","ổ","ỗ","ộ","ơ","ớ","ờ","ở","ỡ","ợ","Ó","Ò","Ỏ","Õ","Ọ","Ô","Ố","Ồ","Ổ","Ỗ","Ộ","Ơ","Ớ","Ờ","Ở","Ỡ","Ợ"};
    for(const char* c : o_arr) res.replace(c, "o");
    const char* u_arr[] = {"ú","ù","ủ","ũ","ụ","ư","ứ","ừ","ử","ữ","ự","Ú","Ù","Ủ","Ũ","Ụ","Ư","Ứ","Ừ","Ử","Ữ","Ự"};
    for(const char* c : u_arr) res.replace(c, "u");
    const char* y_arr[] = {"ý","ỳ","ỷ","ỹ","ỵ","Ý","Ỳ","Ỷ","Ỹ","Ỵ"};
    for(const char* c : y_arr) res.replace(c, "y");
    res.replace("đ", "d"); res.replace("Đ", "D");
    return res;
}

static int calcBubbleHeight(String text, int wrapWidth) {
    text.trim(); // Dọn dẹp \n của AI

    int lineHeight = 0;
#if defined(BOARD_HAS_PSRAM)
    lineHeight = 18; 
#else
    lineHeight = 12; 
#endif

    int totalLines = 0;
    int startIdx = 0;
    int len = text.length();

    while (startIdx < len) {
        int nlIdx = text.indexOf('\n', startIdx);
        if (nlIdx == -1) nlIdx = len;

        String lineStr = text.substring(startIdx, nlIdx);
        int pixelWidth = tft.textWidth(lineStr); 
        
        if (pixelWidth == 0) {
            totalLines += 1; 
        } else {
            totalLines += ((pixelWidth - 1) / wrapWidth) + 1; 
        }

        startIdx = nlIdx + 1;
    }

    return (totalLines * lineHeight) + 10; 
}

void NaoyaMenu::optionsMenu() {
    if (!wifiConnected) {
        wifiConnectMenu(); 
        if (!wifiConnected) return; 
    }

    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    const char* cliTaskNames[] = {"cli", "CLI", "console", "serial", "serial_cli"};
    TaskHandle_t suspendedTasks[5] = {NULL};

    for (int i = 0; i < 5; i++) {
        TaskHandle_t t = xTaskGetHandle(cliTaskNames[i]);
        if (t != NULL && t != currentTask) {
            vTaskSuspend(t);
            suspendedTasks[i] = t; 
        }
    }

    chatHistory.clear();
    String welcomeMsg = "Chào bạn! Tôi là Naoya AI. Bạn cần gì?";
#if !defined(BOARD_HAS_PSRAM)
    welcomeMsg = removeAccents(welcomeMsg);
#endif
    chatHistory.push_back({welcomeMsg, false});
    
    scrollY = 0;
    inputSelected = false;
    bool redraw = true; 

    while (true) {
        if (Serial.available() > 0) {
            String serialCmd = Serial.readStringUntil('\n');
            serialCmd.trim();
            
            if (serialCmd.startsWith("naoya ")) {
                String msg = serialCmd.substring(6); 
                
                if (msg.startsWith("\"") && msg.endsWith("\"")) {
                    msg = msg.substring(1, msg.length() - 1);
                }

                msg.trim(); 

#if !defined(BOARD_HAS_PSRAM)
                msg = removeAccents(msg);
#endif
                chatHistory.push_back({msg, true});
                
                drawInterface(); 
                sendRequest(msg);
                redraw = true; 
            }
        }

        if (redraw) {
            drawInterface();
            redraw = false; 
        }

        if (check(EscPress)) {
            chatHistory.clear();
            returnToMenu = true;

            for (int i = 0; i < 5; i++) {
                if (suspendedTasks[i] != NULL) {
                    vTaskResume(suspendedTasks[i]);
                }
            }
            break;
        }

        if (check(PrevPress)) {
            inputSelected = false;
            scrollY += 30; 
            if (scrollY > 0) scrollY = 0;
            redraw = true; 
        }

        if (check(NextPress)) {
#if defined(BOARD_HAS_PSRAM)
            tft.loadFont(Font16); 
#else
            tft.setTextSize(1);
#endif
            int contentHeight = 0;
            for (auto &msg : chatHistory) {
                int wrapWidth = (tftWidth - 40) - 10;
                contentHeight += calcBubbleHeight(msg.text, wrapWidth) + 10;
            }
            
            if (abs(scrollY) + (tftHeight - 50) >= contentHeight) {
                if (!inputSelected) {
                    inputSelected = true;
                    redraw = true; 
                }
            } else {
                scrollY -= 30;
                redraw = true; 
            }
        }

        if (check(SelPress)) {
            if (inputSelected) {
#if defined(BOARD_HAS_PSRAM)
                tft.unloadFont(); 
#endif
                String q = keyboard("", 100, "Hỏi Naoya AI:");
                q.trim(); 
                if (q != "" && q != "\x1B") {
                    chatHistory.push_back({q, true});
                    inputSelected = false;
                    
                    drawInterface(); 
                    sendRequest(q);
                    redraw = true; 
                } else {
                    redraw = true; 
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

void NaoyaMenu::drawInterface() {
    tft.fillScreen(bruceConfig.bgColor);
    
#if defined(BOARD_HAS_PSRAM)
    tft.loadFont(Font16); 
#else
    tft.setTextSize(1);
#endif

    drawChatBubbles();

    uint16_t boxColor = inputSelected ? bruceConfig.priColor : TFT_DARKGREY;
    
    int boxW = 70;
    int boxH = 30;
    int boxX = tftWidth - boxW - 5;
    int boxY = tftHeight - boxH - 5;

    tft.drawRoundRect(boxX, boxY, boxW, boxH, 5, boxColor);
    tft.setTextColor(inputSelected ? TFT_WHITE : TFT_LIGHTGREY);
    tft.drawString("Chat", boxX + 15, boxY + 6);
}

void NaoyaMenu::drawChatBubbles() {
    int curY = 10 + scrollY; 
    tft.setTextWrap(true); 
    
    for (auto &msg : chatHistory) {
        String safeText = msg.text;
        safeText.trim(); 

        int bubbleW = tftWidth - 40;
        int wrapWidth = bubbleW - 10; 
        
        int bubbleH = calcBubbleHeight(safeText, wrapWidth); 
        
        uint16_t bColor = msg.isUser ? 0x03E0 : 0x18E3; 
        int bX = msg.isUser ? 30 : 10;
        int textOffsetY = msg.isUser ? 5 : 2; 

        if (curY + bubbleH > 0 && curY < tftHeight - 40) {
            tft.fillRoundRect(bX, curY, bubbleW, bubbleH, 8, bColor);
            tft.setTextColor(TFT_WHITE, bColor); 
            
            tft.setViewport(bX + 5, curY + textOffsetY, wrapWidth, bubbleH - 10);
            tft.setCursor(0, 0);
            
            int textLen = safeText.length();
            for (int i = 0; i < textLen; i += 100) {
                tft.print(safeText.substring(i, i + 100));
            }
            
            tft.resetViewport(); 
        }
        curY += bubbleH + 10;
    }
}

void NaoyaMenu::sendRequest(String question) {
    String q_lower = question;
    q_lower.toLowerCase(); 

    // Kiểm tra Easter Eggs
    if (q_lower.indexOf("ban voi uchiha danh nhau ai thang") != -1) {
        chatHistory.push_back({removeAccents("Neu anh ay dung 100% suc manh thi se rat kho doi pho do"), false});
        goto JUMP_TO_SCROLL; 
    }
    
    if (q_lower.indexOf("may se thua a") != -1) {
        chatHistory.push_back({"Nah, I'd win", false});
        goto JUMP_TO_SCROLL;
    }

    tft.unloadFont(); // Đuổi sạch Font khỏi RAM
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(1);
    tft.drawString(" Thinking... ", tftWidth - 85, 5);
    
    // NGHỈ NGƠI TUYỆT ĐỐI: Cho RAM có thời gian hồi phục
    vTaskDelay(pdMS_TO_TICKS(300)); 

    {
        String sysInst = "Ban la Naoya AI. Nguoi tao la Thang 96. TRA LOI TIENG VIET KHONG DAU.";
        if (q_lower.indexOf("gio") != -1) sysInst += " Gio: " + getTimeStr();

        String body;
        {
            JsonDocument doc; 
            doc["systemInstruction"]["parts"][0]["text"] = sysInst;
            doc["contents"][0]["parts"][0]["text"] = question;
            doc["generationConfig"]["maxOutputTokens"] = 800; // Hạ xuống tí cho chắc cú
            serializeJson(doc, body);
        }

        bool success = false;
        WiFiClientSecure client;
        client.setInsecure();
        
        // ==============================================================================
        // ĐÒN QUYẾT ĐỊNH: Ép bộ đệm xuống mức thấp nhất mà SSL vẫn chạy được (3KB/2KB)
        // Việc này giúp tránh lỗi reset do thiếu RAM liền mạch!
        // ==============================================================================
        #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR < 3
            client.setBufferSizes(3072, 2048);
        #endif

        HTTPClient http;
        current_key_idx = (current_key_idx + 1) % naoya_total_keys;
        String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=";
        url += naoya_api_keys[current_key_idx];

        http.begin(client, url);
        http.setTimeout(30000); 
        http.addHeader("Content-Type", "application/json");

        int code = http.POST(body);
        
        if (code == 200) {
            String payload = http.getString();
            JsonDocument res;
            if (!deserializeJson(res, payload)) {
                const char* reply = res["candidates"][0]["content"]["parts"][0]["text"];
                if (reply) {
                    String ans = removeAccents(String(reply));
                    ans.replace("**", "");
                    ans.trim(); 
                    
                    int chunkSize = 250;
                    for (int i = 0; i < ans.length(); i += chunkSize) {
                        chatHistory.push_back({ans.substring(i, i + chunkSize), false});
                    }
                    success = true;
                }
            }
        } else {
            String err = (code < 0) ? http.errorToString(code) : "HTTP " + String(code);
            chatHistory.push_back({removeAccents("Loi mang: " + err), false});
        }
        
        http.end();
        client.stop();
    }

JUMP_TO_SCROLL:
    tft.setTextSize(1);
    int contentHeight = 0;
    for (auto &msg : chatHistory) {
        contentHeight += calcBubbleHeight(msg.text, (tftWidth - 50)) + 10;
    }
    scrollY = -contentHeight + (tftHeight - 50); 
    if (scrollY > 0) scrollY = 0; 
}

String NaoyaMenu::getTimeStr() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return "khong xac dinh";
    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
    return String(buf);
}

void NaoyaMenu::drawIcon(float scale) {
    clearIconArea();

    int pixelScale = (int)(scale * 2.0); 
    if (pixelScale < 1) pixelScale = 1;

    int actualW = 44; 
    int actualH = 60;

    int scaledW = actualW * pixelScale;
    int scaledH = actualH * pixelScale;

    int drawX = iconCenterX - (scaledW / 2);
    int drawY = iconCenterY - (scaledH / 2);

    for (int y = 0; y < actualH; y++) {
        for (int x = 0; x < actualW; x++) {
            int byteIdx = (y * ((NAOYA_ICON_W + 7) / 8)) + (x / 8);
            uint8_t b = pgm_read_byte(&epd_bitmap_z7690173847197_e46228d2b92f93fc13e7b5ebaecd3d0a[byteIdx]);
            
            if (!(b & (1 << (7 - (x % 8))))) { 
                tft.fillRect(drawX + (x * pixelScale), drawY + (y * pixelScale), pixelScale, pixelScale, bruceConfig.priColor);
            }
        }
    }
}
void NaoyaMenu::drawIconImg() { drawIcon(1.0); }
bool NaoyaMenu::getTheme() { return true; }