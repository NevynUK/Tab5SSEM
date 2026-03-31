/*-----------------------------------------------------------------------------
 * File        : Tab5Template.cpp
 * Description : Application entry point for the M5Stack Tab5 SSEM emulator
 *               firmware.  Initialises all hardware peripherals and delegates
 *               the display pipeline to the SSEM Display component.
 * Author      : Mark Stevens
 * Copyright   : Copyright (c) 2026 Mark Stevens
 * Licence     : MIT — see LICENSE in the repository root for full terms.
 * Target      : M5Stack Tab5 (ESP32-P4)
 * Build system: ESP-IDF v5.5.1
 *---------------------------------------------------------------------------*/

#include <M5GFX.h>
#include <cstdio>
#include <ctime>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Display.hpp"
#include "Rtc.hpp"
#include "SDCard.hpp"
#include "Touch.hpp"

/** @brief Global display instance used by M5GFX and the SSEM Display component. */
M5GFX display;

/**
 * @brief One-time application setup.
 *
 * Initialises the display, touch input, SD card, and RTC in the order
 * required by the hardware constraints documented in the component headers.
 * Sets an initial time on the RTC, then hands control to the SSEM display
 * pipeline via Display::Run().
 */
void Setup(void)
{
    // Tab5 native panel is portrait (720×1280).
    // Rotation 3 = landscape rotated 180 degrees (1280×720).
    display.init();
    display.setBrightness(128);
    display.setRotation(3);

    // display.init() drove GPIO_NUM_23 high to select the ST7123 I2C address.
    // TouchInput can safely reconfigure it as an interrupt input now that
    // init has completed.
    TouchInput::Initialise(display);

    SDCard *sdCard = SDCard::Initialise();
    Rtc *rtc = Rtc::Initialise();

    if (rtc != nullptr)
    {
        struct tm setTime = {};
        setTime.tm_year  = 2026 - 1900;
        setTime.tm_mon   = 3 - 1;       // March (0-based)
        setTime.tm_mday  = 21;
        setTime.tm_hour  = 14;
        setTime.tm_min   = 42;
        setTime.tm_sec   = 0;
        setTime.tm_wday  = 6;           // Saturday
        setTime.tm_isdst = -1;
        rtc->SetTime(setTime);
    }

    Display::Run(display, sdCard);
}

extern "C" void app_main(void)
{
    Setup();

    // Initialise the message with all storelines zeroed and labelled "JP 0".
    Display::DisplayMessage message = {};

    for (int i = 0; i < Display::STORELINE_COUNT; ++i)
    {
        message.storelineValues[i] = 0U;
        snprintf(message.storelineText[i], sizeof(message.storelineText[i]), "JP 0");
    }

    message.controlState = nullptr;
    Display::PostMessage(message);

    // Count from 0 upward, setting every storeline to the current count value,
    // and post a display update every second.
    uint32_t count = 0U;

    while (true)
    {
        for (int i = 0; i < Display::STORELINE_COUNT; ++i)
        {
            message.storelineValues[i] = count;
        }

        Display::PostMessage(message);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ++count;
    }
}
