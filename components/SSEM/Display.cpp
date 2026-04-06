/*-----------------------------------------------------------------------------
 * File        : Display.cpp
 * Description : SSEM emulator display controller for the M5Stack Tab5.
 *               Manages the splash screen, main interface header, footer,
 *               storeline LED grid, and storeline text labels.
 * Author      : Mark Stevens
 * Copyright   : Copyright (c) 2026 Mark Stevens
 * Licence     : MIT — see LICENSE in the repository root for full terms.
 * Target      : M5Stack Tab5 (ESP32-P4)
 * Build system: ESP-IDF v5.5.1
 *---------------------------------------------------------------------------*/

#include "Display.hpp"
#include "Touch.hpp"
#include <cstdio>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

/**
 * @brief Pointer to the global M5GFX display instance; assigned by Run().
 */
M5GFX *Display::_display = nullptr;

/**
 * @brief Set to true by OnSplashTouch to signal touch-based dismissal.
 */
volatile bool Display::_splashDismissed = false;

/**
 * @brief Flags indicating which storelines have changed.
 */
bool Display::_changed[Display::STORELINE_COUNT] = {};

/**
 * @brief SSEM store words; all initialised to zero (JP 0) in Run().
 */
uint32_t Display::_store[Display::STORELINE_COUNT] = {};

/**
 * @brief Storeline text labels; all initialised to "JP 0" in Run().
 */
char Display::_labels[Display::STORELINE_COUNT][32] = {};

/**
 * @brief FreeRTOS queue handle; created by Run() before the Display task is started.
 */
