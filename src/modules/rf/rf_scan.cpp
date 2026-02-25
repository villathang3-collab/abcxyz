#include "rf_scan.h"
#include "core/Unifont16.h"
#include "core/display.h"
#include "core/led_control.h"
#include "core/sd_functions.h"
#include "core/type_convertion.h"
#include "rf_send.h"
#include <globals.h>
#include <sstream>
// Thêm đoạn này vào đầu file nếu chưa có
#if TFT_WIDTH <= 135 && TFT_HEIGHT <= 240
#include "core/Unifont12.h"
#include "core/Unifont20.h"
#endif
// [THÊM] Thư viện dịch thuật
#include "core/Translation.h"

RFScan::RFScan() { setup(); }

RFScan::~RFScan() { deinitRfModule(); }

void RFScan::setup() {
    if (!initRfModule("rx", bruceConfigPins.rfFreq)) { return; }

    RCSwitch_Enable_Receive(rcswitch);

    if (bruceConfigPins.rfScanRange < 0 || bruceConfigPins.rfScanRange > 3) {
        bruceConfigPins.setRfScanRange(3);
    }
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) { bruceConfigPins.setRfFxdFreq(1); }

    display_info(received, signals, ReadRAW, codesOnly, autoSave, title);

    if (bruceConfigPins.rfFxdFreq) frequency = bruceConfigPins.rfFreq;

    // Clear cache for RAW signal
    rcswitch.resetAvailable();
    returnToMenu = false;
    restartScan = false;

    return loop();
}

void RFScan::loop() {
    while (1) {
        if (check(EscPress) || returnToMenu) return;
        if (check(NextPress)) {
            select_menu_option();
            if (returnToMenu) return;
            return setup();
        }
        if (restartScan) return setup();

        if (bruceConfigPins.rfFxdFreq) frequency = bruceConfigPins.rfFreq;
        if (frequency <= 0) init_freqs();

        while (frequency <= 0) { // FastScan
            if (check(EscPress) || returnToMenu) return;
            if (check(NextPress)) {
                select_menu_option();
                if (returnToMenu) return;
                return setup();
            }

            if (fast_scan()) return setup(); // frequency found, reset
        }

        if (rcswitch.available() && !ReadRAW) {
            read_rcswitch();
            if (autoSave && (lastSavedKey != received.key || received.key == 0)) save_signal();
        }
        if (rcswitch.RAWavailable() && ReadRAW) {
            read_raw();
            if (autoSave && (lastSavedKey != received.key || received.key == 0)) save_signal();
        }
    }
}

void RFScan::RCSwitch_Enable_Receive(RCSwitch rcswitch) {
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        rcswitch.enableReceive(bruceConfigPins.CC1101_bus.io0);
    } else {
        rcswitch.enableReceive(bruceConfigPins.rfRx);
    }
}

void RFScan::init_freqs() {
    for (int i = 0; i < _MAX_TRIES; i++) {
        _freqs[i].freq = 433.92;
        _freqs[i].rssi = -75;
    }
    _try = 0;
}

bool RFScan::fast_scan() {

    if (idx < range_limits[bruceConfigPins.rfScanRange][0] ||
        idx > range_limits[bruceConfigPins.rfScanRange][1]) {
        idx = range_limits[bruceConfigPins.rfScanRange][0];
    }
    float checkFrequency = subghz_frequency_list[idx];
    setMHZ(checkFrequency);
    tft.drawPixel(0, 0, 0); // To make sure CC1101 shared with TFT works properly
    vTaskDelay(5 / portTICK_PERIOD_MS);
    rssi = ELECHOUSE_cc1101.getRssi();
    if (rssi > rssiThreshold) {
        _freqs[_try].freq = checkFrequency;
        _freqs[_try].rssi = rssi;
        _try++;
        if (_try >= _MAX_TRIES) {
            int max_index = 0;
            for (int i = 1; i < _MAX_TRIES; ++i) {
                if (_freqs[i].rssi > _freqs[max_index].rssi) { max_index = i; }
            }

            bruceConfigPins.setRfFreq(_freqs[max_index].freq, 2); // change to fixed frequency
            frequency = _freqs[max_index].freq;
            setMHZ(frequency);
            Serial.println("Frequency Found: " + String(frequency));
            rcswitch.resetAvailable();
            // When changing to fixed frequency, need to restart the module to reset the registers
            // so we get good signal reception at this frequency
            deinitRfModule();

            return true;
        }
    }
    ++idx;
    return false;
}

