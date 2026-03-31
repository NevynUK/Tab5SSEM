# Add SSEM Interface

## Splash Screen

The splash screen should be modified to inform the user that this is the SSEM Emulator.

Additionally the splash screen should display the SD card information, namely

- Is the SD Card present?
- SD Card capacity

The splash screen shall be removed if either of the following conditions occur:

- The user touches the screen
- 5 seconds has elapsed since the screen was displayed

## Main Interface

The main interface shall have a banner at the top of the screen and a footer at the bottom of the screen.  Both header and footer shall have a white background and the text shall be black.

The header font shall be 14 pixels.

The footer font shall be 12 pixels.

### Centre Panel

The centre panel shall be divided into three sections from left to right:

- Storeline LEDs
- Storeline Text
- Control Panel

#### Storeline LEDs

The Storelines contents is made up of 32 unsigned integers.  Each of the Storelines is displayed as 32 LEDs.  Each Storeline shall be on its own line.

The two states of an LED are represented as follows:

- On: White outer circle filled green
- Off: White outer circle filled black

The LEDs shall be sized to fill the space between the header and the footer.

The LED area (all storeline rows combined) shall be enclosed in a white-bordered box with 4 pixels of padding on all sides.

#### Storeline Text

Each of the store lines shall have a line of text to the right of the LEDs.

The text column shall have a left margin of 16 pixels to provide separation from the LED column.

The text area (all storeline text lines combined) shall be enclosed in a white-bordered box with 4 pixels of padding on all sides.

Initially each line will be set to `JP 0`.

### Control Panel

This will initially be blank.

## Display Task

The Display code should run in a FreeRTOS task.  The code should use a message queue to receive the state of the Display.  Create a message definition that will hold

- Storeline values
- Storeline text
- Control State (initially a `nullptr`)

The code controlling the messages shall be in the `app_main` function.  This will:

- Set the storelines to 0
- Set the Storeline text to `JP 0`
- Set the control state to `nullptr`

The code will loop counting from 0 to UINT_MAX.  Each of the Storelines will be set to the count value.  The code will use `vTaskDelay` to wait for 1 second between each value.

The Display class will receive the message from the message queue and use the data in the message to update the display.

Each Storeline should have a number to the left of the LEDs.  This can exist outside of the box containing the LEDs.