#include "FlipperMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "core/mykeyboard.h" 

// [QUAN TRỌNG] Include icon để hiển thị
#include "icons.h" 

// ==============================================================================
// CÁC MẢNG HẰNG SỐ (ĐÃ KHÔI PHỤC)
// ==============================================================================

const String captive_portals[] = {
    "Default", "Facebook", "Amazon", "Apple", "Microsoft", "Google"
};

const String deauth_reasons[] = {
    "Reserved", "Unspecified reason", "Prev auth no longer valid", "Deauth leaving IBSS/ESS",
    "Disassociated due to inactivity", "Unable to handle all STAs", "Class 2 frame from nonauth STA",
    "Class 3 frame from nonassoc STA", "Leaving BSS", "Not authenticated",
    "Unacceptable Power Cap", "Unacceptable Supported Ch", "BSS Transition Mgmt",
    "Invalid element", "MIC failure", "4-Way Handshake timeout", "Group Key Handshake timeout",
    "Element in 4-Way diff", "Invalid group cipher", "Invalid pairwise cipher",
    "Invalid AKMP", "Unsupported RSNE", "Invalid RSNE capabilities",
    "802.1X auth failed", "Cipher suite rejected"
};

const String mac_type_str[] = {"Default", "Random", "Custom", "Same as AP"};
const String securityType[] = {"OPEN", "WPA"};
const String channelList[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14",
    "36", "38", "40", "42", "44", "46", "48", "50", "52", "54", "56", "58", "60",
    "62", "64", "100", "102", "104", "106", "108", "110", "112", "114", "116",
    "118", "120", "122", "124", "126", "128", "132", "134", "136", "138", "140",
    "142", "144", "149", "151", "153", "155", "157", "159", "161", "165"
};

// ==============================================================================
// TIỆN ÍCH
// ==============================================================================

String FlipperMenu::generateRandomMac() {
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) {
        mac[i] = random(0, 256);
    }
    mac[0] &= 0xFE;
    mac[0] |= 0x02;

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return String(macStr);
}

void FlipperMenu::applyMacType(int type, FlipperAP *targetNet) {
    if (type == 0) customMac = "00:E0:4C:01:02:03";
    else if (type == 1) customMac = generateRandomMac();
    else if (type == 3 && targetNet) customMac = targetNet->bssid;
}

// ==============================================================================
// ENTRY POINT (GỌI THENG UI GRID TẠI ĐÂY)
// ==============================================================================

void FlipperMenu::optionsMenu() {
    flipper.begin(); 
    mainMenuScene();
}

void FlipperMenu::mainMenuScene() {
    // Sắp xếp đúng theo yêu cầu Grid 3x2
    // Đã thay đổi màu sắc thành 'bruceConfig.priColor' để đồng bộ Theme
    std::vector<ThengItem> flipperApps = {
        // [1. SCAN WIFI]
        {
            "SCAN WIFI",
            icon_scan, icon_scan_w, icon_scan_h, "", 
            bruceConfig.priColor, 
            [this]() { scanNetworksScene(); }
        },
        
        // [2. SEND BEACON]
        {
            "SEND BEACON",
            icon_beacon, icon_beacon_w, icon_beacon_h, "",
            bruceConfig.priColor,
            [this]() { beaconNetworkScene(); }
        },

        // [3. HELP]
        {
            "HELP",
            icon_help, icon_help_w, icon_help_h, "",
            bruceConfig.priColor,
            [this]() { helpScene(); }
        },

        // [4. LIST WIFI]
        {
            "LIST WIFIS",
            icon_list, icon_list_w, icon_list_h, "",
            bruceConfig.priColor,
            [this]() { listNetworksScene(); }
        },

        // [5. CREATE AP]
        {
            "CREATE AP",
            icon_attack, icon_attack_w, icon_attack_h, "",
            bruceConfig.priColor,
            [this]() { configAPScene(); }
        },

        // [6. EXIT]
{
            "EXIT",
            icon_stop, icon_stop_w, icon_stop_h, "", 
            bruceConfig.priColor,
            [this]() { 
                flipper.sendCommand("STOP"); 
                // displayInfo("Flipper Stopped"); // Có thể bỏ qua để chuyển cảnh nhanh
                returnToMenu = true; // Kích hoạt thoát vòng lặp
            }
        }
    };

    // Chạy giao diện Grid 10x10 -> Phóng to Pixel Art
    runThengGUI("FLIPPER ZERO", flipperApps);
}