void RFScan::read_rcswitch() {
    // Add decoded data only (if any) to the RCCode
    uint64_t decoded = rcswitch.getReceivedValue();

    if (decoded) { // if there is a value decoded by RCSwitch, show it
        Serial.println("RcSwitch signal captured");
        blinkLed();
        ++signals;
        found_freq = frequency;
        received.frequency = long(frequency * 1000000);
        received.key = decoded;
        received.preset = String(rcswitch.getReceivedProtocol());
        received.protocol = "RcSwitch";
        received.te = rcswitch.getReceivedDelay();
        received.Bit = rcswitch.getReceivedBitlength();
        received.filepath = "signal_" + String(signals);
        received.data = "";

        frequency = 0;
        display_info(received, signals, ReadRAW, codesOnly, autoSave, title);
    }

    rcswitch.resetAvailable();
}

void RFScan::read_raw() {
    // Add RAW data (& decoded data if any) to the RCCode
    vTaskDelay(400 / portTICK_PERIOD_MS); // wait for all the signal to be read
    found_freq = frequency;
    unsigned int *raw = rcswitch.getRAWReceivedRawdata();
    uint64_t decoded = rcswitch.getReceivedValue();
    int transitions = 0;
    String _data = "";
    std::vector<int> durations;
    std::vector<int> indexed_durations;
    uint64_t result = 0;
    uint8_t repetition = 0;

    received.te = 0;
    for (transitions = 0; transitions < RCSWITCH_RAW_MAX_CHANGES; transitions++) {
        if (raw[transitions] == 0) break;
        if (transitions > 0) _data += " ";
        signed int sign = (transitions % 2 == 0) ? 1 : -1;

        int duration = sign * (int)raw[transitions];
        if (duration < -5000 && repetition < 2) { repetition += 1; }
        _data += String(duration);
        if (received.te == 0 && duration > 0) received.te = duration;

        if (!decoded && repetition == 1 && duration >= -5000) {
            int index = find_pulse_index(indexed_durations, duration);
            if (index == -1) {
                indexed_durations.push_back(abs(duration));
                index = indexed_durations.size() - 1;
            }
            durations.push_back(index); // Store indexes for CRC calculation
        }
    }

    received.data = _data;
    received.filepath = "signal_" + String(signals);
    received.frequency = long(frequency * 1000000);

    // if there is a value decoded by RCSwitch, show it
    if (decoded) {
        Serial.println("RcSwitch signal captured");
        blinkLed();
        ++signals;
        received.key = decoded;
        received.preset = String(rcswitch.getReceivedProtocol());
        received.protocol = "RcSwitch";
        received.indexed_durations = {};
        received.te = rcswitch.getReceivedDelay();
        received.Bit = rcswitch.getReceivedBitlength();
        frequency = 0;
        display_info(received, signals, ReadRAW, codesOnly, autoSave, title);
    }
    // if there is no value decoded by RCSwitch, but we calculated a CRC, show it
    else if (repetition >= 2 && !durations.empty()) {
        Serial.println("Raw signal captured");
        blinkLed();
        ++signals;
        received.preset = "0";
        received.protocol = "RAW";
        received.key = crc64_ecma(durations); // Calculate CRC-64
        received.indexed_durations = indexed_durations;
        received.Bit = durations.size();
        frequency = 0;
        display_info(received, signals, ReadRAW, codesOnly, autoSave, title);
    }
    // If there is no decoded value and no CRC calculated, only show the data when specified
    else if (!codesOnly) {
        Serial.println("Raw data captured");
        blinkLed();
        ++signals;
        received.preset = "0";
        received.protocol = "RAW";
        received.key = 0;
        received.indexed_durations = {};
        received.Bit = 0;
        frequency = 0;
        display_info(received, signals, ReadRAW, codesOnly, autoSave, title);
    }

    rcswitch.resetAvailable();
}

