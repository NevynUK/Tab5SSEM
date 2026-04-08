# SSEM Interface Specification

## Splash Screen

The splash screen informs the user that this is the SSEM Emulator.

The splash screen also displays SD card information:

- Is the SD card present?
- SD card capacity

The splash screen is dismissed when either of the following conditions occur:

- The user touches the screen
- 5 seconds have elapsed since the screen was displayed

## Main Interface

The main interface has a banner at the top of the screen and a footer at the bottom of the screen.  Both header and footer have a white background with black text.

The header font is 14 pixels.

The footer font is 12 pixels.

### Centre Panel

The centre panel is divided into four sections from left to right:

- Storeline number column
- Storeline LEDs
- Storeline text
- Control panel

#### Storeline Number Column

Each storeline row has a zero-based row index (0–31) drawn to the left of the LED box.  This number sits outside the LED box and is 28 pixels wide.

#### Storeline LEDs

The storeline contents are made up of 32 unsigned 32-bit integers.  Each storeline is displayed as a row of 32 LEDs, one per bit.  Each storeline occupies its own row.

The two states of an LED are represented as follows:

- On: white outer circle filled green
- Off: white outer circle filled black

Each LED cell is 20 × 20 pixels.  The outer circle radius is 9 pixels; the inner fill circle radius is 7 pixels, leaving a 2-pixel white border.

The 32 LEDs in each row are grouped into 8 groups of 4, with an 8-pixel gap between adjacent groups.

The total width of the LED section is `(20 × 32) + (7 × 8)` = **696 pixels**.

The LEDs are sized to fill the space between the header and the footer.

The LED area (all storeline rows combined) is enclosed in a white-bordered box with 4 pixels of padding on all sides.

#### Storeline Text

Each storeline has a text column to the right of the LED section.

The text column has a left margin of 16 pixels to provide visual separation from the LED column.

Each storeline text row contains two fields, left to right:

1. **Hex value** — the storeline word rendered as an 8-digit zero-padded uppercase hexadecimal number (no `0x` prefix), e.g. `0000001A`.  Reserved column width: 72 pixels.
2. **Mnemonic label** — a short instruction label string, initially `JMP 0`.

The text area (all storeline text rows combined) is enclosed in a white-bordered box with 4 pixels of padding on all sides.

## Control Panel

The control panel occupies the remaining area to the right of the storeline text column.  It is inset by 8 pixels of padding on all sides.  The following controls appear from top to bottom in the order listed.

### Halted / Running Indicator

A white-bordered rounded rectangle (48 pixels tall, corner radius 8 pixels) spans the full panel width at the top of the panel.  It displays one of two states:

| State | Text | Text colour |
|---|---|---|
| Halted | `Halted` | Red |
| Running | `Running` | Green |

The indicator is redrawn whenever the running state changes.  It can also be updated programmatically via `Display::SetRunning(bool running)`.

### Speed Radio Buttons

A "Speed:" label appears above two side-by-side radio buttons:

| Option | Meaning |
|---|---|
| Maximum | Run as fast as the hardware allows |
| Original | Simulate the original 1948 Manchester Baby clock rate |

The selected option has its inner circle filled white; the unselected option has a hollow circle (black inner fill) with a white outer ring.

The currently selected speed is available via `Display::GetSpeed()`, which returns a `Display::SpeedSetting` enum value.  The default on startup is `SpeedSetting::Maximum`.

**Disabled state**: while the SSEM CPU is executing, the speed section is rendered entirely in dark grey (label, radio-button outlines, inner fills, and option text).  Touch events on the speed radio buttons are ignored.  The section is re-enabled when execution stops.

### Files

A "Files:" label appears above a scrollable list box.  The list is populated by calling `Display::SetFiles(const vector<string> &files)`, which resets the scroll offset and clears any existing selection.  Only the filename portion of each path is displayed (the directory prefix is stripped).

The list box has:

- A white-bordered outer box.
- A scroll-up arrow at the top of the box.  The arrow is white when upward scrolling is available, dark grey otherwise.
- A scroll-down arrow at the bottom of the box.  The arrow is white when downward scrolling is available, dark grey otherwise.
- A row of visible file entries between the arrows.  Each row is 26 pixels tall; the number of visible rows is determined by the available height.
- The selected row (if any) is highlighted with a white background and black text.  All other rows have a black background and white text.

**Disabled state**: while the SSEM CPU is executing, all list items are rendered with dark grey text on a black background, both scroll arrows are dark grey, and touch events on the list (item selection and scroll arrows) are ignored.

### Load Button

A rounded-rectangle button labelled **Load** spans the full panel width above the Stop/Run button.  It is positioned at the bottom of the panel, immediately above the Stop/Run button.

| State | Border | Text |
|---|---|---|
| Enabled | White | White |
| Disabled | Dark grey | Dark grey |

