#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <Button.h>
// Đã xóa thư viện esp_adc_cal.h để tránh xung đột Driver Legacy vs Driver NG
#include <globals.h>
#include <interface.h>

volatile bool nxtPress = false;
volatile bool prvPress = false;
volatile bool slPress = false;
volatile bool ecPress = false;
volatile bool nxtHold = false;
volatile bool prvHold = false;
Button *btnNext;
Button *btnPrev;
Button *btnOK;
static void cbNextClick(void *, void *);
static void cbNextHold(void *, void *);
static void cbPrevClick(void *, void *);
static void cbPrevHold(void *, void *);
static void cbOKClick(void *, void *);
static void cbOKHold(void *, void *);

void _setup_gpio() {
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
    pinMode(OK_BTN, INPUT_PULLUP);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LED_OFF);
    pinMode(ADC_PIN, INPUT);
    analogReadResolution(12);

    pinMode(CHARGE_PIN, INPUT);

    button_config_t cfgNext = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 300,
        .short_press_time = 40,
        .gpio_button_config = {
                               .gpio_num = DW_BTN,
                               .active_level = 0,
                               },
    };
    button_config_t cfgPrev = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 300,
        .short_press_time = 40,
        .gpio_button_config = {
                               .gpio_num = UP_BTN,
                               .active_level = 0,
                               },
    };
    button_config_t cfgOK = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 1500,
        .short_press_time = 40,
        .gpio_button_config = {
                               .gpio_num = OK_BTN,
                               .active_level = 0,
                               },
    };
    btnNext = new Button(cfgNext);
    btnPrev = new Button(cfgPrev);
    btnOK = new Button(cfgOK);
    btnNext->attachSingleClickEventCb(cbNextClick, nullptr);
    btnNext->attachLongPressStartEventCb(cbNextHold, nullptr);
    btnPrev->attachSingleClickEventCb(cbPrevClick, nullptr);
    btnPrev->attachLongPressStartEventCb(cbPrevHold, nullptr);
    btnOK->attachSingleClickEventCb(cbOKClick, nullptr);
    btnOK->attachLongPressStartEventCb(cbOKHold, nullptr);
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;

    bruceConfigPins.irRx = RXLED;
    bruceConfigPins.irTx = TXLED;
}

void _post_setup_gpio() {
    // Cấu hình chân GPIO 7 (DIN của I2S) là OUTPUT và kéo xuống LOW
    // để tắt tiếng/tránh nhiễu khi chưa phát nhạc
    pinMode(7, OUTPUT); // Hoặc dùng I2S_DIN_PIN nếu đã include định nghĩa
    digitalWrite(7, LOW);
}

void cbNextClick(void *, void *) { nxtPress = true; }
void cbNextHold(void *, void *) { nxtHold = true; }
void cbPrevClick(void *, void *) { prvPress = true; }
void cbPrevHold(void *, void *) { prvHold = true; }
void cbOKClick(void *, void *) { slPress = true; }
void cbOKHold(void *, void *) { ecPress = true; }

// --- ĐÃ SỬA HÀM NÀY ĐỂ KHẮC PHỤC LỖI CONFLICT ---
int getBattery() {
    // Thay vì dùng esp_adc_cal (Legacy), ta dùng hàm chuẩn Arduino (New Driver)
    // analogReadMilliVolts trả về trực tiếp số mV đã được căn chỉnh (calibrated)
    uint32_t voltage_mv = analogReadMilliVolts(ADC_PIN);

    // Nhân 2 cho mạch phân áp (giả định mạch chia áp 1/2 như code cũ)
    uint32_t voltage = voltage_mv * 2;

    voltage = constrain(voltage, 3200, 4200);
    int percent = map(voltage, 3200, 4200, 0, 100);
    return constrain(percent, 0, 100);
}

bool isCharging() { return digitalRead(CHARGE_PIN) == HIGH; }

void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
        return;
    }
    int bl = MINBRIGHT + ((255 - MINBRIGHT) * brightval / 100);
    analogWrite(TFT_BL, bl);
}

void InputHandler(void) {
    static unsigned long tm = 0;
    static bool btn_activity = false;
    const unsigned long repeatInterval = 200;
    if (digitalRead(DW_BTN) == HIGH) nxtHold = false;
    if (digitalRead(UP_BTN) == HIGH) prvHold = false;
    if (nxtPress || prvPress || slPress || ecPress) { btn_activity = true; }
    if (nxtHold && millis() - tm > repeatInterval) {
        nxtPress = true;
        btn_activity = true;
    }
    if (prvHold && millis() - tm > repeatInterval) {
        prvPress = true;
        btn_activity = true;
    }
    if (btn_activity && millis() - tm > repeatInterval) {
        tm = millis();
        btn_activity = false;

        wakeUpScreen();
        AnyKeyPress = true;

        NextPress = nxtPress;
        PrevPress = prvPress;
        SelPress = slPress;
        EscPress = ecPress;

        nxtPress = false;
        prvPress = false;
        slPress = false;
        ecPress = false;
    }
}

void powerOff() {
    digitalWrite(TFT_BL, LOW);
    digitalWrite(LED, LOW);
    esp_deep_sleep_start();
}

void checkReboot() {}