// ==============================================================================
// CÁC HÀM LOGIC NGHIỆP VỤ (SCAN, ATTACK...)
// ==============================================================================

void FlipperMenu::scanNetworksScene() {
    drawMainBorderWithTitle(_("Loading..."));
    displayTextLine(_("Sending SCAN command"));

    flipper.scanNetworks();

    unsigned long startWait = millis();
    while(!flipper.scanFinished && millis() - startWait < 10000) {
        flipper.update();
        delay(10);
    }

    flipper.listNetworks(); 
    displayInfo(_("Scan complete"));
}

void FlipperMenu::listNetworksScene() {
    if(flipper.networks.empty()) {
        displayError(_("No networks found. Scan first!"), false);
        return;
    }

    while (true) {
        options.clear();
        for (auto &net : flipper.networks) {
            String prefix = net.deauth_active ? "D|" : "";
            String chStr = net.channel > 14 ? "5G|" : "2G|";
            options.push_back({prefix + chStr + net.ssid, [this, &net]() {
                wifiInfoScene(net);
            }});
        }
        options.push_back({_("Back"),  [this]() { returnToMenu = true; }});

        drawMainBorderWithTitle(_("Scanned Wifis"));
        loopOptions(options);

        if (returnToMenu) {
            returnToMenu = false;
            break;
        }
    }
}

void FlipperMenu::wifiInfoScene(FlipperAP &net) {
    while (true) {
        String secStr = net.security == 0 ? "OPEN" : "WPA/WPA2/WPA3";
        String info = "AP: " + net.ssid + "\n" + secStr + "\nCh: " + String(net.channel) + " | RSSI: " + String(net.rssi) + "\nMAC: " + net.bssid;

        options = {
            {net.deauth_active ? _("[Stop Deauth]") : _("[Deauth]"), [&]() {
                if(!net.deauth_active) {
                    net.deauth_active = true;
                    flipper.sendCommand("DEAUTH " + String(net.id));
                } else {
                    net.deauth_active = false;
                    flipper.sendCommand("STOP " + String(net.id));
                }
                displayInfo(net.deauth_active ? _("Deauthing...") : _("Stopped Deauth"));
            }},
            {_("[Config Attack]"), [&]() { configAttackScene(net); }},
            {_("[Start Evil Twin]"), [&]() {
                applyMacType(macType, &net);
                flipper.sendCommand("APMAC " + customMac);
                delay(200);
                flipper.sendCommand("REASON " + String(deauthReason));
                delay(200);
                flipper.sendCommand("PORTAL " + String(captivePortal));
                delay(200);
                flipper.sendCommand("EVIL " + String(net.id));
                evilCredsScene();
            }},
            {_("Back"), [&]() { returnToMenu = true; }}
        };

        drawMainBorderWithTitle(_("Wifi Info"));
        displayTextLine(info);
        loopOptions(options);

        if (returnToMenu) { returnToMenu = false; break; }
    }
}

void FlipperMenu::configAttackScene(FlipperAP &net) {
    while (true) {
        options = {
            {_("Reason: ") + deauth_reasons[deauthReason], [&]() { deauthReason = (deauthReason + 1) % 25; }},
            {_("Portal: ") + captive_portals[captivePortal], [&]() { captivePortal = (captivePortal + 1) % 6; }},
            {_("Mac: ") + mac_type_str[macType], [&]() {
                macType = (macType + 1) % 4;
                applyMacType(macType, &net);
            }},
            {_("Custom MAC: ") + customMac, [&]() {
                if (macType == 2) {
                    customMac = keyboard(customMac, 17, "Set Custom MAC");
                }
            }},
            {_("Back"), [&]() { returnToMenu = true; }}
        };

        drawMainBorderWithTitle(_("Config Attack"));
        loopOptions(options);
        if(returnToMenu) { returnToMenu = false; break; }
    }
}

void FlipperMenu::evilCredsScene() {
    flipper.capturedCredentials = "";
    bool isRunning = true;

    drawMainBorderWithTitle(_("Captured Data"));

    while(isRunning) {
        flipper.update(); 

        tft.fillRect(0, 40, tftWidth, tftHeight - 40, bruceConfig.bgColor);
        if (flipper.lastError != "") {
            tft.setTextColor(TFT_RED);
            tft.drawCentreString("ERROR: " + flipper.lastError, tftWidth / 2, 80, 2);
        }
        else if(flipper.capturedCredentials != "") {
            tft.setTextColor(TFT_GREEN);
            tft.setCursor(5, 50);
            tft.print(flipper.capturedCredentials); 
        } else {
            tft.setTextColor(TFT_YELLOW);
            tft.drawCentreString(_("Waiting for creds..."), tftWidth / 2, 80, 1);
        }

        if (check(AnyKeyPress)) {
            flipper.sendCommand("STOP");
            isRunning = false;
        }
        delay(100);
    }
}

