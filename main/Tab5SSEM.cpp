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

#include <dirent.h>
#include <sys/stat.h>

#include "Display.hpp"
#include "Rtc.hpp"
#include "SDCard.hpp"
#include "Touch.hpp"
#include <string>
#include <vector>
#include "StoreLines.hpp"
#include "Compiler.hpp"
#include "Instructions.hpp"

using namespace std;

/**
 * @brief Tagused for logging the main component.
 */
const char *LOG_TAG = "Tab5SSEM";

/**
 * @brief Global display instance used by M5GFX and the SSEM Display component.
 */
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
        setTime.tm_year = 2026 - 1900;
        setTime.tm_mon = 3 - 1; // March (0-based)
        setTime.tm_mday = 21;
        setTime.tm_hour = 14;
        setTime.tm_min = 42;
        setTime.tm_sec = 0;
        setTime.tm_wday = 6; // Saturday
        setTime.tm_isdst = -1;
        rtc->SetTime(setTime);
    }

    Display::Run(display, sdCard);

    // Initialise the SSEM instructions lookup table.
    Instructions::PopulateLookupTable();
}

/**
 * @brief Read the names of all SSEM program files on the SD card.
 *
 * Scans the SD card mount point and returns the full path of every file
 * whose name ends with the ".ssem" extension (case-sensitive).
 *
 * @return vector<string> Vector of full file paths for each
 *         ".ssem" file found.  The vector is empty if the SD card could
 *         not be read or no matching files are present.
 */
vector<string> ReadSdCardFileNames()
{
    ESP_LOGI(LOG_TAG, "SD card contents:");

    static constexpr const char *SSEM_EXTENSION = ".ssem";
    static constexpr size_t SSEM_EXTENSION_LENGTH = 5U;

    vector<string> filenames;

    DIR *dp = opendir(SDCard::MOUNT_POINT);
    if (dp != nullptr)
    {
        struct dirent *ep;
        while ((ep = readdir(dp)) != nullptr)
        {
            if (ep->d_name[0] == '.')
            {
                continue;
            }

            const string name = ep->d_name;
            if ((name.size() > SSEM_EXTENSION_LENGTH) && name.compare(name.size() - SSEM_EXTENSION_LENGTH, SSEM_EXTENSION_LENGTH, SSEM_EXTENSION) == 0)
            {
                string fullPath = string(SDCard::MOUNT_POINT) + "/" + name;
                struct stat fileInfo;
                if (stat(fullPath.c_str(), &fileInfo) == 0)
                {
                    ESP_LOGI(LOG_TAG, "    %s, %" PRIu32, ep->d_name, fileInfo.st_size);
                    filenames.push_back(fullPath);
                }
                else
                {
                    ESP_LOGE(LOG_TAG, "    %s, stat failed: %s", ep->d_name, strerror(errno));
                }
            }
        }
        closedir(dp);
    }
    else
    {
        ESP_LOGE(LOG_TAG, "Failed to open directory %s", SDCard::MOUNT_POINT);
    }

    return (filenames);
}

/**
 * @brief Read the contents of the specified file, one line per entry.
 *
 * Opens the file at the given path for reading.  Each line is stripped of
 * its trailing newline and carriage-return characters before being appended
 * to the result vector.  Blank lines and lines that could not be read are
 * skipped.
 *
 * @param filename  Full path to the file to read (e.g. "/sdcard/Add.ssem").
 * @return vector<string>  File contents with one entry per line.
 *         The vector is empty if the file could not be opened.
 */
vector<string> ReadSdCardFileContents(const string &filename)
{
    vector<string> lines;

    string fullPath = string(SDCard::MOUNT_POINT) + "/" + filename;
    FILE *file = fopen(fullPath.c_str(), "r");
    if (file == nullptr)
    {
        ESP_LOGE(LOG_TAG, "Failed to open file %s: %s", fullPath.c_str(), strerror(errno));
        return (lines);
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file) != nullptr)
    {
        string line = buffer;

        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
        {
            line.pop_back();
        }

        if (!line.empty())
        {
            lines.push_back(line);
        }
    }

    fclose(file);
    return (lines);
}

extern "C" void app_main(void)
{
    Setup();

    SDCard *sdCard = SDCard::GetInstance();
    if (sdCard != nullptr)
    {
        if (sdCard->IsMounted())
        {
            ESP_LOGI(LOG_TAG, "SD card is mounted.");
            ReadSdCardFileNames();
        }
        else
        {
            ESP_LOGE(LOG_TAG, "SD card is NOT mounted.");
        }
    }
    else
    {
        ESP_LOGE(LOG_TAG, "SDCard instance is null.");
    }

    Display::DisplayMessage message = {};

    for (int i = 0; i < Display::STORELINE_COUNT; ++i)
    {
        message.storelineValues[i] = 0U;
        snprintf(message.storelineText[i], sizeof(message.storelineText[i]), "JP 0");
    }

    message.controlState = nullptr;
    Display::PostMessage(message);

    if (sdCard != nullptr && sdCard->IsMounted())
    {
        string targetFile = "hfr989.ssem";

        ESP_LOGI(LOG_TAG, "Attempting to read file: %s", targetFile.c_str());
        vector<string> fileContents = ReadSdCardFileContents(targetFile);
        for (const auto &line : fileContents)
        {
            ESP_LOGI(LOG_TAG, "    %s", line.c_str());
        }
        if (!fileContents.empty())
        {
            StoreLines storeLines = Compiler::Compile(fileContents);
            ESP_LOGI(LOG_TAG, "Contents of %s:", targetFile.c_str());
            uint32_t lineNumber = 0;
            for (const auto &line: storeLines)
            {
                ESP_LOGI(LOG_TAG, "    %u: %s", lineNumber++, line.Disassemble().c_str());
            }
        }
        else
        {
            ESP_LOGE(LOG_TAG, "File contents are empty or file could not be read: %s", targetFile.c_str());
        }
    }

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
        vTaskDelay(pdMS_TO_TICKS(10));

        ++count;
    }
}
