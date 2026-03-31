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
    /** Number of storelines in the SSEM store. */
    static constexpr int STORELINE_COUNT = 32;

    /**
     * @brief Message sent to the Display task via the message queue.
     *
     * Holds a complete snapshot of the display state.  The Display task
     * dequeues one of these each cycle and redraws the storeline area.
     */
    struct DisplayMessage
    {
        /** Current value of each storeline word. */
        uint32_t storelineValues[STORELINE_COUNT];

        /** Text label shown beside each storeline. */
        char storelineText[STORELINE_COUNT][32];

        /** Reserved — always nullptr for now. */
        void *controlState;
    };

    /**
     * @brief Initialises storage, shows the splash screen, draws the main
     *        interface, then starts the Display FreeRTOS task.
     *
     * Stores the display pointer, zeroes the SSEM store, sets all labels
     * to "JP 0", delegates to ShowSplash() then ShowMain(), creates the
     * message queue and launches the Display task.
     *
     * @param display  Reference to the global M5GFX instance (already
     *                 initialised with the correct rotation).
     * @param sdCard   Pointer to the SDCard singleton, or nullptr if no
     *                 card is present.
     */
    static void Run(M5GFX &display, SDCard *sdCard);

    /**
     * @brief Posts a DisplayMessage to the display queue.
     *
     * Non-blocking: if the queue is full the message is discarded and false
     * is returned.
     *
     * @param message  The display state snapshot to enqueue.
     * @return true if the message was accepted; false if the queue was full.
     */
    static bool PostMessage(const DisplayMessage &message);

private:
    /**
     * @brief Renders the splash screen and blocks until dismissed.
     *
     * Displays the SSEM Emulator title, "Manchester Baby" subtitle, and SD
     * card information.  Registers a temporary touch callback and blocks for
     * up to SPLASH_TIMEOUT_MS milliseconds, or until a touch is detected.
     *
     * @param sdCard  Pointer to the SDCard singleton, or nullptr.
     */
    static void ShowSplash(SDCard *sdCard);

    /**
     * @brief Renders the full main SSEM interface.
     *
     * Fills the screen black, then draws the header, footer, and all 32
     * storeline rows.  Flushes the framebuffer to the MIPI-DSI panel on
     * completion.
     */
    static void ShowMain();

    /**
     * @brief Draws the header banner across the top of the screen.
     *
     * White background, black text, centred, using Font4 (~14 px).
     */
    static void DrawHeader();

    /**
     * @brief Draws the footer bar across the bottom of the screen.
     *
     * White background, black text, left-aligned, using Font2 (~8 px).
     */
    static void DrawFooter();

    /**
     * @brief Draws all 32 storeline rows into the centre panel.
     *
     * Wraps individual DrawStoreline() calls inside a single
     * startWrite / endWrite pair for efficiency.
     */
    static void DrawAllStorelines();

    /**
     * @brief Draws one storeline row: index number, 32 LEDs, and the text label.
     *
     * The storeline index is drawn to the left of the LED box, outside it.
     * Must be called within an active startWrite / endWrite pair on _display.
     *
     * @param lineIndex  Zero-based storeline index (0–31).
     */
    static void DrawStoreline(int lineIndex);

    /**
     * @brief Draws a single LED as a white-bordered circle with a colour fill.
     *
     * Renders a filled white circle of the given radius, then overlays a
     * smaller filled circle in green (on) or black (off) to simulate an
     * illuminated LED.  Must be called within an active startWrite /
     * endWrite pair on _display.
     *
     * @param centreX  X co-ordinate of the LED centre.
     * @param centreY  Y co-ordinate of the LED centre.
     * @param radius   Outer radius of the white border circle in pixels.
     * @param on       true for green inner fill (LED lit); false for black.
     */
    static void DrawLed(int centreX, int centreY, int radius, bool on);

    /**
     * @brief Touch callback active during the splash screen.
     *
     * Sets _splashDismissed to true when at least one touch point is active,
     * allowing the splash polling loop to exit early.
     *
     * @param points  Array of screen-space touch co-ordinates.
     * @param count   Number of active touch points; zero when all lifted.
     */
    static void OnSplashTouch(const lgfx::touch_point_t *points, int count);

    /** Pointer to the global M5GFX display instance; assigned by Run(). */
    static M5GFX *_display;

    /** Flagged true by OnSplashTouch to dismiss the splash early. */
    static volatile bool _splashDismissed;

    /** SSEM store — 32 unsigned 32-bit words, all zero initially (JP 0). */
    static uint32_t _store[32];

    /** Text label shown to the right of each storeline, initially "JP 0". */
    static char _labels[32][32];

    /** Number of LEDs per storeline (one per bit). */
    static constexpr int LED_COUNT = 32;

    /** Height of the header banner in pixels. */
    static constexpr int HEADER_HEIGHT = 32;

    /** Height of the footer bar in pixels. */
    static constexpr int FOOTER_HEIGHT = 24;

    /** Width and height of each LED cell in pixels (square cells). */
    static constexpr int LED_CELL_WIDTH = 20;

    /** Total pixel width of the LED grid section (LED_CELL_WIDTH × LED_COUNT). */
    static constexpr int LED_SECTION_WIDTH = LED_CELL_WIDTH * LED_COUNT;

    /** Width in pixels reserved to the left of the LED box for the per-row
     *  storeline index number.  The number is drawn outside the LED box. */
    static constexpr int STORELINE_NUMBER_WIDTH = 28;

    /** X co-ordinate of the left edge of the LED section (offset from the
     *  left of the screen by the storeline number column). */
    static constexpr int LED_SECTION_X = STORELINE_NUMBER_WIDTH;

    /** X co-ordinate of the left edge of the storeline text section. */
    static constexpr int TEXT_SECTION_X = LED_SECTION_X + LED_SECTION_WIDTH;

    /** Pixel width of the storeline text section. */
    static constexpr int TEXT_SECTION_WIDTH = 200;

    /** X co-ordinate of the left edge of the control panel section. */
    static constexpr int CONTROL_SECTION_X = TEXT_SECTION_X + TEXT_SECTION_WIDTH;

    /** Outer radius of each LED circle in pixels. */
    static constexpr int LED_OUTER_RADIUS = 9;

    /** Inner fill radius of each LED circle in pixels (leaves a white border). */
    static constexpr int LED_INNER_RADIUS = 7;

    /** Maximum time the splash is displayed before auto-dismissal (milliseconds). */
    static constexpr int SPLASH_TIMEOUT_MS = 5000;

    /** Padding in pixels between a white border box and its enclosed content. */
    static constexpr int BOX_PADDING = 4;

    /** Left margin of the text section in pixels, providing separation between
     *  the LED column and the storeline text. */
    static constexpr int TEXT_LEFT_MARGIN = 16;

    /** FreeRTOS queue handle for receiving DisplayMessage updates. */
    static QueueHandle_t _queue;

    /**
     * @brief FreeRTOS task body for the Display task.
     *
     * Blocks indefinitely on _queue.  On each message received, copies the
     * storeline values and text labels into the static members then redraws
     * all storeline rows and flushes the framebuffer.
     *
     * @param parameter  Unused; required by the FreeRTOS task signature.
     */
    static void DisplayTask(void *parameter);
};
