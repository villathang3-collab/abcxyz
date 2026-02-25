#include "ConfigMenu.h"
#include "core/Translation.h" // Đừng quên dòng này
#include "core/display.h"
#include "core/i2c_finder.h"
#include "core/main_menu.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#ifdef HAS_RGB_LED
#include "core/led_control.h"
#endif

void ConfigMenu::optionsMenu() {
    options = {
        {_("Brightness"), setBrightnessMenu},
        {_("Dimmer Time"), setDimmerTimeMenu},
        {_("Rotation"), lambdaHelper(gsetRotation, true)},
        {_("UI Color"), setUIColor},
        {_("UI Theme"), setTheme},

        // --- MENU NGÔN NGỮ ---
        {_("Language"),
         [this]() {
             int idx = 0;
             if (bruceConfig.lang == 1) idx = 1;                                    
             else if (bruceConfig.lang == 2 && bruceConfig.langPath != "") idx = 2; 

             std::vector<Option> langOpts;

             langOpts.push_back(
                 {"English",
                  [this]() {
                      bruceConfig.lang = 0;
                      bruceConfig.saveFile();
                  },
                  bruceConfig.lang == 0}
             );

             langOpts.push_back(
                 {"Tiếng Việt",
                  [this]() {
                      bruceConfig.lang = 1;
                      bruceConfig.saveFile();
                  },
                  bruceConfig.lang == 1}
             );

             langOpts.push_back(
                 {"Filipino",
                  [this]() {
                      bruceConfig.lang = 3; 
                      bruceConfig.saveFile();
                  },
                  bruceConfig.lang == 3}
             );
             
             if (bruceConfig.langPath != "") {
                 langOpts.push_back(
                     {Translation::sdLangName,
                      [this]() {
                          bruceConfig.lang = 2;
                          bruceConfig.saveFile();
                      },
                      bruceConfig.lang == 2}
                 );
             }

             langOpts.push_back({_("Load from SD Card..."), [this]() {
                                     if (!setupSdCard()) {
                                         displayError("No SD Card");
                                         return;
                                     }
                                     String path = loopSD(SD, true, "JSON");
                                     if (path != "") {
                                         Translation::loadUserLanguage(SD, path);
                                         bruceConfig.lang = 2;
                                         bruceConfig.langSrc = 2; 
                                         bruceConfig.saveFile();
                                         returnToMenu = true; 
                                     }
                                 }});

             langOpts.push_back({_("Load from Internal..."), [this]() {
                                     String path = loopSD(LittleFS, true, "JSON");
                                     if (path != "") {
                                         Translation::loadUserLanguage(LittleFS, path);
                                         bruceConfig.lang = 2;
                                         bruceConfig.langSrc = 1; 
                                         bruceConfig.saveFile();
                                         returnToMenu = true; 
                                     }
                                 }});

             langOpts.push_back({_("Back"), [this]() { returnToMenu = true; }});

             loopOptions(langOpts, MENU_TYPE_REGULAR, _("Select Language").c_str(), idx);
             returnToMenu = true;
         }},
         
        // --- MENU GAME BUTTON MAPPING (Đã sửa chỉ còn 2 chế độ) ---
        {_("Game Button Mapping"),
         [this]() {
             std::vector<Option> mappingOptions;
             mappingOptions.push_back(
                 {_("Normal"),
                  [this]() {
                      bruceConfig.gameButtonMapping = 0;
                      bruceConfig.saveFile();
                      returnToMenu = true; // Chọn xong thoát ra ngay
                  },
                  bruceConfig.gameButtonMapping == 0}
             );
             mappingOptions.push_back(
                 {_("Reversed"),
                  [this]() {
                      bruceConfig.gameButtonMapping = 1;
                      bruceConfig.saveFile();
                      returnToMenu = true; // Chọn xong thoát ra ngay
                  },
                  bruceConfig.gameButtonMapping == 1}
             );
             mappingOptions.push_back({_("Back"), [this]() { returnToMenu = true; }});
             loopOptions(mappingOptions, MENU_TYPE_REGULAR, _("Game Button Mapping").c_str(), bruceConfig.gameButtonMapping);
             returnToMenu = true;
         }},

        // Các mục cấu hình khác
        {String("InstaBoot: " + String(bruceConfig.instantBoot ? "ON" : "OFF")),
         [=]() {
             bruceConfig.instantBoot = !bruceConfig.instantBoot;
             bruceConfig.saveFile();
         }},

#ifdef HAS_RGB_LED
        {_("LED Color"),
         [=]() {
             beginLed();
             setLedColorConfig();
         }},
        {_("LED Effect"),
         [=]() {
             beginLed();
             setLedEffectConfig();
         }},
        {_("LED Brightness"),
         [=]() {
             beginLed();
             setLedBrightnessConfig();
         }},
        {_("Blink LED"), setLedBlinkConfig},
#endif
        {_("Sound"), setSoundConfig},
#if defined(HAS_NS4168_SPKR)
        {_("Volume"), setSoundVolume},
#endif
        {_("WiFi Startup"), setWifiStartupConfig},
        {_("Startup App"), setStartupApp},
        {_("Hide/Show Apps"), []() { mainMenu.hideAppsMenu(); }},
        {_("BLE API"), []() { enableBLEAPI(); }},
        {_("Wifi & Pass"), setNetworkCredsMenu},
        {_("Install BadUSB/BLE"), setBadUSBBLEMenu},
        {_("Clock"), setClock},
        {_("Sleep Mode"), setSleepMode},
        {_("Factory Reset"), [=]() { bruceConfig.factoryReset(); }},
        {_("Restart"), [=]() { ESP.restart(); }},
    };

    options.push_back({_("Power Off"), powerOff});
    options.push_back({_("Deep Sleep"), goToDeepSleep});

    if (bruceConfig.devMode) options.push_back({_("Dev Mode"), [this]() { devMenu(); }});

    options.push_back({_("About"), showDeviceInfo});
    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, _("Configuration").c_str());
}

