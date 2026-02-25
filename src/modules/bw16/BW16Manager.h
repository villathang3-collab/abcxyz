#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>

struct BW16Network {
    int id;
    String ssid;
    int rssi;
    int channel;
    String encType;
    bool selected = false; // Trạng thái chọn
};

class BW16Manager {
private:
    HardwareSerial *serial;
    void parseLine(String line);

public:
    std::vector<BW16Network> networks;

    BW16Manager();
    void begin();
    void stop();
    void scanNetworks();
    void selectTarget(int id);

    // Các hàm tấn công
    void atkDeauth();
    void atkDeauthFlood();
    void atkBeacon();

    // Hàm gửi lệnh thô
    void sendRawCommand(String cmd);
};

extern BW16Manager bw16;
