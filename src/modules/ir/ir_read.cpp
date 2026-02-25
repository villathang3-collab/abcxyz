/**
 * @file ir_read.cpp
 * @author @im.nix (https://github.com/Niximkk)
 * @author Rennan Cockles (https://github.com/rennancockles)
 * @brief Read IR signals
 * @version 0.2
 * @date 2024-08-03
 */
#include "ir_read.h"
#include "core/Translation.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/settings.h"
#include "ir_utils.h"
#include <IRrecv.h>
#include <IRutils.h>
#include <globals.h>

// ==========================================
// CHỈ NẠP FONT CUSTOM NẾU LÀ MÀN HÌNH NHỎ
// ==========================================
#if TFT_WIDTH <= 135 && TFT_HEIGHT <= 240
    #include "core/Unifont8.h"   // Font 8px
    #include "core/Unifont16.h"  // Font 16px
    #include "core/Unifont20.h"  // Font 20px
#endif

// Định nghĩa các mức Font
#ifndef FP
    #define FP 1
    #define FM 2
    #define FG 3
#endif

/* Dont touch this */
#define IR_FREQUENCY 38000
#define DUTY_CYCLE 0.330000

String uint32ToString(uint32_t value) {
    char buffer[12] = {0};
    snprintf(buffer, sizeof(buffer), "%02lX %02lX %02lX %02lX",
             value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF);
    return String(buffer);
}

