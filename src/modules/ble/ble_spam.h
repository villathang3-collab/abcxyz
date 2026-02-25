#pragma once
#include <Arduino.h>

enum EBLEPayloadType { Microsoft, SourApple, AppleJuice, Samsung, Google };

// --- BIẾN TOÀN CỤC ---
// 0=FAST (10ms), 1=MEDIUM (500ms), 2=SLOW (1000ms)
extern int g_speed_mode;

// Hàm đổi tốc độ (Gọi từ Menu)
void toggleSpamSpeed();

// Các hàm chính
void aj_adv(int ble_choice);
void executeSpam(EBLEPayloadType type);

// Hàm iBeacon (Có tham số mặc định CHỈ Ở ĐÂY)
void ibeacon(
    const char *DeviceName = "Bruce iBeacon",
    const char *BEACON_UUID = "8ec76ea3-6668-48da-9866-75be8bc86f4d", int ManufacturerId = 0x4C00
);