void FlipperMenu::beaconNetworkScene() {
    while (true) {
        options = {
            {_("Custom AP Beacon"),   [this]() {
                beaconInfoScene(0, "Custom_bruce_AP");
            }},
            {_("Random AP Beacon"),   [this]() {
                flipper.sendCommand("RANDOM");
                beaconInfoScene(1, "");
            }},
            {_("RickRoll AP Beacon"), [this]() {
                flipper.sendCommand("RICKROLL");
                beaconInfoScene(2, "");
            }},
            {_("Back"), [this]() { returnToMenu = true; }}
        };
        drawMainBorderWithTitle(_("Beacon Selection"));
        loopOptions(options);
        if (returnToMenu) { returnToMenu = false; break; }
    }
}

void FlipperMenu::beaconInfoScene(int type, String customSsid) {
    if (type == 0) {
        flipper.sendCommand("BSSID " + customSsid);
    }

    bool isRunning = true;
    drawMainBorderWithTitle(_("Beacon Info"));

    while(isRunning) {
        flipper.update();
        tft.fillRect(0, 40, tftWidth, tftHeight - 40, bruceConfig.bgColor);

        tft.setTextColor(bruceConfig.priColor);
        if (type == 0) tft.drawCentreString(customSsid, tftWidth / 2, 80, 2);
        else if (type == 1) tft.drawCentreString("Random", tftWidth / 2, 80, 2);
        else if (type == 2) {
            tft.drawCentreString("RickRoll", tftWidth / 2, 60, 2);
            tft.drawCentreString("Never gonna give you up", tftWidth / 2, 100, 1);
        }

        if (check(AnyKeyPress)) {
            flipper.sendCommand("STOP");
            isRunning = false;
        }
        delay(100);
    }
}

void FlipperMenu::configAPScene() {
    while(true) {
        options = {
            {_("AP NAME: ") + ssidAP, [&]() { 
                ssidAP = keyboard(ssidAP, 32, "Set AP Name");
            }},
            {_("SECURITY: ") + securityType[securityAP], [&]() { securityAP = (securityAP + 1) % 2; }},
            
            {_("Password: ") + passwordAP, [&]() { 
                if(securityAP == 1) { 
                    passwordAP = keyboard(passwordAP, 63, "Set Password");
                }
            }},
            
            {_("Channel: ") + channelList[channelAP], [&]() { channelAP = (channelAP + 1) % 59; }},
            {_("Portal: ") + captive_portals[captivePortal], [&]() { captivePortal = (captivePortal + 1) % 6; }},
            {_("Mac: ") + mac_type_str[macType], [&]() {
                macType = (macType + 1) % 3; 
                applyMacType(macType);
            }},
            
            {_("Custom MAC: ") + customMac, [&]() { 
                if(macType == 2) {
                    customMac = keyboard(customMac, 17, "Set Custom MAC");
                }
            }},
            
            {_("[ START ]"), [&]() {
                applyMacType(macType);
                if(securityAP == 1) {
                    flipper.sendCommand("PASSWORD " + passwordAP);
                    delay(200);
                }
                flipper.sendCommand("APMAC " + customMac);
                delay(200);
                flipper.sendCommand("CHANNEL " + channelList[channelAP]);
                delay(200);
                flipper.sendCommand("PORTAL " + String(captivePortal));
                delay(200);
                flipper.sendCommand("APSTART " + ssidAP);

                evilCredsScene(); 
            }},
            {_("Back"), [&]() { returnToMenu = true; }}
        };

        drawMainBorderWithTitle(_("Config Fake AP"));
        loopOptions(options);
        if(returnToMenu) { returnToMenu = false; break; }
    }
}

void FlipperMenu::helpScene() {
    drawMainBorderWithTitle(_("Help"));
    tft.setTextColor(bruceConfig.priColor);
    tft.setCursor(5, 50);
    tft.print("https://github.com/\ngorebrau/delfyRTL");

    while(true) {
        if(check(AnyKeyPress)) break;
        delay(50);
    }
}

void FlipperMenu::drawIconImg() { drawIcon(1.0); }
bool FlipperMenu::getTheme() { return true; }
void FlipperMenu::drawIcon(float scale) {}