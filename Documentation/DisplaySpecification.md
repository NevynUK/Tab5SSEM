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

The centre panel is divided into three sections from left to right:

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
2. **Mnemonic label** — a short instruction label string, initially `JP 0`.

The text area (all storeline text rows combined) is enclosed in a white-bordered box with 4 pixels of padding on all sides.

### Control Panel

This will initially be blank.

## Display Task

The Display code runs in a dedicated FreeRTOS task.  Communication uses a depth-1 message queue so that the task always processes the most recent state snapshot; any unread message is overwritten by a newer one.

### Message Definition (`DisplayMessage`)

The message holds a complete snapshot of the display state:

| Field | Type | Description |
|---|---|---|
| `storelineValues` | `uint32_t[32]` | Current value of each storeline word |
| `storelineText` | `char[32][32]` | Mnemonic label for each storeline |
| `controlState` | `void *` | Reserved — always `nullptr` for now |

### `app_main` Behaviour

The `app_main` function:

1. Calls `Setup()` to initialise the display and all peripherals.
2. Sends an initial message with all storeline values set to `0`, all labels set to `JP 0`, and `controlState` set to `nullptr`.
3. Loops from `0` to `UINT_MAX`, setting every storeline value to the current count, posting a message, and calling `vTaskDelay(pdMS_TO_TICKS(1000))` between each iteration.

### Display Task Behaviour

The Display task blocks on `xQueueReceive`.  On each message received it:

1. Copies the storeline values and labels into the static display state.
2. Calls `DrawAllStorelines()` to redraw all 32 rows.
3. Calls `display()` to flush the framebuffer to the MIPI-DSI panel.

## Control Panel

The control panel is the remaining area on the right of the screen.  The following controls / features are on the control panel starting at the top of the panel.  They will appear in the order presented.

### Halted / Running Indicator

The top of the screen will contain a white rounded rectangle.  The rectangle can contain the word Running (green text) or Halted (red text).  The contents can be changed with a API that takes a boolean running.

### Speed Radio Button

A speed button shall be available.  This should be two radio buttons with the options Maximum and original.  The value will be available through an API that returns an enum representing the speed.  The default on startup is maximum.

### Files

A scrollable list box will display a list of files.  The files shall be passed to an API as a vector<string> reference object.

### Load File Button and Stop / Run Button

Two buttons shall be displayed as a rounded rectangular button:

- Load
- Stop / Run

The buttons shall have a white text on a black background and a white border when enabled.  The white text and border shall be grey when the button is disabled.

The enabled state for a button is changed through an API for each button.

#### Load Button

The load button can only be enabled when:

- No program is running
- A file has been selected.

#### Stop / Run Button

This button is only enabled when a file has been selected.

The button will raise a callback.

An API allows the state to be changed.