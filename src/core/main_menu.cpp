#include "main_menu.h"
#include "display.h"
#include "utils.h"
#include <globals.h>

MainMenu::MainMenu() {
    _menuItems.push_back(&wifiMenu);
    _menuItems.push_back(&bleMenu);
#if !defined(LITE_VERSION)
    _menuItems.push_back(&ethernetMenu);
#endif
    _menuItems.push_back(&rfMenu);
    _menuItems.push_back(&naoyaMenu);
    _menuItems.push_back(&rfidMenu);
    _menuItems.push_back(&bw16Menu);
    _menuItems.push_back(&irMenu);
#if defined(FM_SI4713) && !defined(LITE_VERSION)
    _menuItems.push_back(&fmMenu);
#endif
    _menuItems.push_back(&fileMenu);
    _menuItems.push_back(&gpsMenu);
    _menuItems.push_back(&nrf24Menu);
#if !defined(LITE_VERSION)
#if !defined(DISABLE_INTERPRETER)
    _menuItems.push_back(&scriptsMenu);
#endif
    _menuItems.push_back(&loraMenu);
#endif
    _menuItems.push_back(&othersMenu);
    _menuItems.push_back(&clockMenu);
#if !defined(LITE_VERSION)
    _menuItems.push_back(&connectMenu);
#endif
    _menuItems.push_back(&configMenu);

    _totalItems = _menuItems.size();
}

MainMenu::~MainMenu() {}

void MainMenu::begin(void) {
    returnToMenu = false;
    options = {};

    std::vector<String> l = bruceConfig.disabledMenus;
    for (int i = 0; i < _totalItems; i++) {
        String itemName = _menuItems[i]->getName();
        if (find(l.begin(), l.end(), itemName) == l.end()) { // If menu item is not disabled
            options.push_back(
                {// selected lambda
                 _menuItems[i]->getName(),
                 [this, i]() { _menuItems[i]->optionsMenu(); },
                 false,                                  // selected = false
                 [](void *menuItem, bool shouldRender) { // render lambda
                     if (!shouldRender) return false;
                     drawMainBorder(false);

                     MenuItemInterface *obj = static_cast<MenuItemInterface *>(menuItem);
                     float scale = float((float)tftWidth / (float)240);
                     if (bruceConfigPins.rotation & 0b01) scale = float((float)tftHeight / (float)135);
                     obj->draw(scale);
#if defined(HAS_TOUCH)
                     TouchFooter();
#endif
                     return true;
                 },
                 _menuItems[i]
                }
            );
        }
    }
    _currentIndex = loopOptions(options, MENU_TYPE_MAIN, "Main Menu", _currentIndex);
};

/*********************************************************************
**  Function: hideAppsMenu
**  Menu to Hide or show menus
**********************************************************************/

void MainMenu::hideAppsMenu() {
    auto items = this->getItems();
RESTART: // using gotos to avoid stackoverflow after many choices
    options.clear();
    for (auto item : items) {
        String label = item->getName();
        std::vector<String> l = bruceConfig.disabledMenus;
        bool enabled = find(l.begin(), l.end(), label) == l.end();
        options.push_back({label, [this, label]() { bruceConfig.addDisabledMenu(label); }, enabled});
    }
    options.push_back({"Show All", [=]() { bruceConfig.disabledMenus.clear(); }, true});
    addOptionToMainMenu();
    loopOptions(options);
    bruceConfig.saveFile();
    if (!returnToMenu) goto RESTART;
}
