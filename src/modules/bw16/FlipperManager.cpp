#include "FlipperManager.h"
#include "core/settings.h" 

FlipperManager flipper;

FlipperManager::FlipperManager() {
    serial = &Serial2; // Dùng chung Serial2 với BW16
    lastCmdTime = 0;
}

void FlipperManager::begin() {
    // FIX BUG: Nới rộng bộ đệm để không rớt chữ khi BW16 nhả cả đống LIST. Phải set TRƯỚC begin.
    serial->setRxBufferSize(2048); 
    
    serial->begin(115200, SERIAL_8N1, bruceConfigPins.uart_bus.rx, bruceConfigPins.uart_bus.tx);
    rxBuffer = "";
    capturedCredentials = "";
    lastError = "";
    scanFinished = false;
    
    networks.clear();
    cmdQueue.clear();  // Xóa sạch hàng đợi lệnh khi khởi động
    lastCmdTime = 0;
    delay(10);
}

void FlipperManager::update() {
    // 1. ĐỌC UART NON-BLOCKING (Parser vòng lặp)
    while (serial->available()) {
        char c = serial->read();
        if (c == '\n') {
            rxBuffer.trim();
            if (rxBuffer.length() > 0) {
                processLine(rxBuffer);
            }
            rxBuffer = "";
        } else if (c != '\r') {
            rxBuffer += c;
        }
    }

    // 2. MÁY TRẠNG THÁI XỬ LÝ HÀNG ĐỢI LỆNH (Thay thế delay)
    if (!cmdQueue.empty()) {
        if (millis() - lastCmdTime >= CMD_DELAY_MS) {
            sendCommand(cmdQueue.front());     // Gửi lệnh ở đầu hàng đợi
            cmdQueue.erase(cmdQueue.begin());  // Xóa lệnh vừa gửi
            lastCmdTime = millis();            // Đánh dấu thời gian gửi
        }
    }
}

void FlipperManager::sendCommand(String cmd) {
    serial->print(cmd + "\n");
    serial->flush(); // Đẩy dữ liệu ra chân TX ngay lập tức, khắc phục lỗi kẹt lệnh STOP
}

// =========================================================
// THỰC THI CHI TIẾT CÁC LỆNH UART (MAPPING)
// =========================================================
void FlipperManager::scanNetworks() { 
    networks.clear();       // Dọn mảng cũ trước khi scan
    scanFinished = false; 
    sendCommand("SCAN"); 
}

void FlipperManager::listNetworks() { 
    // KHÔNG ĐƯỢC CLEAR MẢNG Ở ĐÂY để tránh lỗi UI nhấp nháy "no network found".
    sendCommand("LIST"); 
}

void FlipperManager::stopAll() { 
    cmdQueue.clear();       // Xóa trắng hàng đợi lệnh đang dang dở (nếu có)
    sendCommand("STOP"); 
}

void FlipperManager::stopTarget(int id) { 
    cmdQueue.clear(); 
    sendCommand("STOP " + String(id)); 
}

void FlipperManager::deauth(int id)          { sendCommand("DEAUTH " + String(id)); }
void FlipperManager::evilTwin(int id)        { sendCommand("EVIL " + String(id)); }
void FlipperManager::setApMac(String mac)    { sendCommand("APMAC " + mac); }
void FlipperManager::setReason(int reason)   { sendCommand("REASON " + String(reason)); }
void FlipperManager::setPortal(int portal)   { sendCommand("PORTAL " + String(portal)); }
void FlipperManager::setBssid(String bssid)  { sendCommand("BSSID " + bssid); }
void FlipperManager::beaconRandom()          { sendCommand("RANDOM"); }
void FlipperManager::beaconRickRoll()        { sendCommand("RICKROLL"); }
void FlipperManager::setPassword(String p)   { sendCommand("PASSWORD " + p); }
void FlipperManager::setChannel(String ch)   { sendCommand("CHANNEL " + ch); }
void FlipperManager::startApNode(String s)   { sendCommand("APSTART " + s); }

// =========================================================
// HÀM GỘP CHUỖI LỆNH (Macro) - Sử dụng Hàng đợi (Queue)
// =========================================================
void FlipperManager::startEvilTwin(int id, int reason, int portal, String mac) {
    cmdQueue.clear(); // Hủy các thao tác cũ
    cmdQueue.push_back("APMAC " + mac);
    cmdQueue.push_back("REASON " + String(reason));
    cmdQueue.push_back("PORTAL " + String(portal));
    cmdQueue.push_back("EVIL " + String(id));
    lastCmdTime = 0; // Kích hoạt lệnh đầu tiên chạy ngay lập tức ở chu kỳ update() tiếp theo
}

void FlipperManager::sendBeacon(int type, String customSsid) {
    cmdQueue.clear();
    if (type == 1) cmdQueue.push_back("RANDOM");
    else if (type == 2) cmdQueue.push_back("RICKROLL");
    else cmdQueue.push_back("BSSID " + customSsid);
    lastCmdTime = 0;
}

void FlipperManager::startAP(String ssid, String password, int security, String channel, int portal, String mac) {
    cmdQueue.clear();
    if (security == 1) { // 1 = Chuẩn WPA
        cmdQueue.push_back("PASSWORD " + password); 
    }
    cmdQueue.push_back("APMAC " + mac);
    cmdQueue.push_back("CHANNEL " + channel);
    cmdQueue.push_back("PORTAL " + String(portal));
    cmdQueue.push_back("APSTART " + ssid);
    lastCmdTime = 0; 
}

// =========================================================
// PARSER (Đọc phản hồi từ DelfyRTL)
// =========================================================

void FlipperManager::processLine(String line) {
    if (line.startsWith("AP:")) {
        // Tách chuỗi theo định dạng: AP:ID|SSID|BSSID|Channel|Security|RSSI
        String data = line.substring(3);
        int p1 = data.indexOf('|');
        int p2 = data.indexOf('|', p1 + 1);
        int p3 = data.indexOf('|', p2 + 1);
        int p4 = data.indexOf('|', p3 + 1);
        int p5 = data.indexOf('|', p4 + 1);

        // FIX BUG: Check 5 vạch pipe để né rác do tràn đệm
        if (p1 != -1 && p2 != -1 && p3 != -1 && p4 != -1 && p5 != -1) {
            int incoming_id = data.substring(0, p1).toInt();
            
            // LOGIC MỚI: Nhận được thằng đầu tiên của mảng mới thì đi dọn rác mảng cũ (Tránh UI nhấp nháy)
            if (incoming_id == 0) {
                networks.clear();
            }

            FlipperAP net;
            net.id = incoming_id;
            net.ssid = data.substring(p1 + 1, p2);
            net.bssid = data.substring(p2 + 1, p3);
            net.channel = data.substring(p3 + 1, p4).toInt();
            net.security = data.substring(p4 + 1, p5).toInt();
            net.rssi = data.substring(p5 + 1).toInt();
            networks.push_back(net);
        }
    } 
    else if (line.startsWith("SCAN:OK")) {
        scanFinished = true;
        listNetworks(); 
    } 
    else if (line.startsWith("EV:")) {
        String token = line.substring(3);
        int pipeIdx = token.indexOf('|');
        if(pipeIdx != -1) {
             capturedCredentials += token.substring(pipeIdx + 1) + "\n";
        } else {
             capturedCredentials += token + "\n";
        }
    } 
    else if (line.startsWith("ERROR:")) {
        lastError = line.substring(6);
    }
}