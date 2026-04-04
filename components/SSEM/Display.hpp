/*-----------------------------------------------------------------------------
 * File        : Display.hpp
 * Description : SSEM emulator display controller for the M5Stack Tab5.
 *               Manages the splash screen, main interface header, footer,
 *               storeline LED grid, and storeline text labels.
 * Author      : Mark Stevens
 * Copyright   : Copyright (c) 2026 Mark Stevens
 * Licence     : MIT — see LICENSE in the repository root for full terms.
 * Target      : M5Stack Tab5 (ESP32-P4)
 * Build system: ESP-IDF v5.5.1
 *---------------------------------------------------------------------------*/

#pragma once

#include <M5GFX.h>
#include "SDCard.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

/**
 * @brief SSEM emulator display controller.
 *
 * Static class that manages the entire display pipeline for the SSEM
 * Manchester Baby emulator.  Owns the splash screen lifecycle, the main
 * interface layout (header, footer, storeline LED grid, storeline labels,
 * and blank control panel), and the associated touch callbacks.
 *
 * All state is held in static members so that registered touch callbacks
 * (which are plain function pointers) can reach the display context without
 * requiring a global variable outside this class.
 *
 * @note Run() must be called after TouchInput::Initialise() so that
 *       AddCallback() / RemoveCallback() are available for splash dismissal.
 */
class Display
{
public:
    /**
     * @brief Number of storelines in the SSEM store.
     */
    static constexpr int STORELINE_COUNT = 32;

    /**
     * @brief Message sent to the Display task via the message queue.
     *
     * Holds a complete snapshot of the display state.  The Display task
     * dequeues one of these each cycle and redraws the storeline area.
     */
    struct DisplayMessage
    {
        /**
         * @brief Current value of each storeline word.
         */
        uint32_t storelineValues[STORELINE_COUNT];

        /**
         * @brief Text label shown beside each storeline.
         */
        char storelineText[STORELINE_COUNT][32];

        /**
         * @brief Reserved — always nullptr for now.
         */
        void *controlState;
    };

    static void Run(M5GFX &display, SDCard *sdCard);
    static bool PostMessage(const DisplayMessage &message);

private:
    static void ShowSplash(SDCard *sdCard);
    static void ShowMain();
    static void DrawHeader();
    static void DrawFooter();
    static void DrawAllStorelines();
    static void DrawStoreline(int lineIndex);
    static void DrawLed(int centreX, int centreY, int radius, bool on);
    static void OnSplashTouch(const lgfx::touch_point_t *points, int count);

    /**
     * @brief Pointer to the global M5GFX display instance; assigned by Run().
     */
    static M5GFX *_display;

    /**
     * @brief Flagged true by OnSplashTouch to dismiss the splash early.
     */
    static volatile bool _splashDismissed;

    /**
     * @brief Flags indicating which storelines have changed.
     */
    static bool _changed[Display::STORELINE_COUNT];

    /**
     * @brief SSEM store — 32 unsigned 32-bit words, all zero initially (JP 0).
     */
    static uint32_t _store[32];

    /**
     * @brief Text label shown to the right of each storeline, initially "JP 0".
     */
    static char _labels[32][32];

    /**
     * @brief Number of LEDs per storeline (one per bit).
     */
    static constexpr int LED_COUNT = 32;

    /**
     * @brief Height of the header banner in pixels.
     */
    static constexpr int HEADER_HEIGHT = 32;

    /**
     * @brief Height of the footer bar in pixels.
     */
    static constexpr int FOOTER_HEIGHT = 24;

    /**
     * @brief Width and height of each LED cell in pixels (square cells).
     */
    static constexpr int LED_CELL_WIDTH = 20;

    /**
     * @brief Number of LEDs per visual group, separated by a small gap.
     */
    static constexpr int LED_GROUP_SIZE = 4;

    /**
     * @brief Gap in pixels inserted between adjacent LED groups.
     */
    static constexpr int LED_GROUP_GAP = 8;

    /**
     * @brief Number of LED groups per storeline.
     */
    static constexpr int LED_GROUP_COUNT = LED_COUNT / LED_GROUP_SIZE;

    /**
     * @brief Total pixel width of the LED grid section including inter-group gaps.
     */
    static constexpr int LED_SECTION_WIDTH = (LED_CELL_WIDTH * LED_COUNT) + ((LED_GROUP_COUNT - 1) * LED_GROUP_GAP);

    /**
     * @brief Width in pixels reserved to the left of the LED box for the per-row
     *        storeline index number.  The number is drawn outside the LED box.
     */
    static constexpr int STORELINE_NUMBER_WIDTH = 28;

    /**
     * @brief X co-ordinate of the left edge of the LED section (offset from the
     *        left of the screen by the storeline number column).
     */
    static constexpr int LED_SECTION_X = STORELINE_NUMBER_WIDTH;

    /**
     * @brief X co-ordinate of the left edge of the storeline text section.
     */
    static constexpr int TEXT_SECTION_X = LED_SECTION_X + LED_SECTION_WIDTH;

    /**
     * @brief Pixel width of the storeline text section.
     */
    static constexpr int TEXT_SECTION_WIDTH = 200;

    /**
     * @brief X co-ordinate of the left edge of the control panel section.
     */
    static constexpr int CONTROL_SECTION_X = TEXT_SECTION_X + TEXT_SECTION_WIDTH;

    /**
     * @brief Outer radius of each LED circle in pixels.
     */
    static constexpr int LED_OUTER_RADIUS = 9;

    /**
     * @brief Inner fill radius of each LED circle in pixels (leaves a white border).
     */
    static constexpr int LED_INNER_RADIUS = 7;

    /**
     * @brief Maximum time the splash is displayed before auto-dismissal (milliseconds).
     */
    static constexpr int SPLASH_TIMEOUT_MS = 5000;

    /**
     * @brief Padding in pixels between a white border box and its enclosed content.
     */
    static constexpr int BOX_PADDING = 4;

    /**
     * @brief Left margin of the text section in pixels, providing separation between
     *        the LED column and the storeline text.
     */
    static constexpr int TEXT_LEFT_MARGIN = 16;

    /**
     * @brief Width in pixels reserved for the zero-padded 8-digit hex value column
     *        that appears inside the text box, to the left of the mnemonic label.
     */
    static constexpr int HEX_COLUMN_WIDTH = 72;

    /**
     * @brief FreeRTOS queue handle for receiving DisplayMessage updates.
     */
    static QueueHandle_t _queue;

    static void DisplayTask(void *parameter);
};