void ConfigMenu::devMenu() {
    options = {
        {_("I2C Scanner"),     find_i2c_addresses                                   },
        {_("CC1101 Pins"),     [=]() { setSPIPinsMenu(bruceConfigPins.CC1101_bus); }},
        {_("NRF24 Pins"),      [=]() { setSPIPinsMenu(bruceConfigPins.NRF24_bus); } },
        {_("SD Card Pins"),    [=]() { setSPIPinsMenu(bruceConfigPins.SDCARD_bus); }},
        {_("I2C Pins"),        [=]() { setI2CPinsMenu(bruceConfigPins.i2c_bus); }   },
        {_("UART Pins"),       [=]() { setUARTPinsMenu(bruceConfigPins.uart_bus); } },
        {_("GPS Pins"),        [=]() { setUARTPinsMenu(bruceConfigPins.gps_bus); }  },
        {_("Serial USB"),
         [=]() {
             USBserial.setSerialOutput(&Serial);
             Serial1.end();
         }                                                                          },
        {_("Serial UART"),
         [=]() {
             if (bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
                 bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.uart_bus.tx)) {
                 sdcardSPI.end();
             }
             if (bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
                 bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.uart_bus.tx) ||
                 bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
                 bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.uart_bus.tx)) {
                 CC_NRF_SPI.end();
             }
             pinMode(bruceConfigPins.uart_bus.rx, INPUT);
             pinMode(bruceConfigPins.uart_bus.tx, OUTPUT);
             Serial1.begin(115200, SERIAL_8N1, bruceConfigPins.uart_bus.rx, bruceConfigPins.uart_bus.tx);
             USBserial.setSerialOutput(&Serial1);
         }                                                                          },
        {_("Disable DevMode"), [this]() { bruceConfig.setDevMode(false); }          },
        {_("Back"),            [this]() { optionsMenu(); }                          },
    };

    loopOptions(options, MENU_TYPE_SUBMENU, _("Dev Mode").c_str());
}

void ConfigMenu::drawIconImg() {
    drawImg(
        *bruceConfig.themeFS(),
        bruceConfig.getThemeItemImg(bruceConfig.theme.paths.config),
        0,
        imgCenterY,
        true
    );
}

void ConfigMenu::drawIcon(float scale) {
    clearIconArea();
    int radius = scale * 9;
    for (int i = 0; i < 6; i++) {
        tft.drawArc(
            iconCenterX,
            iconCenterY,
            3.5 * radius,
            2 * radius,
            15 + 60 * i,
            45 + 60 * i,
            bruceConfig.priColor,
            bruceConfig.bgColor,
            true
        );
    }
    tft.drawArc(
        iconCenterX,
        iconCenterY,
        2.5 * radius,
        radius,
        0,
        360,
        bruceConfig.priColor,
        bruceConfig.bgColor,
        false
    );
}