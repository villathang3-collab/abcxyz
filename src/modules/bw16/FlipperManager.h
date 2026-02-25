#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>

// Cấu trúc lưu trữ dữ liệu Wi-Fi quét được từ DelfyRTL
struct FlipperAP {
    int id;
    String ssid;
    String bssid;
    int channel;
    int security;
    int rssi;
    bool deauth_active = false;
};

class FlipperManager {
private:
    HardwareSerial *serial;
    String rxBuffer;
    void processLine(String line);

    // =======================================================
    // MÁY TRẠNG THÁI (STATE MACHINE) & HÀNG ĐỢI LỆNH
    // =======================================================
    std::vector<String> cmdQueue;    // Hàng đợi chứa các lệnh cần gửi tuần tự
    uint32_t lastCmdTime;            // Lưu thời gian gửi lệnh cuối cùng
    const uint32_t CMD_DELAY_MS = 200; // Độ trễ 200ms tương đương Furi_delay bản gốc

public:
    std::vector<FlipperAP> networks;
    String capturedCredentials;
    String lastError;
    bool scanFinished = false;

    FlipperManager();
    void begin();
    
    // Hàm update phải được gọi liên tục trong loop() để xử lý UART & Hàng đợi
    void update(); 
    
    // Hàm gửi lệnh chung và ép đẩy bộ đệm (flush)
    void sendCommand(String cmd);

    // =======================================================
    // ÁNH XẠ 100% CÁC LỆNH UART TỪ SOURCE FLIPPER (delfyrtl.c)
    // =======================================================
    void scanNetworks();              // Lệnh: SCAN\n
    void listNetworks();              // Lệnh: LIST\n
    void stopAll();                   // Lệnh: STOP\n
    void stopTarget(int id);          // Lệnh: STOP %d\n
    void deauth(int id);              // Lệnh: DEAUTH %d\n
    void evilTwin(int id);            // Lệnh: EVIL %d\n
    void setApMac(String mac);        // Lệnh: APMAC %s\n
    void setReason(int reason);       // Lệnh: REASON %d\n
    void setPortal(int portal);       // Lệnh: PORTAL %d\n
    void setBssid(String bssid);      // Lệnh: BSSID %s\n
    void beaconRandom();              // Lệnh: RANDOM\n
    void beaconRickRoll();            // Lệnh: RICKROLL\n
    void setPassword(String pwd);     // Lệnh: PASSWORD %s\n
    void setChannel(String ch);       // Lệnh: CHANNEL %s\n
    void startApNode(String ssid);    // Lệnh: APSTART %s\n

    // =======================================================
    // HÀM GỘP THỰC THI NHIỀU LỆNH (NON-BLOCKING)
    // =======================================================
    void startEvilTwin(int id, int reason, int portal, String mac);
    void sendBeacon(int type, String customSsid = ""); 
    void startAP(String ssid, String password, int security, String channel, int portal, String mac);
};

// Khởi tạo biến toàn cục
extern FlipperManager flipper;