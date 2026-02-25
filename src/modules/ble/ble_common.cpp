#include "ble_common.h"
#include "core/Translation.h" // Thêm include Translation
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "esp_mac.h"

#define SERVICE_UUID "1bc68b2a-f3e3-11e9-81b4-2a2ae2dbcce4"
#define CHARACTERISTIC_RX_UUID "1bc68da0-f3e3-11e9-81b4-2a2ae2dbcce4"
#define CHARACTERISTIC_TX_UUID "1bc68efe-f3e3-11e9-81b4-2a2ae2dbcce4"

#if __has_include(<NimBLEExtAdvertising.h>)
#define NIMBLE_V2_PLUS 1
#endif

#define SCANTIME 5
#define SCANTYPE ACTIVE
#define SCAN_INT 100
#define SCAN_WINDOW 99

#define ENDIAN_CHANGE_U16(x) ((((x) & 0xFF00) >> 8) + (((x) & 0xFF) << 8))

BLEServer *pServer = NULL;
BLEService *pService = NULL;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;
bool bleDataTransferEnabled = false;

bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) { deviceConnected = true; };
    void onDisconnect(BLEServer *pServer) { deviceConnected = false; }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    NimBLEAttValue data;
    void onWrite(NimBLECharacteristic *pCharacteristic) { data = pCharacteristic->getValue(); }
};

int scanTime = SCANTIME; // In seconds
BLEScan *pBLEScan;

uint8_t sta_mac[6];
char strID[18];
char strAddl[200];

void ble_info(String name, String address, String signal) {
    drawMainBorder();
    tft.setTextColor(bruceConfig.priColor);

    // Title translated to Information
    tft.drawCentreString(Translation::tr("-=Information=-"), tftWidth / 2, 28, SMOOTH_FONT);

    // Labels translated
    tft.drawString(Translation::tr("Name: ") + name, 10, 48);
    tft.drawString(Translation::tr("Address: ") + address, 10, 66);
    tft.drawString(Translation::tr("Signal: ") + String(signal) + " dBm", 10, 84);

    // Footer instructions translated
    tft.drawCentreString(
        Translation::tr("   Press ") + String(BTN_ALIAS) + Translation::tr(" to act"),
        tftWidth / 2,
        tftHeight - 20,
        1
    );

    delay(300);
    while (!check(SelPress)) {
        while (!check(SelPress)) { yield(); }
        returnToMenu = true;
        break;
    }
}

#ifdef NIMBLE_V2_PLUS
class AdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
#else
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
#endif
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
        String bt_title;
        String bt_name;
        String bt_address;
        String bt_signal;

        bt_name = advertisedDevice->getName().c_str();
        bt_title = advertisedDevice->getName().c_str();
        bt_address = advertisedDevice->getAddress().toString().c_str();
        bt_signal = String(advertisedDevice->getRSSI());

        if (bt_title.isEmpty()) bt_title = bt_address;
        if (bt_name.isEmpty()) bt_name = Translation::tr("<no name>");

        if (options.size() < 250)
            options.emplace_back(bt_title.c_str(), [=]() { ble_info(bt_name, bt_address, bt_signal); });
        else {
            Serial.println(Translation::tr("Memory low, stopping BLE scan..."));
            pBLEScan->stop();
        }
    }
};