void RFScan::select_menu_option() {
#if !defined(T_EMBED_1101) && !defined(CONFIG_IDF_TARGET_ESP32C5)
    rcswitch.disableReceive(); // it is causing T-Embed to restart
#endif

    options = {};

    // [DỊCH] Các tùy chọn Menu
    if (received.protocol != "")
        options.emplace_back(Translation::tr("Replay").c_str(), [this]() { set_option(REPLAY); });
    if (received.data != "" && received.protocol != "RAW")
        options.emplace_back(Translation::tr("Replay as RAW").c_str(), [this]() { set_option(REPLAY_RAW); });

    if (received.protocol != "")
        options.emplace_back(Translation::tr("Save Signal").c_str(), [this]() { set_option(SAVE); });
    if (received.data != "" && received.protocol != "RAW")
        options.emplace_back(Translation::tr("Save as RAW").c_str(), [this]() { set_option(SAVE_RAW); });

    if (received.protocol != "")
        options.emplace_back(Translation::tr("Reset Signal").c_str(), [this]() { set_option(RESET); });

    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE)
        options.emplace_back(Translation::tr("Range").c_str(), [this]() { set_option(RANGE); });
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE && !bruceConfigPins.rfFxdFreq)
        options.emplace_back(Translation::tr("Threshold").c_str(), [this]() { set_option(THRESHOLD); });

    if (ReadRAW)
        options.emplace_back(Translation::tr("Mode = RAW").c_str(), [&]() {
            ReadRAW = false;
            return select_menu_option();
        });
    else
        options.emplace_back(Translation::tr("Mode = Decode").c_str(), [&]() {
            ReadRAW = true;
            return select_menu_option();
        });

    if (ReadRAW && codesOnly)
        options.emplace_back(Translation::tr("Filter = Code").c_str(), [&]() {
            codesOnly = false;
            return select_menu_option();
        });
    else if (ReadRAW)
        options.emplace_back(Translation::tr("Filter = All").c_str(), [&]() {
            codesOnly = true;
            return select_menu_option();
        });

    if (autoSave)
        options.emplace_back(Translation::tr("Save = Auto").c_str(), [&]() {
            autoSave = false;
            return select_menu_option();
        });
    else
        options.emplace_back(Translation::tr("Save = Manual").c_str(), [&]() {
            autoSave = true;
            return select_menu_option();
        });

    options.emplace_back(Translation::tr("Close Menu").c_str(), [this]() { set_option(CLOSE_MENU); });
    options.emplace_back(Translation::tr("Main Menu").c_str(), [this]() { set_option(MAIN_MENU); });

    loopOptions(options);
}

