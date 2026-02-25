#pragma once
#include <Arduino.h>
#include <FS.h>
#include <map>

class Translation {
public:
    static std::map<String, String> dynamicDict;
    static String sdLangName;

    static bool loadUserLanguage(fs::FS &fs, String path);

    // TỐI ƯU: Thêm "const String &" để truyền tham chiếu (nhanh hơn copy)
    static String tr(const String &original);
};