void ble_scan_setup() {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
#ifdef NIMBLE_V2_PLUS
    pBLEScan->setScanCallbacks(new NimBLEScanCallbacks());
#else
    pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
#endif

    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(SCAN_INT);
    pBLEScan->setWindow(SCAN_WINDOW);

#ifdef NIMBLE_V2_PLUS
    esp_read_mac(sta_mac, ESP_MAC_BT);
#else
    esp_read_mac(sta_mac, ESP_MAC_BT);
#endif

    sprintf(
        strID,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        sta_mac[0],
        sta_mac[1],
        sta_mac[2],
        sta_mac[3],
        sta_mac[4],
        sta_mac[5]
    );
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void ble_scan() {
    displayTextLine(Translation::tr("Scanning.."));

    options = {};
    ble_scan_setup();
#ifdef NIMBLE_V2_PLUS
    BLEScanResults foundDevices = pBLEScan->getResults(scanTime * 1000, false);
    for (int i = 0; i < foundDevices.getCount(); i++) {
        const NimBLEAdvertisedDevice *advertisedDevice = foundDevices.getDevice(i);
        String bt_title;
        String bt_name;
        String bt_address;
        String bt_signal;

        bt_name = advertisedDevice->getName().c_str();
        bt_title = advertisedDevice->getName().c_str();
        bt_address = advertisedDevice->getAddress().toString().c_str();
        bt_signal = String(advertisedDevice->getRSSI());

        if (bt_title.isEmpty()) bt_title = bt_address;
        if (bt_name.isEmpty()) bt_name = Translation::tr("<no name>");

        if (options.size() < 250)
            options.emplace_back(bt_title.c_str(), [=]() { ble_info(bt_name, bt_address, bt_signal); });
        else {
            Serial.println(Translation::tr("Memory low, stopping BLE scan..."));
            pBLEScan->stop();
        }
    }
#else
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
#endif

    addOptionToMainMenu();

    loopOptions(options);
    options.clear();

    pBLEScan->clearResults();
}

bool initBLEServer() {
    uint64_t chipid = ESP.getEfuseMac();
    String blename = "Bruce-" + String((uint8_t)(chipid >> 32), HEX);

    BLEDevice::init(blename.c_str());
    pServer = BLEDevice::createServer();

    pServer->setCallbacks(new MyServerCallbacks());
    pService = pServer->createService(SERVICE_UUID);
    pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_RX_UUID, NIMBLE_PROPERTY::NOTIFY);

    pTxCharacteristic->addDescriptor(new NimBLE2904());
    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_TX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new MyCallbacks());

    return true;
}

void disPlayBLESend() {
    uint8_t senddata[2] = {0};
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();
    tft.setTextSize(1);

    pService->start();
    pServer->getAdvertising()->start();

    uint64_t chipid = ESP.getEfuseMac();
    String blename = "Bruce-" + String((uint8_t)(chipid >> 32), HEX);

    BLEConnected = true;

    bool wasConnected = false;
    bool first_run = true;
    while (!check(EscPress)) {
        if (deviceConnected) {
            if (!wasConnected) {
                tft.fillRect(10, 26, tftWidth - 20, tftHeight - 36, TFT_BLACK);
                drawBLE_beacon(180, 28, TFT_BLUE);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.setTextSize(FM);
                tft.setCursor(12, 50);
                tft.printf("%s\n", Translation::tr("BLE Sending...").c_str());
                tft.setTextSize(FM);
            }
            tft.fillRect(10, 100, tftWidth - 20, 28, TFT_BLACK);
            tft.setCursor(12, 100);
            if (senddata[0] % 4 == 0) {
                tft.printf("0x%02X>    ", senddata[0]);
            } else if (senddata[0] % 4 == 1) {
                tft.printf("0x%02X>>   ", senddata[0]);
            } else if (senddata[0] % 4 == 2) {
                tft.printf("0x%02X >>  ", senddata[0]);
            } else if (senddata[0] % 4 == 3) {
                tft.printf("0x%02X  >  ", senddata[0]);
            }

            senddata[1]++;
            if (senddata[1] > 3) {
                senddata[1] = 0;
                senddata[0]++;
                pTxCharacteristic->setValue(senddata, 1);
                pTxCharacteristic->notify();
            }
            wasConnected = true;
        } else {
            if (wasConnected || first_run) {
                first_run = false;
                tft.fillRect(10, 26, tftWidth - 20, tftHeight - 36, TFT_BLACK);
                tft.setTextSize(FM);
                tft.setCursor(12, 50);
                tft.setTextColor(TFT_RED);
                tft.printf("%s\n", Translation::tr("BLE Disconnected").c_str());
                tft.setCursor(12, 75);
                tft.setTextColor(tft.color565(18, 150, 219));

                tft.printf("%s %s\n", Translation::tr("Name:").c_str(), blename.c_str());
                tft.setCursor(12, 100);
                tft.printf("UUID: 1bc68b2a\n");
                drawBLE_beacon(180, 40, TFT_DARKGREY);
            }
            wasConnected = false;
        }
    }

    tft.setTextColor(TFT_WHITE);
    pService->~NimBLEService();
    pServer->getAdvertising()->stop();
    BLEDevice::deinit();
    BLEConnected = false;
}

static bool is_ble_inited = false;

void ble_test() {
    initBLEServer();
    delay(100);
    is_ble_inited = true;

    disPlayBLESend();
}
