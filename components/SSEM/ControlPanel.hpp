/*-----------------------------------------------------------------------------
 * File        : ControlPanel.hpp
 * Description : SSEM emulator control panel for the M5Stack Tab5.  Manages
 *               the Halted/Running status indicator, speed radio buttons,
 *               scrollable file list, and Load / Stop–Run action buttons.
 * Author      : Mark Stevens
 * Copyright   : Copyright (c) 2026 Mark Stevens
 * Licence     : MIT — see LICENSE in the repository root for full terms.
 * Target      : M5Stack Tab5 (ESP32-P4)
 * Build system: ESP-IDF v5.5.1
 *---------------------------------------------------------------------------*/

#pragma once

#include <M5GFX.h>
#include <functional>
#include <string>
#include <vector>

using namespace std;

/**
 * @brief SSEM emulator control panel.
 *
 * Static class that owns all interactive controls in the right-hand panel
 * of the SSEM interface.  Controls appear top-to-bottom in the following
 * order: Halted/Running status indicator, speed radio buttons, scrollable
 * file list, Load button, and Stop/Run button.
 *
 * All state is held in static members so that the touch callback (a plain
 * function pointer required by TouchInput) can reach the panel context
 * without requiring a global variable outside this class.
 *
 * @note Initialise() must be called after TouchInput::Initialise().
 */
class ControlPanel
{
public:
    /**
     * @brief Selectable SSEM execution speed mode.
     */
    enum class SpeedSetting {
        /**
         * @brief Run as fast as the hardware allows.
         */
        Maximum,
        /**
         * @brief Simulate original 1948 Manchester Baby clock rate.
         */
        Original
    };

    /**
     * @brief Callback type invoked when the Stop/Run button is pressed.
     *
     * The bool argument is the new intended running state:
     * true = the user wants to start execution, false = stop.
     */
    using StopRunCallback = function<void(bool running)>;

    /**
     * @brief Callback type invoked when the Load button is pressed.
     *
     * The string argument is the full path of the currently selected file.
     */
    using LoadCallback = function<void(const string &filename)>;

    static void Initialise(M5GFX &display);
    static void Draw();

    static void SetRunning(bool running);
    static void SetFiles(const vector<string> &files);
    static void SetLoadEnabled(bool enabled);
    static void SetStopRunEnabled(bool enabled);

    static SpeedSetting GetSpeed();

    static void SetStopRunCallback(StopRunCallback callback);
    static void SetLoadCallback(LoadCallback callback);

private:
    static void DrawRunningIndicator();
    static void DrawSpeedSection();
    static void DrawFileList();
    static void DrawActionButtons();
    static void DrawButton(int x, int y, int width, int height, const char *label, bool enabled);

    static void OnPanelTouch(const lgfx::touch_point_t *points, int count);
    static void HandlePress(int touchX, int touchY);
    static bool HitTest(int touchX, int touchY, int x, int y, int width, int height);
    static string Basename(const string &path);

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    /**
     * @brief Pointer to the global M5GFX display instance; assigned by Initialise().
     */
    static M5GFX *_display;

    /**
     * @brief true while the SSEM CPU is executing a program.
     */
    static bool _running;

    /**
     * @brief Currently selected execution speed.
     */
    static SpeedSetting _speedSetting;

    /**
     * @brief Ordered list of loadable SSEM program file paths.
     */
    static vector<string> _files;

    /**
     * @brief Zero-based index of the selected file; −1 when none is selected.
     */
    static int _selectedFile;

    /**
     * @brief Index of the first file visible at the top of the list.
     */
    static int _scrollOffset;

    /**
     * @brief true when the Load button is enabled.
     */
    static bool _loadEnabled;

    /**
     * @brief true when the Stop/Run button is enabled.
     */
    static bool _stopRunEnabled;

    /**
     * @brief Previous touch active state used to detect press edges.
     */
    static bool _prevTouched;

    /**
     * @brief Registered callback for the Stop/Run button.
     */
    static StopRunCallback _stopRunCallback;

    /**
     * @brief Registered callback for the Load button.
     */
    static LoadCallback _loadCallback;

    // -----------------------------------------------------------------------
    // Layout constants (all units: pixels)
    //
    // Horizontal values are derived from the Display layout:
    //   CONTROL_SECTION_X = STORELINE_NUMBER_WIDTH(28)
    //                      + LED_SECTION_WIDTH(696)
    //                      + TEXT_SECTION_WIDTH(200) = 924
    // -----------------------------------------------------------------------

    /**
     * @brief X co-ordinate of the left edge of the control section.
     */
    static constexpr int CONTROL_SECTION_X = 928;
    /**
     * @brief Padding applied inside all four edges of the content area.
     */
    static constexpr int PANEL_PADDING = 8;
    /**
     * @brief X co-ordinate of the left edge of the panel content area.
     */
    static constexpr int PANEL_X = CONTROL_SECTION_X + PANEL_PADDING;
    /**
     * @brief Pixel width of the display in landscape orientation.
     */
    static constexpr int DISPLAY_WIDTH = 1280;
    /**
     * @brief Width of the panel content area in pixels.
     */
    static constexpr int PANEL_WIDTH = DISPLAY_WIDTH - CONTROL_SECTION_X - (2 * PANEL_PADDING);