void RFScan::set_option(RFMenuOption option) {
    switch (option) {
        case REPLAY:
        case REPLAY_RAW: replay_signal(option == REPLAY_RAW); break;

        case SAVE:
        case SAVE_RAW: save_signal(option == SAVE_RAW); break;

        case RANGE: rf_range_selection(); break; // using a common function to other features
        case RESET: reset_signals(); break;
        case THRESHOLD: set_threshold(); break;

        case CLOSE_MENU: break;

        case MAIN_MENU: returnToMenu = true; return;
    }

    restartScan = true;
    deinitRfModule();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void RFScan::replay_signal(bool asRaw) {
    String actualProtocol = received.protocol;
    if (asRaw) { received.protocol = "RAW"; }
    sendRfCommand(received);
    addToRecentCodes(received);
    received.protocol = actualProtocol;
}

void RFScan::save_signal(bool asRaw) {
    asRaw = asRaw || received.protocol == "RAW";
    Serial.println(asRaw ? "RCSwitch_SaveSignal RAW true" : "RCSwitch_SaveSignal RAW false");
    decimalToHexString(received.key, hexString);
    RCSwitch_SaveSignal(found_freq, received, asRaw, hexString, autoSave);
    lastSavedKey = received.key;
}

void RFScan::reset_signals() {
    received.Bit = 0;
    received.data = "";
    received.key = 0;
    received.preset = "";
    received.protocol = "";
    signals = 0;
}

void RFScan::set_threshold() {
    options = {
        {Translation::tr("(-55) More Accurate").c_str(), [&]() { rssiThreshold = -55; }},
        {Translation::tr("(-60)").c_str(),               [&]() { rssiThreshold = -60; }},
        {Translation::tr("(-65) Default ").c_str(),      [&]() { rssiThreshold = -65; }},
        {Translation::tr("(-70)").c_str(),               [&]() { rssiThreshold = -70; }},
        {Translation::tr("(-75)").c_str(),               [&]() { rssiThreshold = -75; }},
        {Translation::tr("(-80) Less Accurate").c_str(), [&]() { rssiThreshold = -80; }},
    };
    loopOptions(options);
}

// ... (Đoạn set_range comment cũ giữ nguyên) ...

void display_info(RfCodes received, int signals, bool ReadRAW, bool codesOnly, bool autoSave, String title) {
// ==========================================
// [MÀN HÌNH NHỎ] (Giữ nguyên)
// ==========================================
#if TFT_WIDTH <= 135 && TFT_HEIGHT <= 240
    drawMainBorder();
    if (title != "") {
        setFont(FG);
        tft.drawCentreString(Translation::tr(title), tftWidth / 2, 30);
        tft.unloadFont();
    }
    if (received.protocol != "") display_signal_data(received);

    tft.loadFont(Font16);
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);

    int yPos = 60;
    int h = 20;

    if (!ReadRAW) tft.drawString(Translation::tr("Rec: Only RCSwitch"), 10, yPos);
    else if (codesOnly) tft.drawString(Translation::tr("Rec: RAW+CRC/RCSwitch"), 10, yPos);
    else tft.drawString(Translation::tr("Rec: Any RAW signal"), 10, yPos);
    yPos += h;

    if (autoSave) {
        tft.drawString(Translation::tr("Auto save: Enabled"), 10, yPos);
        yPos += h;
    }

    String scanInfo = Translation::tr("Scan: ");
    if (bruceConfigPins.rfFxdFreq) scanInfo += String(bruceConfigPins.rfFreq) + " MHz";
    else scanInfo += String(subghz_frequency_ranges[bruceConfigPins.rfScanRange]);
    tft.drawString(scanInfo, 10, yPos);
    yPos += h;

    tft.drawString(Translation::tr("Signals found: ") + String(signals), 10, yPos);

    tft.loadFont(Font12);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString(Translation::tr("[NEXT] Options"), 10, tftHeight - 15);
    tft.unloadFont();

// ==========================================
// [MÀN HÌNH TO] - ĐÃ DỊCH XUỐNG 18PX
// ==========================================
#else
    // 1. Vẽ tiêu đề
    if (title != "") drawMainBorderWithTitle(Translation::tr(title));
    else drawMainBorder();

    // 2. Hiển thị data nếu có
    if (received.protocol != "") display_signal_data(received);

    // 3. Cấu hình Font 16 (FM)
    tft.loadFont(Font16);
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);

    // [LOGIC MỚI] Tính toán vị trí và cộng thêm 18px như yêu cầu
    int fontH = tft.fontHeight();

    // Công thức cũ: fontH * 2 + 5
    // Công thức mới: (fontH * 2 + 5) + 18 = fontH * 2 + 23
    int yPos = fontH * 2 + 23;

    int h = fontH + 4; // Khoảng cách dòng

    // Vẽ nội dung
    if (!ReadRAW) tft.drawString(Translation::tr("Recording: Only RCSwitch codes."), 10, yPos);
    else if (codesOnly) tft.drawString(Translation::tr("Recording: RAW with CRC or RCSwitch."), 10, yPos);
    else tft.drawString(Translation::tr("Recording: Any RAW signal."), 10, yPos);
    yPos += h;

    if (autoSave) {
        tft.drawString(Translation::tr("Auto save: Enabled"), 10, yPos);
        yPos += h;
    }

    String scanInfo = Translation::tr("Scanning: ");
    if (bruceConfigPins.rfFxdFreq) scanInfo += String(bruceConfigPins.rfFreq) + " MHz";
    else scanInfo += String(subghz_frequency_ranges[bruceConfigPins.rfScanRange]);

    tft.drawString(scanInfo, 10, yPos);
    yPos += h;

    tft.drawString(Translation::tr("Total signals found: ") + String(signals), 10, yPos);

    // Footer (Nút bấm)
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString(Translation::tr("Press [NEXT] for options."), 10, tftHeight - fontH - 10);
#endif
}
void display_signal_data(RfCodes received) {
    std::string txt = received.data.c_str();
    std::stringstream ss(txt);
    std::string palavra;
    int transitions = 0;
    char hexString[64];

    while (ss >> palavra) transitions++;

    // [DỊCH] Protocol
    if (received.preset != "")
        padprintln(
            String(Translation::tr("Protocol")) + ": " + String(received.protocol) + "(" + received.preset +
            ")"
        );
    else padprintln(String(Translation::tr("Protocol")) + ": " + String(received.protocol));

    if (received.key > 0) {
        decimalToHexString(received.key, hexString);
        if (received.protocol == "RAW") {
            // [DỊCH] Lenght (giữ nguyên typo gốc của code nếu cần khớp dictionary, hoặc sửa cả 2)
            // Trong lang_vi.h có "Lenght: " nên ta dùng đúng key đó
            padprintln(
                String(Translation::tr("Lenght: ")) + String(received.Bit) + Translation::tr(" transitions")
            );
            // tft.setCursor(tft.getCursorX(), tft.getCursorY() + 2);
            padprintln(
                String(Translation::tr("Record length: ")) + String(transitions) +
                Translation::tr(" transitions")
            );
        } else {
            padprintln(String(Translation::tr("Lenght: ")) + String(received.Bit) + Translation::tr(" bits"));
            const char *b = dec2binWzerofill(received.key, min(received.Bit, 40));
            // tft.setCursor(tft.getCursorX(), tft.getCursorY() + 2);
            padprintln(String(Translation::tr("Binary: ")) + String(b));
        }
    } else {
        strcpy(hexString, Translation::tr("No code identified").c_str());
        padprintln(String(Translation::tr("Lenght: ")) + Translation::tr("No code identified"));
        padprintln(
            String(Translation::tr("Record length: ")) + String(transitions) + Translation::tr(" transitions")
        );
    }

    if (received.protocol == "RAW") padprintln("CRC: " + String(hexString)); // CRC chưa có trong dict, để tạm
    else padprintln(String(Translation::tr("Key: ")) + String(hexString));

    padprintln("");
}

