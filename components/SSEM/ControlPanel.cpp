/*-----------------------------------------------------------------------------
 * File        : ControlPanel.cpp
 * Description : SSEM emulator control panel for the M5Stack Tab5.  Implements
 *               the Halted/Running status indicator, speed radio buttons,
 *               scrollable file list, and Load / Stop–Run action buttons.
 * Author      : Mark Stevens
 * Copyright   : Copyright (c) 2026 Mark Stevens
 * Licence     : MIT — see LICENSE in the repository root for full terms.
 * Target      : M5Stack Tab5 (ESP32-P4)
 * Build system: ESP-IDF v5.5.1
 *---------------------------------------------------------------------------*/

#include "ControlPanel.hpp"
#include "Touch.hpp"
#include <cstdio>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

/**
 * @brief Pointer to the global M5GFX display instance; assigned by Initialise().
 */
M5GFX *ControlPanel::_display = nullptr;

/**
 * @brief true while the SSEM CPU is executing a program.
 */
bool ControlPanel::_running = false;

/**
 * @brief Currently selected execution speed; defaults to Maximum.
 */
ControlPanel::SpeedSetting ControlPanel::_speedSetting = ControlPanel::SpeedSetting::Maximum;

/**
 * @brief Ordered list of loadable SSEM program file paths.
 */
std::vector<std::string> ControlPanel::_files;

/**
 * @brief Zero-based index of the selected file; −1 when none is selected.
 */
int ControlPanel::_selectedFile = -1;

/**
 * @brief Index of the first file visible at the top of the list.
 */
int ControlPanel::_scrollOffset = 0;

/**
 * @brief true when the Load button is enabled.
 */
bool ControlPanel::_loadEnabled = false;

/**
 * @brief true when the Stop/Run button is enabled.
 */
bool ControlPanel::_stopRunEnabled = false;

/**
 * @brief Previous touch active state used to detect press edges.
 */
bool ControlPanel::_prevTouched = false;

/**
 * @brief Registered callback for the Stop/Run button; empty by default.
 */
ControlPanel::StopRunCallback ControlPanel::_stopRunCallback = nullptr;

/**
 * @brief Registered callback for the Load button; empty by default.
 */
ControlPanel::LoadCallback ControlPanel::_loadCallback = nullptr;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------

/**
 * @brief Initialises the control panel and registers the touch callback.
 *
 * Stores the display pointer and registers OnPanelTouch with TouchInput.
 * Must be called after TouchInput::Initialise() so that AddCallback() is
 * available.
 *
 * @param display  Reference to the global M5GFX instance (already initialised).
 */
void ControlPanel::Initialise(M5GFX &display)
{
    _display = &display;
    TouchInput::GetInstance()->AddCallback(OnPanelTouch);
}

/**
 * @brief Redraws the entire control panel from the current state.
 *
 * Clears the panel background, then redraws all four regions in order:
 * running indicator, speed section, file list, and action buttons.
 * Flushes the framebuffer to the MIPI-DSI panel on completion.
 */
void ControlPanel::Draw()
{
    _display->startWrite();
    _display->fillRect(CONTROL_SECTION_X, HEADER_HEIGHT, DISPLAY_WIDTH - CONTROL_SECTION_X, DISPLAY_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT, TFT_BLACK);
    _display->endWrite();

    DrawRunningIndicator();
    DrawSpeedSection();
    DrawFileList();
    DrawActionButtons();

    _display->display();
}

/**
 * @brief Updates the running/halted state and redraws the affected regions.
 *
 * Updates the status indicator and the Stop/Run button label.
 *
 * @param running  true if the SSEM CPU is now executing; false if halted.
 */
void ControlPanel::SetRunning(bool running)
{
    _running = running;
    DrawRunningIndicator();
    DrawActionButtons();
    _display->display();
}

/**
 * @brief Replaces the file list and redraws the list box.
 *
 * Resets the scroll offset and file selection whenever the list is
 * refreshed.
 *
 * @param files  Ordered vector of full file paths to display.
 */
void ControlPanel::SetFiles(const std::vector<std::string> &files)
{
    _files = files;
    _selectedFile = -1;
    _scrollOffset = 0;
    DrawFileList();
    DrawActionButtons();
    _display->display();
}

/**
 * @brief Sets the enabled state of the Load button.
 *
 * Redraws the action buttons to reflect the new state.
 *
 * @param enabled  true to enable the Load button; false to disable it.
 */