QueueHandle_t Display::_queue = nullptr;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------

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
void Display::Run(M5GFX &display, SDCard *sdCard)
{
    _display = &display;

    for (int i = 0; i < STORELINE_COUNT; ++i)
    {
        _store[i] = 0U;
        _changed[i] = true;
        snprintf(_labels[i], sizeof(_labels[i]), "JP 0");
    }

    ShowSplash(sdCard);
    ShowMain();

    _queue = xQueueCreate(4, sizeof(DisplayMessage));
    xTaskCreate(DisplayTask, "DisplayTask", 8192, nullptr, 5, nullptr);
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------

/**
 * @brief Renders the splash screen and blocks until dismissed.
 *
 * Displays the SSEM Emulator title, "Manchester Baby" subtitle, and SD
 * card information.  Registers a temporary touch callback and blocks for
 * up to SPLASH_TIMEOUT_MS milliseconds, or until a touch is detected.
 *
 * @param sdCard  Pointer to the SDCard singleton, or nullptr.
 */
void Display::ShowSplash(SDCard *sdCard)
{
    const int centreX = _display->width() / 2;
    const int centreY = _display->height() / 2;

    _display->startWrite();
    _display->fillScreen(TFT_WHITE);
    _display->setTextColor(TFT_BLACK, TFT_WHITE);
    _display->setTextDatum(textdatum_t::middle_center);

    // Title and subtitle
    _display->setFont(&fonts::Font4);
    _display->drawString("SSEM Emulator", centreX, centreY - 80);
    _display->drawString("Manchester Baby", centreX, centreY - 40);

    // SD card information
    if (sdCard != nullptr && sdCard->IsMounted())
    {
        const sdmmc_card_t *card = sdCard->GetCard();
        const uint64_t sizeBytes = static_cast<uint64_t>(card->csd.capacity) * static_cast<uint64_t>(card->csd.sector_size);
        const double sizeGigabytes = static_cast<double>(sizeBytes) / (1024.0 * 1024.0 * 1024.0);

        char sdInfo[64];
        snprintf(sdInfo, sizeof(sdInfo), "SD Card: %.1f GB  (%s)", sizeGigabytes, card->cid.name);
        _display->drawString(sdInfo, centreX, centreY + 10);
    }
    else
    {
        _display->setTextColor(TFT_RED, TFT_WHITE);
        _display->drawString("SD Card: not present", centreX, centreY + 10);
        _display->setTextColor(TFT_BLACK, TFT_WHITE);
    }

    // Dismissal hint
    _display->setFont(&fonts::Font2);
    _display->drawString("Touch the screen or wait 5 seconds to continue.", centreX, centreY + 70);

    _display->endWrite();
    _display->display();

    // Register touch callback and poll until dismissed or timed out
    _splashDismissed = false;
    TouchInput::GetInstance()->AddCallback(OnSplashTouch);

    const TickType_t startTick = xTaskGetTickCount();

    while (!_splashDismissed)
    {
        if ((xTaskGetTickCount() - startTick) >= pdMS_TO_TICKS(SPLASH_TIMEOUT_MS))
        {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }

    TouchInput::GetInstance()->RemoveCallback(OnSplashTouch);
}

/**
 * @brief Renders the full main SSEM interface.
 *
 * Fills the screen black, then draws the header, footer, and all 32
 * storeline rows.  Flushes the framebuffer to the MIPI-DSI panel on
 * completion.
 */
void Display::ShowMain()
{
    _display->startWrite();
    _display->fillScreen(TFT_BLACK);
    _display->endWrite();

    DrawHeader();
    DrawFooter();
    DrawAllStorelines();

    _display->display();
}

/**
 * @brief Draws the header banner across the top of the screen.
 *
 * White background, black text, centred, using Font4 (~14 px).
 */
void Display::DrawHeader()
{
    _display->startWrite();
    _display->fillRect(0, 0, _display->width(), HEADER_HEIGHT, TFT_WHITE);
    _display->setFont(&fonts::Font4);
    _display->setTextColor(TFT_BLACK, TFT_WHITE);
    _display->setTextDatum(textdatum_t::middle_center);
    _display->drawString("SSEM - Manchester Baby", _display->width() / 2, HEADER_HEIGHT / 2);
    _display->endWrite();
}

/**
 * @brief Draws the footer bar across the bottom of the screen.
 *
 * White background, black text, left-aligned, using Font2 (~8 px).
 */
void Display::DrawFooter()
{
    const int footerY = _display->height() - FOOTER_HEIGHT;

    _display->startWrite();
    _display->fillRect(0, footerY, _display->width(), FOOTER_HEIGHT, TFT_WHITE);
    _display->setFont(&fonts::Font2);
    _display->setTextColor(TFT_BLACK, TFT_WHITE);
    _display->setTextDatum(textdatum_t::middle_left);
    _display->drawString("SSEM Emulator", 8, footerY + FOOTER_HEIGHT / 2);
    _display->endWrite();
}

/**
 * @brief Draws all 32 storeline rows into the centre panel.
 *
 * Wraps individual DrawStoreline() calls inside a single
 * startWrite / endWrite pair for efficiency.
 */
void Display::DrawAllStorelines()
{
    _display->startWrite();

    const int centreHeight = _display->height() - HEADER_HEIGHT - FOOTER_HEIGHT;
    const int boxY = HEADER_HEIGHT + BOX_PADDING;
    const int boxHeight = centreHeight - (2 * BOX_PADDING);

    // White border box enclosing all LED rows
    _display->drawRect(LED_SECTION_X, boxY, LED_SECTION_WIDTH + BOX_PADDING, boxHeight, TFT_WHITE);

    // White border box enclosing all storeline text rows (with TEXT_LEFT_MARGIN separation from LEDs)
    _display->drawRect(TEXT_SECTION_X + TEXT_LEFT_MARGIN - BOX_PADDING, boxY, TEXT_SECTION_WIDTH - TEXT_LEFT_MARGIN + (2 * BOX_PADDING), boxHeight, TFT_WHITE);

    for (int i = 0; i < STORELINE_COUNT; ++i)
    {
        if (_changed[i])
        {
            DrawStoreline(i);
        }
    }

    _display->endWrite();
}

/**
 * @brief Draws one storeline row: index number, 32 LEDs, and the text label.
 *
 * The storeline index is drawn to the left of the LED box, outside it.
 * Must be called within an active startWrite / endWrite pair on _display.
 *
 * @param lineIndex  Zero-based storeline index (0–31).
 */
void Display::DrawStoreline(int lineIndex)
{
    const int centreHeight = _display->height() - HEADER_HEIGHT - FOOTER_HEIGHT;
    const int rowHeight = centreHeight / STORELINE_COUNT;
    const int totalUsed = rowHeight * STORELINE_COUNT;
    const int storelineOffset = (centreHeight - totalUsed) / 2;
    const int rowY = HEADER_HEIGHT + storelineOffset + lineIndex * rowHeight;
    const int ledCentreY = rowY + rowHeight / 2;

    // Draw the storeline index number to the left of the LED box (outside it)
    char lineNumber[4];
    snprintf(lineNumber, sizeof(lineNumber), "%" PRId32, (int32_t) lineIndex);
    _display->setFont(&fonts::Font2);
    _display->setTextColor(TFT_WHITE, TFT_BLACK);
    _display->setTextDatum(textdatum_t::middle_right);
    _display->drawString(lineNumber, LED_SECTION_X - BOX_PADDING, ledCentreY);

    // Draw 32 LEDs — bit 0 (LSB) is displayed leftmost
    const uint32_t value = _store[lineIndex];

    for (int bit = 0; bit < LED_COUNT; ++bit)
    {
        const bool on = ((value >> bit) & 1U) != 0U;
        const int group = bit / LED_GROUP_SIZE;
        const int ledCentreX = LED_SECTION_X + bit * LED_CELL_WIDTH + LED_CELL_WIDTH / 2 + group * LED_GROUP_GAP;
        DrawLed(ledCentreX, ledCentreY, LED_OUTER_RADIUS, on);
    }

    // Draw the zero-padded 8-digit hex value of the storeline word
    char hexValue[9];
    snprintf(hexValue, sizeof(hexValue), "%08" PRIX32, value);
    _display->setFont(&fonts::Font2);
    _display->setTextColor(TFT_WHITE, TFT_BLACK);
    _display->setTextDatum(textdatum_t::middle_left);
    _display->drawString(hexValue, TEXT_SECTION_X + TEXT_LEFT_MARGIN + BOX_PADDING, ledCentreY);

    // Draw the mnemonic text label to the right of the hex column
    const int labelX = TEXT_SECTION_X + TEXT_LEFT_MARGIN + BOX_PADDING + HEX_COLUMN_WIDTH;
    const int labelWidth = TEXT_SECTION_WIDTH - TEXT_LEFT_MARGIN - HEX_COLUMN_WIDTH - 1;
    _display->fillRect(labelX, rowY + 1, labelWidth, rowHeight - 2, TFT_BLACK);

    _display->drawString(_labels[lineIndex], labelX, ledCentreY);
}

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
void Display::DrawLed(int centreX, int centreY, int radius, bool on)
{
    _display->fillCircle(centreX, centreY, radius, TFT_WHITE);
    _display->fillCircle(centreX, centreY, radius - 2, on ? TFT_GREEN : TFT_BLACK);
}

/**
 * @brief Touch callback active during the splash screen.
 *
 * Sets _splashDismissed to true when at least one touch point is active,
 * allowing the splash polling loop to exit early.
 *
 * @param points  Array of screen-space touch co-ordinates.
 * @param count   Number of active touch points; zero when all lifted.
 */
void Display::OnSplashTouch(const lgfx::touch_point_t *points, int count)
{
    (void) points;

    if (count > 0)
    {
        _splashDismissed = true;
    }
}

/**
 * @brief Posts a DisplayMessage to the display queue.
 *
 * Non-blocking: if the queue is full the message is discarded and false
 * is returned.
 *
 * @param message  The display state snapshot to enqueue.
 * @return true if the message was accepted; false if the queue was full.
 */
bool Display::PostMessage(const DisplayMessage &message)
{
    return (xQueueSend(_queue, &message, portMAX_DELAY) == pdTRUE);
}

/**
 * @brief FreeRTOS task body for the Display task.
 *
 * Blocks indefinitely on _queue.  On each message received, copies the
 * storeline values and text labels into the static members then redraws
 * all storeline rows and flushes the framebuffer.
 *
 * @param parameter  Unused; required by the FreeRTOS task signature.
 */
void Display::DisplayTask(void *parameter)
{
    (void) parameter;

    DisplayMessage message;

    while (true)
    {
        if (xQueueReceive(_queue, &message, portMAX_DELAY) == pdTRUE)
        {
            for (int i = 0; i < STORELINE_COUNT; ++i)
            {
                if (_store[i] != message.storelineValues[i])
                {
                    _changed[i] = true;
                    _store[i] = message.storelineValues[i];
                    snprintf(_labels[i], sizeof(_labels[i]), "%s", message.storelineText[i]);
                }
                else
                {
                    _changed[i] = false;
                }
            }

            DrawAllStorelines();
            _display->display();
        }
    }
}
