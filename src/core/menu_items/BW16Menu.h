#pragma once
#include "modules/bw16/BW16Manager.h"
#include <DNSServer.h>
#include <MenuItemInterface.h>
#include <WebServer.h>
#include <WiFi.h>
#include <functional>

class BW16Menu : public MenuItemInterface {
public:
    BW16Menu() : MenuItemInterface("BW16") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    void drawIconImg() override;
    bool getTheme() override;

private:
    void scanForTarget();
    void showTargetOptions(BW16Network net);
    void showMultiAttackMenu();
    void runAttackMenu(String title, String status, std::function<void()> startCmd);
    void runEvilTwinAttack(BW16Network targetNet);
};