void ControlPanel::SetLoadEnabled(bool enabled)
{
    _loadEnabled = enabled;
    DrawActionButtons();
    _display->display();
}

/**
 * @brief Sets the enabled state of the Stop/Run button.
 *
 * Redraws the action buttons to reflect the new state.
 *
 * @param enabled  true to enable the Stop/Run button; false to disable it.
 */
void ControlPanel::SetStopRunEnabled(bool enabled)
{
    _stopRunEnabled = enabled;
    DrawActionButtons();
    _display->display();
}

/**
 * @brief Returns the currently selected execution speed.
 *
 * @return SpeedSetting  The active speed mode (Maximum or Original).
 */
ControlPanel::SpeedSetting ControlPanel::GetSpeed()
{
    return (_speedSetting);
}

/**
 * @brief Registers the callback for the Stop/Run button.
 *
 * The callback is invoked with the new intended running state when the
 * Stop/Run button is pressed: true = start, false = stop.
 *
 * @param callback  Callable accepting a bool (the new running state).
 */
void ControlPanel::SetStopRunCallback(StopRunCallback callback)
{
    _stopRunCallback = callback;
}

/**
 * @brief Registers the callback for the Load button.
 *
 * The callback is invoked with the full path of the selected file when
 * the Load button is pressed.
 *
 * @param callback  Callable accepting a const std::string reference (file path).
 */
void ControlPanel::SetLoadCallback(LoadCallback callback)
{
    _loadCallback = callback;
}

// ---------------------------------------------------------------------------
// Private drawing methods
// ---------------------------------------------------------------------------

/**
 * @brief Draws the Halted/Running status indicator.
 *
 * Renders a white-bordered rounded rectangle with a black fill, containing
 * "Running" in green or "Halted" in red, centred within the rectangle.
 */
void ControlPanel::DrawRunningIndicator()
{
    _display->startWrite();

    // White border
    _display->fillRoundRect(PANEL_X, INDICATOR_Y, PANEL_WIDTH, INDICATOR_HEIGHT, CORNER_RADIUS, TFT_WHITE);
    // Black fill inside the border
    _display->fillRoundRect(PANEL_X + 2, INDICATOR_Y + 2, PANEL_WIDTH - 4, INDICATOR_HEIGHT - 4, CORNER_RADIUS - 1, TFT_BLACK);

    const uint16_t textColour = _running ? static_cast<uint16_t>(TFT_GREEN) : static_cast<uint16_t>(TFT_RED);
    const char *label = _running ? "Running" : "Halted";

    _display->setFont(&fonts::Font4);
    _display->setTextColor(textColour, TFT_BLACK);
    _display->setTextDatum(textdatum_t::middle_center);
    _display->drawString(label, PANEL_X + PANEL_WIDTH / 2, INDICATOR_Y + INDICATOR_HEIGHT / 2);

    _display->endWrite();
}

/**
 * @brief Draws the speed radio-button section.
 *
 * Renders a "Speed:" label above two radio buttons labelled "Maximum"
 * and "Original" placed side by side on a single row.  The selected
 * option has its inner circle filled white; the unselected option has
 * a hollow circle (black inner fill).
 */
void ControlPanel::DrawSpeedSection()
{
    _display->startWrite();

    // Clear section background
    _display->fillRect(PANEL_X, SPEED_Y, PANEL_WIDTH, SPEED_HEIGHT, TFT_BLACK);

    // Section label
    _display->setFont(&fonts::Font4);
    _display->setTextColor(TFT_WHITE, TFT_BLACK);
    _display->setTextDatum(textdatum_t::top_left);
    _display->drawString("Speed:", PANEL_X, SPEED_Y + 2);

    const int halfWidth = PANEL_WIDTH / 2;
    const int rowCentreY = SPEED_Y + 40;

    // Maximum radio button (left half)
    const int maxCircleX = PANEL_X + RADIO_INDENT + RADIO_OUTER_RADIUS;
    const int maxLabelX = maxCircleX + RADIO_OUTER_RADIUS + RADIO_LABEL_OFFSET;
    const bool maxSelected = (_speedSetting == SpeedSetting::Maximum);
    _display->drawCircle(maxCircleX, rowCentreY, RADIO_OUTER_RADIUS, TFT_WHITE);
    _display->fillCircle(maxCircleX, rowCentreY, RADIO_INNER_RADIUS, maxSelected ? static_cast<uint16_t>(TFT_WHITE) : static_cast<uint16_t>(TFT_BLACK));
    _display->setFont(&fonts::Font4);
    _display->setTextDatum(textdatum_t::middle_left);
    _display->drawString("Maximum", maxLabelX, rowCentreY);

    // Original radio button (right half)
    const int origCircleX = PANEL_X + halfWidth + RADIO_INDENT + RADIO_OUTER_RADIUS;
    const int origLabelX = origCircleX + RADIO_OUTER_RADIUS + RADIO_LABEL_OFFSET;
    const bool origSelected = (_speedSetting == SpeedSetting::Original);
    _display->drawCircle(origCircleX, rowCentreY, RADIO_OUTER_RADIUS, TFT_WHITE);
    _display->fillCircle(origCircleX, rowCentreY, RADIO_INNER_RADIUS, origSelected ? static_cast<uint16_t>(TFT_WHITE) : static_cast<uint16_t>(TFT_BLACK));
    _display->drawString("Original", origLabelX, rowCentreY);

    _display->endWrite();
}