String uint32ToStringInverted(uint32_t value) {
    char buffer[12] = {0};
    snprintf(buffer, sizeof(buffer), "%02lX %02lX %02lX %02lX",
             (value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
    return String(buffer);
}

IrRead::IrRead(bool headless_mode, bool raw_mode) {
    headless = headless_mode;
    raw = raw_mode;
    setup();
}
bool quickloop = false;

void IrRead::setup() {
    irrecv.enableIRIn();

#ifdef USE_BOOST
    PPM.enableOTG();
#endif
    // Checks if irRx pin is properly set
    const std::vector<std::pair<String, int>> pins = IR_RX_PINS;
    int count = 0;
    for (auto pin : pins) {
        if (pin.second == bruceConfigPins.irRx) count++;
    }
    if (count == 0) gsetIrRxPin(true);

    setup_ir_pin(bruceConfigPins.irRx, INPUT);
    if (headless) return;

    returnToMenu = true;
    std::vector<Option> quickRemoteOptions = {
        {"TV", [&]() { quickButtons = quickButtonsTV; begin(); return loop(); }},
        {"AC", [&]() { quickButtons = quickButtonsAC; begin(); return loop(); }},
        {"SOUND", [&]() { quickButtons = quickButtonsSOUND; begin(); return loop(); }},
    };
    options = {
        {Translation::tr("Custom Read").c_str(), [&]() { begin(); return loop(); }},
        {Translation::tr("Quick Remote Setup").c_str(), [&]() { quickloop = true; loopOptions(quickRemoteOptions); }},
        {Translation::tr("Menu").c_str(), yield},
    };
    loopOptions(options);
}

void IrRead::loop() {
    while (1) {
        if (check(EscPress)) {
            returnToMenu = true;
            button_pos = 0;
            quickloop = false;
#ifdef USE_BOOST
            PPM.disableOTG();
#endif
            break;
        }
        if (check(NextPress)) save_signal();
        if (quickloop && button_pos == quickButtons.size()) save_device();
        if (check(SelPress)) save_device();
        if (check(PrevPress)) discard_signal();

        read_signal();
    }
}

void IrRead::begin() {
    _read_signal = false;

    display_banner();

// ==========================================
// [SỬA LỖI] ĐẨY CHỮ XUỐNG DƯỚI ĐỂ KHÔNG BỊ ĐÈ
// ==========================================
#if TFT_WIDTH <= 135 && TFT_HEIGHT <= 240
    // [MÀN HÌNH NHỎ]
    setFont(FM); // Font 16px

    // Sửa Y từ 60 -> 85 (Né dòng "Số tín hiệu" ở Y=55 ra)
    int yPos = 85;

    if (quickloop) {
        tft.setCursor(BORDER_PAD_X, yPos);
        tft.println(Translation::tr("Waiting for signal of button: ") + String(quickButtons[button_pos]));
    } else {
        tft.setCursor(BORDER_PAD_X, yPos);
        tft.println(Translation::tr("Waiting for signal..."));
    }
    tft.unloadFont();

#else
    // [MÀN HÌNH TO]
    if (quickloop) {
        padprintln(Translation::tr("Waiting for signal of button: ") + String(quickButtons[button_pos]));
    } else {
        padprintln(Translation::tr("Waiting for signal..."));
    }
#endif

    tft.println("");
    display_btn_options();
    delay(300);
}

void IrRead::cls() {
    drawMainBorder();
    tft.setCursor(10, 28);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
}

void IrRead::display_banner() {
// ==========================================
// BANNER: TIÊU ĐỀ TO, COUNT VỪA
// ==========================================
#if TFT_WIDTH <= 135 && TFT_HEIGHT <= 240
    // [MÀN HÌNH NHỎ]
    drawMainBorder();

    // 1. Tiêu đề: Font 20 (FG)
    setFont(FG);
    String title = Translation::tr("IR Read");
    tft.drawCentreString(title, tftWidth / 2, 30);
    tft.unloadFont();

    // 2. Bộ đếm: Font 16 (FM)
    setFont(FM);
    String subTitle = Translation::tr("Signals captured: ") + String(signals_read);

    // Vẽ ở Y=55. (Font 16 cao khoảng 16px -> chiếm đến Y=71)
    // Vì vậy dòng ở hàm begin() phải bắt đầu từ > 75
    tft.drawString(subTitle, 10, 55);
    tft.unloadFont();

#else
    // [MÀN HÌNH TO]
    cls();
    tft.setTextSize(FM);
    padprintln(Translation::tr("IR Read"));
    tft.setTextSize(FP);
    padprintln("--------------");
    padprintln(Translation::tr("Signals captured: ") + String(signals_read));
    tft.println("");
#endif
}

void IrRead::display_btn_options() {
// ==========================================
// [SỬA LỖI] BUTTONS VỀ FONT 8 ĐỂ HIỆN FULL
// ==========================================
#if TFT_WIDTH <= 135 && TFT_HEIGHT <= 240
    // [MÀN HÌNH NHỎ]
    // Dùng Font 8 (FP) cho gọn
    setFont(FP);

    int lineHeight = 12; // Font 8 chỉ cần cách nhau 12px

    // Tính toán vẽ từ đáy màn hình lên
    int yPos = tftHeight - 15; // Dòng cuối cùng (ESC)

    // 1. Luôn hiện ESC
    tft.drawString(Translation::tr("[ESC]  Exit"), 10, yPos);

    // 2. Nếu có tín hiệu -> Hiện OK Save Device
    if (signals_read > 0) {
        yPos -= lineHeight;
        tft.drawString(Translation::tr("[OK]   Save Device"), 10, yPos);
    }

    // 3. Nếu đang đọc/xử lý -> Hiện Next/Prev
    if (_read_signal) {
        yPos -= lineHeight;
        tft.drawString(Translation::tr("[NEXT] Save Signal"), 10, yPos);

        yPos -= lineHeight;
        tft.drawString(Translation::tr("[PREV] Discard"), 10, yPos);
    }

    tft.unloadFont();

#else
    // [MÀN HÌNH TO]
    tft.println("");
    tft.println("");
    if (_read_signal) {
        padprintln(Translation::tr("Press [PREV] to discard signal"));
        padprintln(Translation::tr("Press [NEXT] to save signal"));
    }
    if (signals_read > 0) { padprintln(Translation::tr("Press [OK]   to save device")); }
    padprintln(Translation::tr("Press [ESC]  to exit"));
#endif
}

void IrRead::read_signal() {
    if (_read_signal || !irrecv.decode(&results)) return;

    _read_signal = true;
    raw = true;

    display_banner();

// ==========================================
// RAW DATA: FONT 16, VỊ TRÍ NÉ BANNER
// ==========================================
#if TFT_WIDTH <= 135 && TFT_HEIGHT <= 240
    // [MÀN HÌNH NHỎ]
    setFont(FM); // Font 16

    // Vẽ tiêu đề Raw
    tft.drawString(Translation::tr("RAW Data Captured:"), 10, 85); // Y=85 cho đồng bộ với begin()

    String raw_signal = parse_raw_signal();
    String displayRaw = raw_signal.substring(0, 25) + (raw_signal.length() > 25 ? "..." : "");

    // Vẽ nội dung Raw xuống dưới tiêu đề
    tft.drawString(displayRaw, 10, 105);
    tft.unloadFont();

#else
    // [MÀN HÌNH TO]
    padprint(Translation::tr("RAW Data Captured:"));
    String raw_signal = parse_raw_signal();
    tft.println(raw_signal.substring(0, 45) + (raw_signal.length() > 45 ? "..." : ""));
#endif

    display_btn_options();
    delay(500);
}
void IrRead::discard_signal() {
    if (!_read_signal) return;
    irrecv.resume();
    begin();
}

void IrRead::save_signal() {
    if (!_read_signal) return;
    if (!quickloop) {
        String btn_name = keyboard("Btn" + String(signals_read), 30, Translation::tr("Btn name:").c_str());
        append_to_file_str(btn_name);
    } else {
        append_to_file_str(quickButtons[button_pos]);
    }
    signals_read++;
    if (quickloop) button_pos++;
    discard_signal();
    delay(100);
}

String IrRead::parse_state_signal() {
    String r = "";
    uint16_t state_len = (results.bits) / 8;
    for (uint16_t i = 0; i < state_len; i++) {
        // r += uint64ToString(results.state[i], 16) + " ";
        r += ((results.state[i] < 0x10) ? "0" : ""); // adds 0 padding if necessary
        r += String(results.state[i], HEX) + " ";
    }
    r.toUpperCase();
    return r;
}

String IrRead::parse_raw_signal() {

    rawcode = resultToRawArray(&results);
    raw_data_len = getCorrectedRawLength(&results);

    String signal_code = "";

    for (uint16_t i = 0; i < raw_data_len; i++) { signal_code += String(rawcode[i]) + " "; }

    delete[] rawcode;
    rawcode = nullptr;
    signal_code.trim();

    return signal_code;
}

void IrRead::append_to_file_str(String btn_name) {
    strDeviceContent += "name: " + btn_name + "\n";

    if (raw) {
        strDeviceContent += "type: raw\n";
        strDeviceContent += "frequency: " + String(IR_FREQUENCY) + "\n";
        strDeviceContent += "duty_cycle: " + String(DUTY_CYCLE) + "\n";
        strDeviceContent += "data: " + parse_raw_signal() + "\n";
    } else {
        // parsed signal  https://github.com/jamisonderek/flipper-zero-tutorials/wiki/Infrared
        strDeviceContent += "type: parsed\n";
        switch (results.decode_type) {
            case decode_type_t::RC5: {
                if (results.command > 0x3F) strDeviceContent += "protocol: RC5X\n";
                else strDeviceContent += "protocol: RC5\n";
                break;
            }
            case decode_type_t::RC6: {
                strDeviceContent += "protocol: RC6\n";
                break;
            }
            case decode_type_t::SAMSUNG: {
                strDeviceContent += "protocol: Samsung32\n";
                break;
            }
            case decode_type_t::SONY: {
                // check address and command ranges to find the exact protocol
                if (results.address > 0xFF) strDeviceContent += "protocol: SIRC20\n";
                else if (results.address > 0x1F) strDeviceContent += "protocol: SIRC15\n";
                else strDeviceContent += "protocol: SIRC\n";
                break;
            }
            case decode_type_t::NEC: {
                // check address and command ranges to find the exact protocol
                if (results.address > 0xFFFF) strDeviceContent += "protocol: NEC42ext\n";
                else if (results.address > 0xFF1F) strDeviceContent += "protocol: NECext\n";
                else if (results.address > 0xFF) strDeviceContent += "protocol: NEC42\n";
                else strDeviceContent += "protocol: NEC\n";
                break;
            }
            case decode_type_t::UNKNOWN: {
                Serial.print("unknown protocol, try raw mode");
                return;
            }
            default: {
                strDeviceContent += "protocol: " + typeToString(results.decode_type, results.repeat) + "\n";
                break;
            }
        }

        strDeviceContent += "address: " + uint32ToString(results.address) + "\n";
        strDeviceContent += "command: " + uint32ToString(results.command) + "\n";

        // extra fields not supported on flipper
        strDeviceContent += "bits: " + String(results.bits) + "\n";
        if (hasACState(results.decode_type)) strDeviceContent += "state: " + parse_state_signal() + "\n";
        else if (results.bits > 32)
            strDeviceContent += "value: " + uint32ToString(results.value) + " " +
                                uint32ToString(results.value >> 32) + "\n"; // MEMO: from uint64_t
        else strDeviceContent += "value: " + uint32ToStringInverted(results.value) + "\n";
    }
    strDeviceContent += "#\n";
}

void IrRead::save_device() {
    if (signals_read == 0) return;

    String filename = keyboard("MyDevice", 30, Translation::tr("File name:").c_str());

    display_banner();

    FS *fs = nullptr;

    bool sdCardAvailable = setupSdCard();
    bool littleFsAvailable = checkLittleFsSize();

    if (sdCardAvailable && littleFsAvailable) {
        // ask to choose one
        options = {
            {Translation::tr("SD Card").c_str(),  [&]() { fs = &SD; }      },
            {Translation::tr("LittleFS").c_str(), [&]() { fs = &LittleFS; }},
        };

        loopOptions(options);
    } else if (sdCardAvailable) {
        fs = &SD;
    } else if (littleFsAvailable) {
        fs = &LittleFS;
    };

    if (fs && write_file(filename, fs)) {
        String msg = Translation::tr("File saved to ") +
                     String((fs == &SD) ? Translation::tr("SD Card") : Translation::tr("LittleFS")) + ".";
        displaySuccess(msg.c_str(), true);
        signals_read = 0;
        strDeviceContent = "";
    } else
        displayError(
            fs ? Translation::tr("Error writing file.").c_str()
               : Translation::tr("No storage available.").c_str(),
            true
        );

    delay(1000);

    irrecv.resume();
    begin();
}

String IrRead::loop_headless(int max_loops) {

    while (!irrecv.decode(&results)) { // MEMO: default timeout is 15ms
        max_loops -= 1;
        if (max_loops <= 0) {
            Serial.println("timeout");
            return ""; // nothing received
        }
        delay(1000);
        // delay(50);
    }

    irrecv.disableIRIn();

    if (!raw && results.decode_type == decode_type_t::UNKNOWN) {
        Serial.println("# decoding failed, try raw mode");
        return "";
    }

    if (results.overflow)
        displayWarning(Translation::tr("buffer overflow, data may be truncated").c_str(), true);
    // TODO: check results.repeat

    String r = "Filetype: IR signals file\n";
    r += "Version: 1\n";
    r += "#\n";
    r += "#\n";

    strDeviceContent = "";
    append_to_file_str("Unknown"); // writes on strDeviceContent
    r += strDeviceContent;

    return r;
}

bool IrRead::write_file(String filename, FS *fs) {
    if (fs == nullptr) return false;

    if (!(*fs).exists("/BruceIR")) (*fs).mkdir("/BruceIR");

    while ((*fs).exists("/BruceIR/" + filename + ".ir")) {
        int ch = 1;
        int i = 1;

        displayWarning(
            (Translation::tr("File \"") + filename + Translation::tr("\" already exists")).c_str(), true
        );
        display_banner();

        // ask to choose one
        options = {
            {Translation::tr("Append number").c_str(), [&]() { ch = 1; }},
            {Translation::tr("Overwrite").c_str(),     [&]() { ch = 2; }},
            {Translation::tr("Change name").c_str(),   [&]() { ch = 3; }},
        };

        loopOptions(options);

        switch (ch) {
            case 1:
                filename += "_";
                while ((*fs).exists("/BruceIR/" + filename + String(i) + ".ir")) i++;
                filename += String(i);
                break;
            case 2: (*fs).remove("/BruceIR/" + filename + ".ir"); break;
            case 3:
                filename = keyboard(filename, 30, Translation::tr("File name:").c_str());
                display_banner();
                break;
        }
    }

    File file = (*fs).open("/BruceIR/" + filename + ".ir", FILE_WRITE);

    if (!file) { return false; }

    file.println("Filetype: Bruce IR File");
    file.println("Version: 1");
    file.println("#");
    file.println("# " + filename);
    file.print(strDeviceContent);

    file.close();
    delay(100);
    return true;
}
