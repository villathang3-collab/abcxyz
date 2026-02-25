#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <interface.h>
#include "FT6336U.h" 

// Create an instance of the touch screen controller
FT6336U touch = FT6336U(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   Initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    if (!touch.begin()) {
        Serial.println("Touch IC not Started");
        log_i("Touch IC not Started");
    } else {
        log_i("Touch IC Started");
    }
    bruceConfig.colorInverted = 0;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Description:   Second stage GPIO setup
***************************************************************************************/
void _post_setup_gpio() {
    // Brightness control
    pinMode(TFT_BL, OUTPUT);
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 255); // Start with full brightness
}

/*********************************************************************
** Function: _setBrightness
** Description: set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 255;
    else if (brightval == 75) dutyCycle = 130;
    else if (brightval == 50) dutyCycle = 70;
    else if (brightval == 25) dutyCycle = 20;
    else if (brightval == 0) dutyCycle = 0;
    else dutyCycle = ((brightval * 255) / 100);

    ledcWrite(TFT_BL, dutyCycle);
}

/*********************************************************************
** Function: getBattery
** Description: Reads the battery level
**********************************************************************/
int getBattery() {
    // Battery ADC pin is not defined in the provided pinout.
    // Returning 100% as a placeholder.
    return 100;
}

/*********************************************************************
** Function: InputHandler
** Handles touch input
**********************************************************************/
void InputHandler(void) {
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        FT6336U_TouchPointType t = touch.scan();

        if (t.touch_count > 0) { // Check if the screen is being touched
            // --- Coordinate Rotation Logic ---
            if (bruceConfigPins.rotation == 3) {
                t.y = (tftHeight + 20) - t.y;
                t.x = tftWidth - t.x;
            }
            if (bruceConfigPins.rotation == 0) {
                int tmp = t.x;
                t.x = tftWidth - t.y;
                t.y = tmp;
            }
            if (bruceConfigPins.rotation == 2) {
                int tmp = t.x;
                t.x = t.y;
                t.y = (tftHeight + 20) - tmp;
            }

            if (!wakeUpScreen()) AnyKeyPress = true;
            else goto END;

            // Update global touch point variable
            touchPoint.x = t.x;
            touchPoint.y = t.y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);

        END:
            d_tmp = millis();
        }
    }
}

/*********************************************************************
** Function: powerOff
** Turns off the device
**********************************************************************/
void powerOff() {
   esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** (Unused in the base implementation)
**********************************************************************/
void checkReboot() {}
