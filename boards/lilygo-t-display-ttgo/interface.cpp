#include "core/powerSave.h"
#include "core/utils.h"
#include <Button.h>
#include <esp_adc_cal.h>
#include <globals.h>
#include <interface.h>

#define ADC_EN 14
#define AUTO_OFF_TIME (15 * 60 * 1000) // 15 phút tính bằng mili-giây
volatile bool nxtPress = false;
volatile bool prvPress = false;
volatile bool ecPress = false;
volatile bool slPress = false;
volatile bool nxtHold = false;
volatile bool prvHold = false;

// Callback các nút
static void onButtonSingleClickCb1(void *button_handle, void *usr_data);
static void onButtonHoldCb1(void *button_handle, void *usr_data);
static void onButtonSingleClickCb2(void *button_handle, void *usr_data);
static void onButtonHoldCb2(void *button_handle, void *usr_data);
static void onButtonSingleClickCb3(void *button_handle, void *usr_data);
static void onButtonHoldCb3(void *button_handle, void *usr_data);

Button *btn1;
Button *btn2;
Button *btn3;

void _setup_gpio() {
    // setup buttons
    pinMode(DW_BTN, INPUT_PULLUP);
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(OK_BTN, INPUT_PULLUP);

    button_config_t bt1 = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 300, // giữ >0.3s cho Next
        .short_press_time = 40, // click nhanh
        .gpio_button_config = {
                               .gpio_num = DW_BTN,
                               .active_level = 0,
                               },
    };

    button_config_t bt2 = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 300, // giữ >0.3s cho Prev
        .short_press_time = 40,
        .gpio_button_config = {
                               .gpio_num = UP_BTN,
                               .active_level = 0,
                               },
    };

    button_config_t bt3 = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 700, // giữ >1.5s cho ESC
        .short_press_time = 40,
        .gpio_button_config = {
                               .gpio_num = OK_BTN,
                               .active_level = 0,
                               },
    };

    // Nút 1: Next
    btn1 = new Button(bt1);
    btn1->attachSingleClickEventCb(&onButtonSingleClickCb1, NULL);
    btn1->attachLongPressStartEventCb(&onButtonHoldCb1, NULL);

    // Nút 2: Prev
    btn2 = new Button(bt2);
    btn2->attachSingleClickEventCb(&onButtonSingleClickCb2, NULL);
    btn2->attachLongPressStartEventCb(&onButtonHoldCb2, NULL);

    // Nút 3: OK + ESC
    btn3 = new Button(bt3);
    btn3->attachSingleClickEventCb(&onButtonSingleClickCb3, NULL);
    btn3->attachLongPressStartEventCb(&onButtonHoldCb3, NULL);

    // Bật nguồn đo pin
    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);
    pinMode(ADC_PIN, INPUT);

    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;
    bruceConfigPins.irRx = RXLED;
    bruceConfigPins.irTx = TXLED;

    Serial.begin(115200);
}

// ==== Callback ====
// Nút 1
void onButtonSingleClickCb1(void *button_handle, void *usr_data) { nxtPress = true; }
void onButtonHoldCb1(void *button_handle, void *usr_data) { nxtHold = true; }

// Nút 2
void onButtonSingleClickCb2(void *button_handle, void *usr_data) { prvPress = true; }
void onButtonHoldCb2(void *button_handle, void *usr_data) { prvHold = true; }

// Nút 3
static void onButtonSingleClickCb3(void *button_handle, void *usr_data) {
    slPress = true; // OK
    Serial.println("OK button TRIGGERED OK");
}

static void onButtonHoldCb3(void *button_handle, void *usr_data) {
    ecPress = true; // ESC
    Serial.println("OK button LONG PRESS TRIGGERED ESC");
}

// ==== Battery ====
int getBattery() {
    int percent = 0;
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);

    uint32_t raw = analogRead(ADC_PIN);
    uint32_t v1 = esp_adc_cal_raw_to_voltage(raw, &adc_chars) * 2;

    // FIX: Logic cũ: > 4150 trả về 0 -> Sai
    // Logic mới: > 4150 trả về 100 -> Đúng
    if (v1 >= 4150) {
        percent = 0;
    } else if (v1 <= 3500) {
        percent = 0;
    } else {
        percent = map(v1, 3500, 4150, 0, 100);
    }

    return (percent < 0) ? 0 : (percent >= 100) ? 100 : percent;
}