/**
 * @brief Draws the scrollable file list box.
 *
 * Renders a "Files:" label, then draws a white-bordered box containing
 * an up-scroll arrow, up to VISIBLE_ITEMS file rows, and a down-scroll
 * arrow.  The selected item is highlighted with a white background and
 * black text.  Scroll arrows are grey when scrolling in that direction
 * is not available.
 */
void ControlPanel::DrawFileList()
{
    _display->startWrite();

    // "Files:" label above the box
    _display->fillRect(PANEL_X, FILES_LABEL_Y, PANEL_WIDTH, FILES_LABEL_HEIGHT, TFT_BLACK);
    _display->setFont(&fonts::Font4);
    _display->setTextColor(TFT_WHITE, TFT_BLACK);
    _display->setTextDatum(textdatum_t::top_left);
    _display->drawString("Files:", PANEL_X, FILES_LABEL_Y + 2);

    // White border box
    _display->drawRect(PANEL_X, FILES_LIST_Y, PANEL_WIDTH, FILES_LIST_HEIGHT, TFT_WHITE);

    // Clear list interior
    _display->fillRect(PANEL_X + 1, FILES_LIST_Y + 1, PANEL_WIDTH - 2, FILES_LIST_HEIGHT - 2, TFT_BLACK);

    // Scroll-up arrow (grey when already at the top)
    const bool canScrollUp = (_scrollOffset > 0);
    const uint16_t upColour = canScrollUp ? static_cast<uint16_t>(TFT_WHITE) : static_cast<uint16_t>(TFT_DARKGREY);
    const int upArrowCentreX = PANEL_X + PANEL_WIDTH / 2;
    const int upArrowCentreY = FILES_LIST_Y + SCROLL_ARROW_HEIGHT / 2;
    _display->fillTriangle(upArrowCentreX, upArrowCentreY - 6, upArrowCentreX - 8, upArrowCentreY + 5, upArrowCentreX + 8, upArrowCentreY + 5, upColour);

    // Scroll-down arrow (grey when already at the bottom)
    const int totalFiles = static_cast<int>(_files.size());
    const bool canScrollDown = (totalFiles > 0) && ((_scrollOffset + VISIBLE_ITEMS) < totalFiles);
    const uint16_t downColour = canScrollDown ? static_cast<uint16_t>(TFT_WHITE) : static_cast<uint16_t>(TFT_DARKGREY);
    const int downArrowCentreX = PANEL_X + PANEL_WIDTH / 2;
    const int downArrowCentreY = FILES_LIST_BOTTOM - SCROLL_ARROW_HEIGHT / 2;
    _display->fillTriangle(downArrowCentreX, downArrowCentreY + 6, downArrowCentreX - 8, downArrowCentreY - 5, downArrowCentreX + 8, downArrowCentreY - 5, downColour);

    // File list items
    const int itemsAreaY = FILES_LIST_Y + SCROLL_ARROW_HEIGHT;
    const int textX = PANEL_X + 6;

    _display->setFont(&fonts::Font4);
    _display->setTextDatum(textdatum_t::middle_left);

    for (int i = 0; i < VISIBLE_ITEMS; ++i)
    {
        const int fileIndex = _scrollOffset + i;
        const int itemY = itemsAreaY + (i * LIST_ITEM_HEIGHT);
        const int itemCentreY = itemY + LIST_ITEM_HEIGHT / 2;

        if (fileIndex < totalFiles)
        {
            const bool selected = (fileIndex == _selectedFile);
            const uint16_t bgColour = selected ? static_cast<uint16_t>(TFT_WHITE) : static_cast<uint16_t>(TFT_BLACK);
            const uint16_t textColour = selected ? static_cast<uint16_t>(TFT_BLACK) : static_cast<uint16_t>(TFT_WHITE);

            _display->fillRect(PANEL_X + 1, itemY, PANEL_WIDTH - 2, LIST_ITEM_HEIGHT, bgColour);

            char nameBuffer[40];
            const std::string name = Basename(_files[fileIndex]);
            snprintf(nameBuffer, sizeof(nameBuffer), "%s", name.c_str());

            _display->setTextColor(textColour, bgColour);
            _display->drawString(nameBuffer, textX, itemCentreY);
        }
    }

    _display->endWrite();
}

