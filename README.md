# Grid Board Project

Grid Board is an advanced, large-screen animated character display system based on the **ESP32-P4** platform. This project leverages the **DSI interface** of ESP32-P4 to drive a 10.1" LCD panel at high speed, allowing smooth, visually impressive character animations—including emoji—across a 12×5 grid.

---

## Features

- **12×5 grid** for displaying custom characters, numbers, and emoji
- **Animated "card falling" effect** for character updates
- **Unicode & emoji support** via hand-curated font sets
- **BLE communications** using [NimBLE](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/nimble/index.html) stack (see below)
- **DSI high-speed screen refresh** for flicker-free animations on large LCDs
- **Touch support** (GT911) and optional sound effect playback (ES8311, card.pcm)

---

## Hardware & Architecture

- **Main Board:** [Waveshare ESP32-P4 Nano](https://www.waveshare.com/esp32-p4-nano.htm)
- **Display:** 10.1” JD9365 LCD panel, driven via MIPI DSI
- **Touch Panel:** GT911 via I2C
- **BLE Connectivity:** 
    - ESP32-P4 lacks native Bluetooth/Wi-Fi
    - BLE is enabled by connecting an **ESP32-C6** via **SDIO** using [esp-hosted](https://github.com/espressif/esp-hosted) and [esp_wifi_remote](https://github.com/espressif/esp-hosted/tree/main/components/esp_wifi_remote)
    - Firmware uses **NimBLE** stack for robust BLE GATT server implementation

---

## Major Components Used

- `ESP-IDF` **v5.4.2** (project developed and tested with this version)
- `lvgl/lvgl` **v9.2.\***: UI, animation, and grid rendering
- `waveshare/esp_lcd_jd9365_10_1`: LCD panel driver
- `waveshare/esp32_p4_nano`: Board support package
- `esp_lcd_touch_gt911`: Touch controller
- `espressif/esp_hosted` & `esp_wifi_remote`: BLE over SDIO
- `espressif/es8311`: Optional I2S sound
- **Custom fonts:** `ShareTech140.c` (pixel mono), `NotoEmoji64.c` (emoji set)

See [`idf_component.yml`](idf_component.yml) for full dependency list.

---

## Custom Character & Emoji Rendering

A major highlight of Grid Board is the **custom-designed character set**:

- **Character Set:** Includes all uppercase English letters, digits, common punctuation, and hand-picked emoji.
- **Font Creation:** The main grid font (`ShareTech140`) is pixel-based, optimized for readability at large sizes on the LCD.
- **Emoji Support:** Emoji glyphs from [Noto Emoji](https://fonts.google.com/noto/specimen/Noto+Emoji) are rasterized into a separate C array, with efficient lookups for fast drawing.

This careful curation and rasterization allow for:
- Lightning-fast grid updates (no glyph loading at runtime)
- Consistent style and alignment across the grid
- Rendering even on resource-constrained MCUs

---

## Animation Technique: Card Falling Effect

The signature animation is the **"card falling"** transition when updating grid characters.

### How it works:

1. **Grid Representation:** Each cell in the 12×5 grid holds a character or emoji, rendered via LVGL's canvas API for pixel-perfect control.
2. **Animation Trigger:** When a new message is received over BLE, each character or emoji is sequentially inserted into the grid, one by one, following the intended order of the message. The firmware determines the target grid slots for each character, and schedules an animation for each slot as it is updated.
3. **Card Fall Algorithm:**
   - A "card" representing the next character or emoji is created above the target grid slot.
   - Using LVGL’s animation system, the card’s Y position is animated from above the slot down into place, using a linear or ease-in curve.
   - Each column or cell may be animated independently or with a small delay for a staggered, dynamic effect.
   - To optimize performance and avoid screen lag on large displays, the animation engine updates and animates only 10 slots at a time. Once the first batch of 10 slots completes its card-falling animation, the process continues with the next batch, repeating until the entire message is displayed. This batching approach ensures smooth and responsive animations even on high-resolution screens.
4. **Slot Completion:** When the falling card reaches the grid slot, it "lands" and replaces the previous content, possibly with a subtle bounce or sound.
5. **Efficient Redraw:** Only affected cells are updated, minimizing framebuffer operations—critical for large screens.

---

## BLE Communications (NimBLE, SDIO)

**Important:**  
ESP32-P4 does **not** have built-in Wi-Fi or Bluetooth.  
BLE functionality is achieved by:

- **Connecting ESP32-C6** as a slave over SDIO
- Running **esp-hosted** and **esp_wifi_remote** to expose Bluetooth to the main MCU
- Using **NimBLE** for a GATT server, so the board can receive messages, characters, and control commands from external BLE central devices (like a mobile app)

BLE integration is seamless for the user; the technical complexity is fully abstracted by this architecture.

---

## Build Structure

```bash
esp-idf_project/
├── main.cpp # Project entry point, grid logic
├── grid_board.cpp # Animation and display routines
├── ble_server.c # NimBLE BLE server logic
├── gatt_svr.c # BLE GATT service and characteristics
├── ShareTech140.c # Pixel font
├── NotoEmoji64.c # Emoji font
├── card.pcm # Optional sound effect
├── CMakeLists.txt # ESP-IDF build system configuration and source/component list
├── idf_component.yml # Component dependency manifest (external and internal components, versions)
├── sdkconfig # ESP-IDF project configuration (target, display size, memory, etc)
```

---

## Additional Notes

- The combination of **LVGL v9**'s animation engine and custom C array fonts enables smooth, high-fidelity graphics even for large-scale displays.
- All code and assets are tailored for performance and stability on Waveshare’s ESP32-P4 Nano platform.
- Touch input and audio feedback are supported but optional.

---

## Author

**Eric Nam**  
GitHub: [@0015](https://github.com/0015)  
YouTube: [@thatproject](https://youtube.com/@thatproject)

---

## Tags

`esp32-p4`, `grid-board`, `lvgl`, `emoji`, `ble`, `nimble`, `sdio`, `animation`, `display`, `waveshare`, `custom-font`, `pixel-art`