bool RCSwitch_SaveSignal(float frequency, RfCodes codes, bool raw, char *key, bool autoSave) {
    FS *fs;
    String filename = "";

    if (!getFsStorage(fs)) {
        displayError(Translation::tr("No space left on device"), true);
        return false;
    }

    if (!codes.key && codes.data == "") {
        Serial.println("Empty signal, it was not saved.");
        return false;
    }

    String subfile_out = "Filetype: Bruce SubGhz File\nVersion 1\n";
    subfile_out += "Frequency: " + String(int(frequency * 1000000)) + "\n";
    if (!raw) {
        subfile_out += "Preset: " + String(codes.preset) + "\n";
        subfile_out += "Protocol: RcSwitch\n";
        subfile_out += "Bit: " + String(codes.Bit) + "\n";
        subfile_out += "Key: " + String(key) + "\n";
        subfile_out += "TE: " + String(codes.te) + "\n";
        filename = "rcs.sub";
        // subfile_out += "RAW_Data: " + codes.data;
    } else {
        // save as raw
        if (codes.preset == "1") {
            codes.preset = "FuriHalSubGhzPresetOok270Async";
        } else if (codes.preset == "2") {
            codes.preset = "FuriHalSubGhzPresetOok650Async";
        }

        subfile_out += "Preset: " + String(codes.preset) + "\n";
        subfile_out += "Protocol: RAW\n";
        subfile_out += "RAW_Data: " + codes.data;
        filename = "raw.sub";
    }

    String filepath = "/BruceRF";
    if (autoSave) filepath += "/autoSaved";
    File file = createNewFile(fs, filepath, filename);

    if (file) {
        file.println(subfile_out);
        if (!autoSave) displaySuccess(file.path());
    } else {
        displayError(Translation::tr("Error saving file"), true);
    }

    file.close();
    return true;
}

