#include "Translation.h"
#include "config.h"
#include "lang_vi.h"
#include "lang_ph.h"
#include "sd_functions.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <SD.h>

extern BruceConfig bruceConfig;
#if defined(BOARD_HAS_PSRAM)
    #define DICT_ATTR // Để trống = Nạp vào RAM
#else
    #define DICT_ATTR const // Thêm const = Ép vào Flash
#endif
std::map<String, String> Translation::dynamicDict;
String Translation::sdLangName = "Custom (User)";

bool Translation::loadUserLanguage(fs::FS &fs, String path) {
    if (path == "") return false;

    if (&fs == &SD) {
        if (!setupSdCard()) return false;
    }

    if (!fs.exists(path)) return false;

    File file = fs.open(path);
    if (!file) return false;

    // TỐI ƯU: Chỉ định dung lượng RAM tĩnh nếu có thể để tránh phân mảnh
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);

    if (!error) {
        dynamicDict.clear();
        JsonObject root = doc.as<JsonObject>();

        String suffix = (&fs == &SD) ? " (SD)" : " (LFS)";
        String langName = "";

        if (root.containsKey("LanguageName")) {
            langName = root["LanguageName"].as<String>();
        } else {
            int lastSlash = path.lastIndexOf('/');
            int lastDot = path.lastIndexOf('.');
            if (lastDot > lastSlash) {
                langName = path.substring(lastSlash + 1, lastDot);
            } else {
                langName = path.substring(lastSlash + 1);
            }
            if (langName.length() > 0) langName[0] = toupper(langName[0]);
        }

        Translation::sdLangName = langName + suffix;

        for (JsonPair kv : root) {
            String key = kv.key().c_str();
            if (key != "LanguageName") { dynamicDict[key] = kv.value().as<String>(); }
        }
        bruceConfig.langPath = path;
        file.close();
        return true;
    }
    file.close();
    return false;
}

// HÀM DỊCH TỐI ƯU HÓA (HOT PATH)
String Translation::tr(const String &original) {
    // 0. Kiểm tra nhanh
    if (bruceConfig.lang == 0 || original.length() == 0) return original;

    // --- KHAI BÁO CON TRỎ TẠM THỜI (TÙY THEO PHẦN CỨNG) ---
    #if defined(BOARD_HAS_PSRAM)
        // Nếu có PSRAM: Con trỏ trỏ vào vùng RAM (không có const thứ 2)
        const char * (*currentDict)[2] = nullptr;
    #else
        // Nếu không PSRAM: Con trỏ trỏ vào vùng Flash (bắt buộc có const thứ 2)
        const char * const (*currentDict)[2] = nullptr;
    #endif
    // -------------------------------------------------------

    // 1. CHỌN TỪ ĐIỂN
    if (bruceConfig.lang == 1) {
        currentDict = DICT_VI; // Trỏ vào tiếng Việt
    }
    else if (bruceConfig.lang == 3) {
        currentDict = DICT_PH; // Trỏ vào Filipino
    }

    // 2. TRA TỪ ĐIỂN
    if (currentDict != nullptr) {
        int i = 0;
        unsigned int len = original.length();

        while (strcmp(currentDict[i][0], "__END__") != 0) {
            // Tối ưu: Chỉ so sánh nếu độ dài bằng nhau
            if (strlen(currentDict[i][0]) == len) {
                if (original.equals(currentDict[i][0])) {
                    return String(currentDict[i][1]);
                }
            }
            i++;
        }
    }

    // 3. Ngôn ngữ động (SD/LittleFS)
    else if (bruceConfig.lang == 2) {
        auto it = dynamicDict.find(original);
        if (it != dynamicDict.end()) { return it->second; }
    }

    return original;
}