/**
 * @brief Draws the Load and Stop/Run action buttons.
 *
 * The Load button label is always "Load".  The Stop/Run button label
 * is "Run" when the CPU is halted, or "Stop" when it is running.
 */
void ControlPanel::DrawActionButtons()
{
    DrawButton(PANEL_X, LOAD_BUTTON_Y, PANEL_WIDTH, BUTTON_HEIGHT, "Load", _loadEnabled);
    DrawButton(PANEL_X, STOPRUN_BUTTON_Y, PANEL_WIDTH, BUTTON_HEIGHT, _running ? "Stop" : "Run", _stopRunEnabled);
}

/**
 * @brief Draws a single rounded-rectangle button.
 *
 * Enabled style: white border, black fill, white label text.
 * Disabled style: grey border, black fill, grey label text.
 *
 * @param x        Left edge of the button in screen co-ordinates.
 * @param y        Top edge of the button in screen co-ordinates.
 * @param width    Width of the button in pixels.
 * @param height   Height of the button in pixels.
 * @param label    Null-terminated text to draw centred inside the button.
 * @param enabled  true for the enabled style; false for the disabled style.
 */
void ControlPanel::DrawButton(int x, int y, int width, int height, const char *label, bool enabled)
{
    const uint16_t colour = enabled ? static_cast<uint16_t>(TFT_WHITE) : static_cast<uint16_t>(TFT_DARKGREY);

    _display->startWrite();

    _display->fillRoundRect(x, y, width, height, CORNER_RADIUS, TFT_BLACK);
    _display->drawRoundRect(x, y, width, height, CORNER_RADIUS, colour);

    _display->setFont(&fonts::Font4);
    _display->setTextColor(colour, TFT_BLACK);
    _display->setTextDatum(textdatum_t::middle_center);
    _display->drawString(label, x + width / 2, y + height / 2);

    _display->endWrite();
}

// ---------------------------------------------------------------------------
// Private touch handling
// ---------------------------------------------------------------------------

/**
 * @brief Touch callback registered with TouchInput.
 *
 * Detects press edges (the moment a finger first contacts the screen) by
 * comparing the current touch state against the previous state.  Only new
 * press events (count transitions from 0 to > 0) are forwarded to
 * HandlePress().  Events with all fingers lifted reset the edge state.
 *
 * @param points  Array of screen-space touch co-ordinates.
 * @param count   Number of active touch points; zero when all fingers lifted.
 */
void ControlPanel::OnPanelTouch(const lgfx::touch_point_t *points, int count)
{
    const bool touched = (count > 0);

    if (touched && !_prevTouched)
    {
        HandlePress(points[0].x, points[0].y);
    }

    _prevTouched = touched;
}

/**
 * @brief Processes a newly detected touch press on the control panel.
 *
 * Tests the touch co-ordinate against each interactive region in order:
 * speed radio buttons, file list scroll arrows, file list items, Load
 * button, Stop/Run button.  Touches whose X co-ordinate falls to the left
 * of the control section are silently ignored.
 *
 * @param touchX  Screen X co-ordinate of the press.
 * @param touchY  Screen Y co-ordinate of the press.
 */
