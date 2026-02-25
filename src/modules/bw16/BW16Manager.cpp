#include "BW16Manager.h"
// Không include display.h để đảm bảo Manager không vẽ bậy

BW16Manager bw16;

BW16Manager::BW16Manager() { serial = &Serial2; }

void BW16Manager::begin() {
    // Dùng chân mặc định SERIAL_RX/TX (như bản cũ của bạn)
    serial->begin(115200, SERIAL_8N1, SERIAL_RX, SERIAL_TX);
    delay(5);
}

void BW16Manager::stop() {
    serial->println("stop");
    delay(50);
    while (serial->available()) serial->read();
}

void BW16Manager::selectTarget(int id) {
    serial->print("sl:");
    serial->println(id);
    delay(50);
}

void BW16Manager::atkDeauth() { serial->println("atk:deauth"); }
void BW16Manager::atkDeauthFlood() { serial->println("atk:flood"); }
void BW16Manager::atkBeacon() { serial->println("atk:beacon"); }
void BW16Manager::sendRawCommand(String cmd) { serial->println(cmd); }

void BW16Manager::scanNetworks() {
    networks.clear();
    while (serial->available()) serial->read(); // Xóa buffer cũ

    serial->println("sc");

    long start = millis();
    String buffer = "";

    // Scan im lặng (không vẽ màn hình)
    while (millis() - start < 8000) {
        if (serial->available()) {
            char c = (char)serial->read();
            if (c == '\n') {
                buffer.trim();
                // Nếu là heartbeat 'sci' -> Reset timeout, không làm gì cả
                if (buffer == "sci") {
                    start = millis();
                }
                // Nếu là dữ liệu mạng -> Parse
                else if (buffer.startsWith("[")) {
                    parseLine(buffer);
                    start = millis();
                }
                buffer = "";
            } else if (c != '\r') buffer += c;
        }

        if (!networks.empty() && millis() - start > 1500) break;
        yield();
    }
}

void BW16Manager::parseLine(String line) {
    try {
        int idEnd = line.indexOf(']');
        if (idEnd == -1) return;
        int id = line.substring(1, idEnd).toInt();

        String rest = line.substring(idEnd + 1);
        rest.trim();
        int firstTab = rest.indexOf('\t');
        if (firstTab != -1) {
            int channel = rest.substring(0, firstTab).toInt();
            String temp = rest.substring(firstTab + 1);
            int secondTab = temp.indexOf('\t');
            int rssi = temp.substring(0, secondTab).toInt();
            temp = temp.substring(secondTab + 1);
            int thirdTab = temp.indexOf('\t');
            String enc = temp.substring(0, thirdTab);
            String ssid = temp.substring(thirdTab + 1);
            ssid.trim();

            BW16Network net;
            net.id = id;
            net.channel = channel;
            net.rssi = rssi;
            net.ssid = ssid;
            net.encType = enc;
            networks.push_back(net);
        }
    } catch (...) {}
}