String rf_scan(float start_freq, float stop_freq, int max_loops) {
    // derived from https://github.com/mcore1976/cc1101-tool/blob/main/cc1101-tool-esp32.ino#L480

    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError(Translation::tr("rf scanning is available with CC1101 only"), true);
        return ""; // only CC1101 is supported for this
    }
    if (!initRfModule("rx", start_freq)) return "";

    ELECHOUSE_cc1101.setRxBW(256);

    float settingf1 = start_freq;
    float settingf2 = stop_freq;
    float freq = 0;
    long compare_freq = 0;
    float mark_freq;
    int rssi;
    int mark_rssi = -100;
    String out = "";

    while (max_loops || !check(EscPress)) {
        vTaskDelay(1 / portTICK_PERIOD_MS);
        max_loops -= 1;

        setMHZ(freq);

        rssi = ELECHOUSE_cc1101.getRssi();
        if (rssi > -75) {
            if (rssi > mark_rssi) {
                mark_rssi = rssi;
                mark_freq = freq;
            };
        };

        freq += 0.01;

        if (freq > settingf2) {
            freq = settingf1;

            if (mark_rssi > -75) {
                long fr = mark_freq * 100;
                if (fr == compare_freq) {
                    Serial.print(F("\r\nSignal found at  "));
                    Serial.print(F("Freq: "));
                    Serial.print(mark_freq);
                    Serial.print(F(" Rssi: "));
                    Serial.println(mark_rssi);
                    mark_rssi = -100;
                    compare_freq = 0;
                    mark_freq = 0;
                    out += String(mark_freq) + ",";
                } else {
                    compare_freq = mark_freq * 100;
                    freq = mark_freq - 0.10;
                    mark_freq = 0;
                    mark_rssi = -100;
                };
            };
        }; // end of IF freq>stop frequency
    }; // End of While

    deinitRfModule();
    return out;
}

