#pragma once
#include <Arduino.h>

// Định nghĩa màu cho dễ nhìn (W=Trắng, B=Đen/Trong suốt)
#define W 0xFFFF 
#define B 0x0000 

// Kích thước chuẩn
#define ICON_W 10
#define ICON_H 10

// 1. ICON SCAN (Hình Scan_Wifi.png - Cột phát sóng và các sóng cung)
#define icon_scan_w 10
#define icon_scan_h 10
const uint16_t icon_scan[100] PROGMEM = {
    B, B, B, B, W, W, B, B, B, B, // Sóng ngoài cùng
    B, B, W, W, B, B, W, W, B, B,
    B, W, B, B, B, B, B, B, W, B,
    B, B, B, W, W, W, W, B, B, B, // Sóng trong
    B, B, W, B, B, B, B, W, B, B,
    B, B, B, B, W, W, B, B, B, B, // Cột phát
    B, B, B, B, W, W, B, B, B, B,
    B, B, B, B, W, W, B, B, B, B,
    B, B, B, B, W, W, B, B, B, B,
    B, B, B, B, W, W, B, B, B, B
};

// 2. ICON LIST (Hình List_Wifi.png - Khung danh sách 3 gạch)
#define icon_list_w 10
#define icon_list_h 10
const uint16_t icon_list[100] PROGMEM = {
    W, W, W, W, W, W, W, W, W, W, // Viền trên
    W, B, B, B, B, B, B, B, B, W,
    W, B, W, W, W, W, W, W, B, W, // Dòng 1
    W, B, B, B, B, B, B, B, B, W,
    W, B, W, W, W, W, W, W, B, W, // Dòng 2
    W, B, B, B, B, B, B, B, B, W,
    W, B, W, W, W, W, W, W, B, W, // Dòng 3
    W, B, B, B, B, B, B, B, B, W,
    W, B, B, B, B, B, B, B, B, W,
    W, W, W, W, W, W, W, W, W, W  // Viền dưới
};

// 3. ICON BEACON / Random AP (Hình random_AP.png - 3 ăng ten thẳng)
#define icon_beacon_w 10
#define icon_beacon_h 10
const uint16_t icon_beacon[100] PROGMEM = {
    B, B, B, B, B, B, B, B, B, B, // Đệm trên
    B, B, W, B, B, W, B, B, W, B, // 3 ăng ten
    B, B, W, B, B, W, B, B, W, B,
    B, B, W, B, B, W, B, B, W, B,
    B, B, W, W, W, W, W, W, W, B, // Thân máy
    B, W, W, W, W, W, W, W, W, W, // Thân máy + mấu hai bên
    B, B, W, W, W, W, W, W, W, B,
    B, B, W, W, W, W, W, W, W, B,
    B, B, W, W, W, W, W, W, W, B,
    B, B, B, B, B, B, B, B, B, B  // Đệm dưới
};

// 4. ICON ATTACK / Custom AP (Hình Custom_AP.png - 2 ăng ten chéo V)
#define icon_attack_w 10
#define icon_attack_h 10
const uint16_t icon_attack[100] PROGMEM = {
    B, B, B, B, B, B, B, B, B, B, // Đệm trên
    B, B, W, B, B, B, B, B, W, B, // Đầu 2 ăng ten chéo
    B, B, B, W, B, B, B, W, B, B,
    B, B, B, B, W, B, W, B, B, B, // Gốc ăng ten
    B, B, W, W, W, W, W, W, W, B, // Thân máy
    B, W, W, W, W, W, W, W, W, W, // Thân máy + mấu hai bên
    B, B, W, W, W, W, W, W, W, B,
    B, B, W, W, W, W, W, W, W, B,
    B, B, W, W, W, W, W, W, W, B,
    B, B, B, B, B, B, B, B, B, B  // Đệm dưới
};

// 6. ICON HELP (Hình help.png - Dấu chấm hỏi)
#define icon_help_w 10
#define icon_help_h 10
const uint16_t icon_help[100] PROGMEM = {
    B, B, B, W, W, W, W, B, B, B, // Đường cong trên
    B, B, W, B, B, B, B, W, B, B,
    B, B, W, B, B, B, B, W, B, B,
    B, B, B, B, B, B, W, B, B, B, // Đường cong xuống
    B, B, B, B, B, W, B, B, B, B,
    B, B, B, B, W, B, B, B, B, B, // Thân dấu hỏi
    B, B, B, B, W, B, B, B, B, B,
    B, B, B, B, B, B, B, B, B, B, // Dấu cách
    B, B, B, B, W, B, B, B, B, B, // Dấu chấm
    B, B, B, B, B, B, B, B, B, B
};

// 7. ICON STOP / EXIT (Hình exit.png - Cánh cửa có tay nắm)
#define icon_stop_w 10
#define icon_stop_h 10
const uint16_t icon_stop[100] PROGMEM = {
    B, W, W, W, W, W, W, W, B, B, // Cạnh cửa trên
    B, W, B, B, B, B, B, W, B, B,
    B, W, B, B, B, B, B, W, B, B,
    B, W, B, B, B, B, B, W, B, B,
    B, W, B, B, B, W, B, W, B, B, // Tay nắm cửa
    B, W, B, B, B, W, B, W, B, B,
    B, W, B, B, B, B, B, W, B, B,
    B, W, B, B, B, B, B, W, B, B,
    B, W, B, B, B, B, B, W, B, B,
    B, W, W, W, W, W, W, W, B, B  // Cạnh cửa dưới
};