The Load button is enabled automatically when a file is selected in the list.  It becomes disabled whenever the SSEM CPU starts running and is restored to its previous enabled state (enabled if a file was already selected) when execution stops.

When the enabled Load button is pressed, `Display::LoadCallback` is called with the full path of the currently selected file.  The callback is registered via `Display::SetLoadCallback()`.

### Stop / Run Button

A rounded-rectangle button spans the full panel width at the very bottom of the panel.

| Running state | Button label | Border | Text |
|---|---|---|---|
| Halted | `Run` | White | White |
| Running | `Stop` | White | White |
| Disabled (either) | — | Dark grey | Dark grey |

The button is enabled by posting a `DisplayMessage` with a non-null `controlState` field (see [Display Task Behaviour](#display-task-behaviour)).  It can also be enabled or disabled at any time via `Display::SetStopRunEnabled(bool enabled)`.

#### Pressing Run (CPU halted → running)

When the **Run** button is pressed:

1. `_running` is set to `true`.
2. The Halted/Running indicator changes to **Running** (green text).
3. The speed radio buttons are disabled (rendered in dark grey; touch events ignored).
4. The Load button is disabled.
5. The file list items and scroll arrows are rendered in dark grey; touch events on the list are ignored.
6. The button label changes to **Stop**.
7. The framebuffer is flushed to the display.
8. `Display::StopRunCallback` is called with `true` (running).

#### Pressing Stop (CPU running → halted)

When the **Stop** button is pressed:

1. `_running` is set to `false`.
2. The Halted/Running indicator changes to **Halted** (red text).
3. The speed radio buttons are re-enabled (white, touch events active).
4. The Load button is re-enabled if a file is currently selected; otherwise it remains disabled.
5. The file list items and scroll arrows are re-enabled.
6. The button label changes to **Run**.
7. The framebuffer is flushed to the display.
8. `Display::StopRunCallback` is called with `false` (halted).

The callback is registered via `Display::SetStopRunCallback()`.

## Display Task

The Display code runs in a dedicated FreeRTOS task (`DisplayTask`, stack 8192 bytes, priority 5).  Communication uses a depth-4 message queue; `PostMessage()` blocks until the message is accepted.

### Message Definition (`DisplayMessage`)

The message holds a complete snapshot of the display state:

| Field | Type | Description |
|---|---|---|
| `storelineValues` | `uint32_t[32]` | Current value of each storeline word |
| `storelineText` | `char[32][32]` | Mnemonic label for each storeline |
| `controlState` | `void *` | `nullptr` = no change to Stop/Run state; non-null = enable the Stop/Run button |

### Display Task Behaviour

The Display task blocks on `xQueueReceive`.  On each message received it:

1. Compares each incoming storeline value against the stored value.  Only rows whose value has changed are flagged for redraw.
2. Calls `DrawAllStorelines()` to redraw all flagged rows.
3. If `controlState` is non-null, enables the Stop/Run button and redraws the action buttons.
4. Calls `display()` to flush the framebuffer to the MIPI-DSI panel.

### `app_main` Behaviour

The `app_main` function:

1. Calls `Setup()` to initialise the display and all peripherals.
2. Calls `ClearStoreLinesAndUpdateDisplay()`, which posts a `DisplayMessage` with all storeline values set to `0`, all labels set to `JMP 0`, and `controlState` set to `nullptr`.
3. Registers `LoadFile` as the `LoadCallback` via `Display::SetLoadCallback()`.
4. Registers `OnStopRunPressed` as the `StopRunCallback` via `Display::SetStopRunCallback()`.
5. If the SD card is mounted, scans it for `.ssem` files and passes the result to `Display::SetFiles()`.
6. Calls `vTaskDelete(nullptr)` — the `app_main` task is deleted; all further activity is driven by FreeRTOS tasks owned by the component layer.

### Load File Behaviour (`LoadFile` callback)

When the Load button is pressed and a valid file is selected, `LoadFile` is invoked with the full file path.  It:

1. Reads the file from the SD card line by line.
2. Compiles the SSEM source into store lines via `Compiler::Compile()`.
3. Destroys any existing `Cpu` instance and creates a new one from the compiled store lines.
4. Calls `Cpu::Reset()`.
5. Posts a `DisplayMessage` with the compiled storeline values and disassembled mnemonics, and `controlState` set to a non-null sentinel value to enable the Stop/Run button.

### Stop/Run Callback Behaviour (`OnStopRunPressed` callback)

`OnStopRunPressed(bool running)` is invoked by the Display layer after the button state and all visual updates have been applied.  It receives the new intended running state (`true` = start execution, `false` = stop) and is responsible for starting or stopping the SSEM CPU accordingly.

#### Load Button

The load button can only be enabled when:

- No program is running
- A file has been selected.

#### Stop / Run Button

This button is only enabled when a file has been selected.

The button will raise a callback.

An API allows the state to be changed.