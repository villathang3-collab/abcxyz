#include "BleMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/badusb_ble/ducky_typer.h"
#include "modules/ble/ble_common.h"
#include "modules/ble/ble_ninebot.h"
#include "modules/ble/ble_spam.h" // Chứa biến g_speed_mode
#include <globals.h>

void BleMenu::optionsMenu() {
    options.clear();

    // ==========================================================
    // 1. NÚT CHỈNH TỐC ĐỘ SPAM (Đã chuẩn hóa Tiếng Anh)
    // ==========================================================
    String speedText = _("Speed: FAST"); // Mặc định 0
    if (g_speed_mode == 1) speedText = _("Speed: MEDIUM");
    else if (g_speed_mode == 2) speedText = _("Speed: SLOW");

    options.push_back({speedText, [=]() {
                           toggleSpamSpeed(); // Hàm chuyển đổi tốc độ
                           optionsMenu();     // Load lại menu
                           delay(200);
                       }});
    // ==========================================================

    if (BLEConnected) {
        options.push_back({_("Disconnect"), [=]() {
                               BLEDevice::deinit();
                               BLEConnected = false;
                               delete hid_ble;
                               hid_ble = nullptr;
                               if (_Ask_for_restart == 1) _Ask_for_restart = 2;
                           }});
    }

    options.push_back({_("Media Commands"), [=]() { MediaCommands(hid_ble, true); }});
#if !defined(LITE_VERSION)
    options.push_back({_("BLE Scan"), ble_scan});
    options.push_back({_("iBeacon"), [=]() { ibeacon(); }});
    options.push_back({_("Bad BLE"), [=]() { ducky_setup(hid_ble, true); }});
#endif
    options.push_back({_("BLE Keyboard"), [=]() { ducky_keyboard(hid_ble, true); }});

    // SPAM OPTIONS
    options.push_back({_("Applejuice"), lambdaHelper(aj_adv, 0)});
    options.push_back({_("SourApple"), lambdaHelper(aj_adv, 1)});
    options.push_back({_("Spam Windows"), lambdaHelper(aj_adv, 2)});
    options.push_back({_("Spam Samsung"), lambdaHelper(aj_adv, 3)});
    options.push_back({_("Spam Android"), lambdaHelper(aj_adv, 4)});
    options.push_back({_("Spam All"), lambdaHelper(aj_adv, 5)});
    options.push_back({_("Spam Choice"), lambdaHelper(aj_adv, 6)});

#if !defined(LITE_VERSION)
    options.push_back({_("Ninebot"), [=]() { BLENinebot(); }});
#endif
    addOptionToMainMenu();

    // [QUAN TRỌNG] Thêm .c_str() vì loopOptions yêu cầu const char*
    loopOptions(options, MENU_TYPE_SUBMENU, _("Bluetooth").c_str());
}

// ==========================================================
// PHẦN VẼ GIAO DIỆN (Giữ nguyên logic vẽ, chỉ xóa clearIconArea)
// ==========================================================

void BleMenu::drawIconImg() {
    drawImg(
        *bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.ble), 0, imgCenterY, true
    );
}

void BleMenu::drawIcon(float scale) {
    clearIconArea();

    int lineWidth = scale * 5;
    int iconW = scale * 36;
    int iconH = scale * 60;
    int radius = scale * 5;
    int deltaRadius = scale * 10;

    if (iconW % 2 != 0) iconW++;
    if (iconH % 4 != 0) iconH += 4 - (iconH % 4);

    // Vẽ các đường kết nối Bluetooth
    tft.drawWideLine(
        iconCenterX,
        iconCenterY + iconH / 4,
        iconCenterX - iconW,
        iconCenterY - iconH / 4,
        lineWidth,
        bruceConfig.priColor,
        bruceConfig.priColor
    );
    tft.drawWideLine(
        iconCenterX,
        iconCenterY - iconH / 4,
        iconCenterX - iconW,
        iconCenterY + iconH / 4,
        lineWidth,
        bruceConfig.priColor,
        bruceConfig.priColor
    );
    tft.drawWideLine(
        iconCenterX,
        iconCenterY + iconH / 4,
        iconCenterX - iconW / 2,
        iconCenterY + iconH / 2,
        lineWidth,
        bruceConfig.priColor,
        bruceConfig.priColor
    );
    tft.drawWideLine(
        iconCenterX,
        iconCenterY - iconH / 4,
        iconCenterX - iconW / 2,
        iconCenterY - iconH / 2,
        lineWidth,
        bruceConfig.priColor,
        bruceConfig.priColor
    );

    tft.drawWideLine(
        iconCenterX - iconW / 2,
        iconCenterY - iconH / 2,
        iconCenterX - iconW / 2,
        iconCenterY + iconH / 2,
        lineWidth,
        bruceConfig.priColor,
        bruceConfig.priColor
    );

    // Vẽ các cung sóng phát ra
    tft.drawArc(
        iconCenterX,
        iconCenterY,
        2.5 * radius,
        2 * radius,
        210,
        330,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY,
        2.5 * radius + deltaRadius,
        2 * radius + deltaRadius,
        210,
        330,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY,
        2.5 * radius + 2 * deltaRadius,
        2 * radius + 2 * deltaRadius,
        210,
        330,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
}
