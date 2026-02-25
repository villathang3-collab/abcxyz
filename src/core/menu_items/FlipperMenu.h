#pragma once
#include "modules/bw16/FlipperManager.h"
#include <MenuItemInterface.h>
#include <vector>

class FlipperMenu : public MenuItemInterface {
public:
    FlipperMenu() : MenuItemInterface("Flipper DelfyRTL") {}

    void optionsMenu(void) override;

    // [BẮT BUỘC] Override các hàm ảo thuần túy để tránh lỗi biên dịch
    void drawIcon(float scale) override;
    void drawIconImg() override;
    bool getTheme() override;

private:
    void mainMenuScene();
    void scanNetworksScene();
    void listNetworksScene();
    void wifiInfoScene(FlipperAP &net);
    void configAttackScene(FlipperAP &net);
    void evilCredsScene();
    void beaconNetworkScene();
    void beaconInfoScene(int type, String customSsid);
    void configAPScene();
    void helpScene();

    int captivePortal = 0;
    int deauthReason = 2;
    int macType = 0;
    String customMac = "00:E0:4C:01:02:03";

    int securityAP = 0;
    int channelAP = 9;
    String ssidAP = "Delfy_AP";
    String passwordAP = "password123";

    String generateRandomMac();
    void applyMacType(int type, FlipperAP *targetNet = nullptr);
};