void ControlPanel::HandlePress(int touchX, int touchY)
{
    // Must be in the control panel column
    if (touchX < CONTROL_SECTION_X)
    {
        return;
    }

    // -------------------------------------------------------------------
    // Speed radio buttons (side by side on one row)
    // -------------------------------------------------------------------
    {
        const int halfWidth = PANEL_WIDTH / 2;
        const int rowCentreY = SPEED_Y + 38;
        const int rowHalfH = LIST_ITEM_HEIGHT / 2;

        if (HitTest(touchX, touchY, PANEL_X, rowCentreY - rowHalfH, halfWidth, LIST_ITEM_HEIGHT))
        {
            _speedSetting = SpeedSetting::Maximum;
            DrawSpeedSection();
            _display->display();
            return;
        }

        if (HitTest(touchX, touchY, PANEL_X + halfWidth, rowCentreY - rowHalfH, halfWidth, LIST_ITEM_HEIGHT))
        {
            _speedSetting = SpeedSetting::Original;
            DrawSpeedSection();
            _display->display();
            return;
        }
    }

    // -------------------------------------------------------------------
    // Scroll-up arrow
    // -------------------------------------------------------------------
    if (HitTest(touchX, touchY, PANEL_X, FILES_LIST_Y, PANEL_WIDTH, SCROLL_ARROW_HEIGHT))
    {
        if (_scrollOffset > 0)
        {
            _scrollOffset--;
            DrawFileList();
            _display->display();
        }
        return;
    }

    // -------------------------------------------------------------------
    // Scroll-down arrow
    // -------------------------------------------------------------------
    if (HitTest(touchX, touchY, PANEL_X, FILES_LIST_BOTTOM - SCROLL_ARROW_HEIGHT, PANEL_WIDTH, SCROLL_ARROW_HEIGHT))
    {
        const int totalFiles = static_cast<int>(_files.size());
        if ((_scrollOffset + VISIBLE_ITEMS) < totalFiles)
        {
            _scrollOffset++;
            DrawFileList();
            _display->display();
        }
        return;
    }

    // -------------------------------------------------------------------
    // File list item selection
    // -------------------------------------------------------------------
    {
        const int itemsAreaY = FILES_LIST_Y + SCROLL_ARROW_HEIGHT;
        const int itemsAreaBottom = FILES_LIST_BOTTOM - SCROLL_ARROW_HEIGHT;
        const int itemsAreaHeight = itemsAreaBottom - itemsAreaY;

        if (HitTest(touchX, touchY, PANEL_X, itemsAreaY, PANEL_WIDTH, itemsAreaHeight))
        {
            const int relativeY = touchY - itemsAreaY;
            const int itemIndex = relativeY / LIST_ITEM_HEIGHT;
            const int fileIndex = _scrollOffset + itemIndex;
            const int totalFiles = static_cast<int>(_files.size());

            if (fileIndex >= 0 && fileIndex < totalFiles)
            {
                _selectedFile = fileIndex;
                DrawFileList();
                DrawActionButtons();
                _display->display();
            }
            return;
        }
    }

    // -------------------------------------------------------------------
    // Load button
    // -------------------------------------------------------------------
    if (_loadEnabled && HitTest(touchX, touchY, PANEL_X, LOAD_BUTTON_Y, PANEL_WIDTH, BUTTON_HEIGHT))
    {
        if (_loadCallback && _selectedFile >= 0 && _selectedFile < static_cast<int>(_files.size()))
        {
            _loadCallback(_files[_selectedFile]);
        }
        return;
    }

    // -------------------------------------------------------------------
    // Stop/Run button
    // -------------------------------------------------------------------
    if (_stopRunEnabled && HitTest(touchX, touchY, PANEL_X, STOPRUN_BUTTON_Y, PANEL_WIDTH, BUTTON_HEIGHT))
    {
        if (_stopRunCallback)
        {
            _stopRunCallback(!_running);
        }
        return;
    }
}

/**
 * @brief Returns true if the touch point falls within the given rectangle.
 *
 * @param touchX  X co-ordinate of the touch point.
 * @param touchY  Y co-ordinate of the touch point.
 * @param x       Left edge of the test rectangle.
 * @param y       Top edge of the test rectangle.
 * @param width   Width of the test rectangle in pixels.
 * @param height  Height of the test rectangle in pixels.
 * @return bool   true if the point lies inside (inclusive of edges).
 */
bool ControlPanel::HitTest(int touchX, int touchY, int x, int y, int width, int height)
{
    return (touchX >= x && touchX < (x + width) && touchY >= y && touchY < (y + height));
}

/**
 * @brief Extracts the filename component from a full file path.
 *
 * Returns the substring following the last '/' character.  If no '/'
 * is present the full string is returned unchanged.
 *
 * @param path  Full file path, e.g. "/sdcard/program.ssem".
 * @return std::string  The filename portion, e.g. "program.ssem".
 */
std::string ControlPanel::Basename(const std::string &path)
{
    const size_t pos = path.rfind('/');
    return (pos != std::string::npos) ? path.substr(pos + 1) : path;
}