bool isCharging() { return digitalRead(ADC_EN) == LOW; }

// ==== Backlight ====
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

// ==== Input Handler ====
// ==== Input Handler ====
void InputHandler(void) {
    static unsigned long tm = 0;
    static unsigned long dualPressStartTime = 0;
    static bool btn_pressed = false;

    // --- THÊM DÒNG NÀY: Biến lưu thời gian hành động cuối cùng ---
    static unsigned long lastActionTime = 0;

    const unsigned long repeatInterval = 200;

    // Đọc trạng thái vật lý
    bool dwIsDown = (digitalRead(DW_BTN) == LOW);
    bool okIsDown = (digitalRead(OK_BTN) == LOW);
    bool upIsDown = (digitalRead(UP_BTN) == LOW); // Đọc thêm nút UP để chắc chắn

    // 1. Cập nhật thời gian nếu có bất kỳ nút nào đang được nhấn
    if (dwIsDown || okIsDown || upIsDown) { lastActionTime = millis(); }

    // 2. Kiểm tra nếu quá 15 phút không bấm gì -> Tắt máy
    if (millis() - lastActionTime > AUTO_OFF_TIME) {
        Serial.println("Auto power off triggered due to inactivity");
        powerOff();
    }

    // ---- LOGIC NHẤN GIỮ 2 NÚT (Giữ nguyên) ----
    if (dwIsDown && okIsDown) {
        if (dualPressStartTime == 0) {
            dualPressStartTime = millis();
        } else if (millis() - dualPressStartTime > 2000) {
            powerOff();
        }
    } else {
        dualPressStartTime = 0;
    }
    // --------------------------------------------

    // Tắt hold khi thả nút
    if (!dwIsDown) nxtHold = false;
    if (!upIsDown) prvHold = false; // Sửa lại logic đọc UP_BTN cho chuẩn

    // Cập nhật trạng thái nhấn logic
    if (nxtPress || prvPress || ecPress || slPress) {
        btn_pressed = true;
        lastActionTime = millis(); // Cập nhật thời gian khi có sự kiện nhấn logic
    }

    // Next liên tục
    if (nxtHold && millis() - tm > repeatInterval) {
        nxtPress = true;
        btn_pressed = true;
    }

    // Prev liên tục
    if (prvHold && millis() - tm > repeatInterval) {
        prvPress = true;
        btn_pressed = true;
    }

    // Xử lý sự kiện nhấn (Debounce / Repeat)
    if (millis() - tm > repeatInterval) {
        if (btn_pressed) {
            btn_pressed = false;
            tm = millis();

            if (!wakeUpScreen()) AnyKeyPress = true;
            else {
                // Nếu màn hình vừa bật lại thì reset timer để không bị tắt ngay
                lastActionTime = millis();
                return;
            }

            SelPress = slPress;
            EscPress = ecPress;
            NextPress = nxtPress;
            PrevPress = prvPress;

            nxtPress = false;
            prvPress = false;
            ecPress = false;
            slPress = false;
        }
    }
}

// ==== Power Off ====
void powerOff() {
    tft.fillScreen(bruceConfig.bgColor);
    digitalWrite(TFT_BL, LOW);
    tft.writecommand(0x10); // sleep

    // FIX: Chờ người dùng thả nút DW_BTN trước khi kích hoạt ngủ
    // Nếu không có đoạn này, máy sẽ ngủ và tỉnh dậy ngay lập tức do nút vẫn đang nhấn
    while (digitalRead(DW_BTN) == LOW) {
        delay(50); // Delay nhỏ để tránh spam CPU
    }
    delay(100); // Debounce: chờ tín hiệu ổn định hẳn

    // Cấu hình đánh thức sau khi nút đã được thả
    esp_sleep_enable_ext0_wakeup((gpio_num_t)DW_BTN, BTN_ACT);
    esp_deep_sleep_start();
}