String RCSwitch_Read(float frequency, int max_loops, bool raw) {
    RCSwitch rcswitch = RCSwitch();
    RfCodes received;

    if (!frequency) frequency = bruceConfigPins.rfFreq; // default from config

    char hexString[64];

RestartRec:
    drawMainBorder();
    tft.setCursor(10, 28);
    tft.setTextSize(FP);

    // [DỊCH] Waiting message
    // "Waiting for a " + freq + " MHz signal."
    String msg =
        String(Translation::tr("Waiting for a ")) + String(frequency) + " MHz " + Translation::tr(" signal.");
    tft.println(msg);

    // init receive
    if (!initRfModule("rx", frequency)) return "";
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) { // CC1101 in use
        rcswitch.enableReceive(bruceConfigPins.CC1101_bus.io0);
        Serial.println("CC1101 enableReceive()");

    } else {
        rcswitch.enableReceive(bruceConfigPins.rfRx);
    }
    // ... (Phần còn lại của hàm RCSwitch_Read giữ nguyên, chỉ sửa logic hiển thị) ...
    while (!check(EscPress)) {
        if (rcswitch.available()) {
            long value = rcswitch.getReceivedValue();
            if (value) {
                unsigned int *_raw = rcswitch.getReceivedRawdata();
                received.frequency = long(frequency * 1000000);
                received.key = rcswitch.getReceivedValue();
                received.protocol = "RcSwitch";
                received.preset = rcswitch.getReceivedProtocol();
                received.te = rcswitch.getReceivedDelay();
                received.Bit = rcswitch.getReceivedBitlength();
                received.filepath = "unsaved";

                received.data = "";
                int sign = +1;
                for (int i = 0; i < received.Bit * 2; i++) {
                    if (i > 0) received.data += " ";
                    if (i % 2 == 0) sign = +1;
                    else sign = -1;
                    received.data += String(sign * (int)_raw[i]);
                }
                decimalToHexString(received.key, hexString);

                display_info(received, 1, raw);
            }
            rcswitch.resetAvailable();
        }
        if (raw && rcswitch.RAWavailable()) {
            vTaskDelay(100 / portTICK_PERIOD_MS);

            unsigned int *_raw = rcswitch.getRAWReceivedRawdata();
            int transitions = 0;
            signed int sign = 1;
            for (transitions = 0; transitions < RCSWITCH_RAW_MAX_CHANGES; transitions++) {
                if (_raw[transitions] == 0) break;
                if (transitions > 0) received.data += " ";
                if (transitions % 2 == 0) sign = +1;
                else sign = -1;
                received.data += String(sign * (int)_raw[transitions]);
            }
            if (transitions > 20) {
                received.frequency = long(frequency * 1000000);
                received.protocol = "RAW";
                received.preset = "0";
                received.filepath = "unsaved";
                received.data = "";

                display_info(received, 1, raw);
            }
            rcswitch.resetAvailable();
        }

        if (received.key > 0 || received.data.length() > 20) {
            if (received.preset == 0) {
                Serial.println("signal decoding failed, switching to RAW mode");
                raw = true;
            }
            String subfile_out = "Filetype: Bruce SubGhz File\nVersion 1\n";
            subfile_out += "Frequency: " + String(int(frequency * 1000000)) + "\n";
            if (!raw) {
                subfile_out += "Preset: " + String(received.preset) + "\n";
                subfile_out += "Protocol: RcSwitch\n";
                subfile_out += "Bit: " + String(received.Bit) + "\n";
                subfile_out += "Key: " + String(hexString) + "\n";
                subfile_out += "TE: " + String(received.te) + "\n";
            } else {
                if (received.preset == "1") received.preset = "FuriHalSubGhzPresetOok270Async";
                else if (received.preset == "2") received.preset = "FuriHalSubGhzPresetOok650Async";
                subfile_out += "Preset: " + String(received.preset) + "\n";
                subfile_out += "Protocol: RAW\n";
                subfile_out += "RAW_Data: " + received.data;
            }
            return subfile_out;

            if (check(SelPress)) {
                int chosen = 0;
                options = {
                    {Translation::tr("Replay signal").c_str(), [&]() { chosen = 1; }},
                    {Translation::tr("Save signal").c_str(),   [&]() { chosen = 2; }},
                };
                loopOptions(options);
                if (chosen == 1) {
                    rcswitch.disableReceive();
                    sendRfCommand(received);
                    addToRecentCodes(received);
                    goto RestartRec;
                } else if (chosen == 2) {
                    decimalToHexString(received.key, hexString);
                    RCSwitch_SaveSignal(frequency, received, raw, hexString);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    drawMainBorder();
                    tft.setCursor(10, 28);
                    tft.setTextSize(FP);
                    // [DỊCH] Waiting message (again)
                    tft.println(
                        String(Translation::tr("Waiting for a ")) + String(frequency) + " MHz " +
                        Translation::tr(" signal.")
                    );
                }
            }
        }
        if (max_loops > 0) {
            max_loops -= 1;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            if (max_loops == 0) {
                Serial.println("timeout");
                return "";
            }
        }
    }
Exit:
    vTaskDelay(1 / portTICK_PERIOD_MS);
    deinitRfModule();
    return "";
}