    /**
     * @brief Height of the display header banner in pixels (matches Display).
     */
    static constexpr int HEADER_HEIGHT = 32;
    /**
     * @brief Height of the display footer bar in pixels (matches Display).
     */
    static constexpr int FOOTER_HEIGHT = 24;
    /**
     * @brief Pixel height of the display in landscape orientation.
     */
    static constexpr int DISPLAY_HEIGHT = 720;
    /**
     * @brief Y co-ordinate of the top of the panel content area.
     */
    static constexpr int PANEL_Y = HEADER_HEIGHT + PANEL_PADDING;
    /**
     * @brief Y co-ordinate of the bottom of the panel content area.
     */
    static constexpr int PANEL_BOTTOM = DISPLAY_HEIGHT - FOOTER_HEIGHT - PANEL_PADDING;

    /**
     * @brief Height of the Halted/Running status indicator in pixels.
     */
    static constexpr int INDICATOR_HEIGHT = 48;
    /**
     * @brief Height allocated to the speed radio-button section in pixels.
     */
    static constexpr int SPEED_HEIGHT = 56;
    /**
     * @brief Height of the "Files:" label text row in pixels.
     */
    static constexpr int FILES_LABEL_HEIGHT = 22;
    /**
     * @brief Gap in pixels between the files label and the list box.
     */
    static constexpr int FILES_LABEL_GAP = 4;
    /**
     * @brief Height of each action button (Load / Stop–Run) in pixels.
     */
    static constexpr int BUTTON_HEIGHT = 44;
    /**
     * @brief Vertical gap between the two action buttons in pixels.
     */
    static constexpr int BUTTON_GAP = 4;
    /**
     * @brief Vertical gap between major sections in pixels.
     */
    static constexpr int SECTION_GAP = 8;
    /**
     * @brief Height of each file list item row in pixels.
     */
    static constexpr int LIST_ITEM_HEIGHT = 26;
    /**
     * @brief Height of the scroll-arrow touch areas at the top/bottom of the list.
     */
    static constexpr int SCROLL_ARROW_HEIGHT = 26;
    /**
     * @brief Corner radius for buttons and the running indicator in pixels.
     */
    static constexpr int CORNER_RADIUS = 8;

    // Derived Y positions of each region

    /**
     * @brief Y co-ordinate of the top of the Halted/Running indicator.
     */
    static constexpr int INDICATOR_Y = PANEL_Y;
    /**
     * @brief Y co-ordinate of the top of the speed radio-button section.
     */
    static constexpr int SPEED_Y = INDICATOR_Y + INDICATOR_HEIGHT + SECTION_GAP;
    /**
     * @brief Y co-ordinate of the "Files:" label.
     */
    static constexpr int FILES_LABEL_Y = SPEED_Y + SPEED_HEIGHT + SECTION_GAP;
    /**
     * @brief Y co-ordinate of the top of the file list box.
     */
    static constexpr int FILES_LIST_Y = FILES_LABEL_Y + FILES_LABEL_HEIGHT + FILES_LABEL_GAP;
    /**
     * @brief Y co-ordinate of the top of the Stop/Run button.
     */
    static constexpr int STOPRUN_BUTTON_Y = PANEL_BOTTOM - BUTTON_HEIGHT;
    /**
     * @brief Y co-ordinate of the top of the Load button.
     */
    static constexpr int LOAD_BUTTON_Y = STOPRUN_BUTTON_Y - BUTTON_GAP - BUTTON_HEIGHT;
    /**
     * @brief Y co-ordinate of the bottom of the file list box.
     */
    static constexpr int FILES_LIST_BOTTOM = LOAD_BUTTON_Y - SECTION_GAP;
    /**
     * @brief Pixel height of the file list box.
     */
    static constexpr int FILES_LIST_HEIGHT = FILES_LIST_BOTTOM - FILES_LIST_Y;
    /**
     * @brief Maximum number of file list items visible at once.
     */
    static constexpr int VISIBLE_ITEMS = (FILES_LIST_HEIGHT - (2 * SCROLL_ARROW_HEIGHT)) / LIST_ITEM_HEIGHT;

    // Radio button geometry

    /**
     * @brief Outer radius of each radio button circle in pixels.
     */
    static constexpr int RADIO_OUTER_RADIUS = 8;
    /**
     * @brief Inner fill radius of the selected radio button in pixels.
     */
    static constexpr int RADIO_INNER_RADIUS = 5;
    /**
     * @brief Horizontal indent of radio circles from the panel left edge in pixels.
     */
    static constexpr int RADIO_INDENT = 12;
    /**
     * @brief Horizontal distance from radio circle centre to start of text label.
     */
    static constexpr int RADIO_LABEL_OFFSET = 14;
};
