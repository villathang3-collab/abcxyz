#ifndef __MAIN_MENU_H__
#define __MAIN_MENU_H__

#include "core/menu_items/BW16Menu.h"
#include "core/menu_items/BleMenu.h"
#include "core/menu_items/ClockMenu.h"
#include "core/menu_items/ConfigMenu.h"
#include "core/menu_items/ConnectMenu.h"
#include "core/menu_items/EthernetMenu.h"
#include "core/menu_items/FMMenu.h"
#include "core/menu_items/FileMenu.h"
#include "core/menu_items/GpsMenu.h"
#include "core/menu_items/IRMenu.h"
#include "core/menu_items/LoRaMenu.h"
#include "core/menu_items/NRF24.h"
#include "core/menu_items/OthersMenu.h"
#include "core/menu_items/RFIDMenu.h"
#include "core/menu_items/RFMenu.h"
#include "core/menu_items/ScriptsMenu.h"
#include "core/menu_items/WifiMenu.h"
#include "menu_items/NaoyaMenu.h"
#include <MenuItemInterface.h>
class MainMenu {
public:
    FileMenu fileMenu;
    BleMenu bleMenu;
    ClockMenu clockMenu;
    ConnectMenu connectMenu;
    ConfigMenu configMenu;
    FMMenu fmMenu;
    GpsMenu gpsMenu;
    BW16Menu bw16Menu;
    NaoyaMenu naoyaMenu;
    IRMenu irMenu;
    NRF24Menu nrf24Menu;
    OthersMenu othersMenu;
    RFIDMenu rfidMenu;
    RFMenu rfMenu;
    ScriptsMenu scriptsMenu;
    WifiMenu wifiMenu;
#if !defined(LITE_VERSION)
    LoRaMenu loraMenu;
    EthernetMenu ethernetMenu;
#endif

    MainMenu();
    ~MainMenu();

    void begin(void);
    std::vector<MenuItemInterface *> getItems(void) { return _menuItems; }
    void hideAppsMenu();

private:
    int _currentIndex = 0;
    int _totalItems = 0;
    std::vector<MenuItemInterface *> _menuItems;
};
extern MainMenu mainMenu;

#endif
