# List of source files
set(SOURCES
    "main.cpp"
    "grid_board.cpp"
    "ble_server.c"
    "gatt_svr.c"
    "ShareTech140.c"
    "NotoEmoji64.c"
)

# List of include directories
set(INCLUDE_DIRS
    "."
)

# Required components
set(REQUIRED_COMPONENTS
    nvs_flash
    bt
    lvgl   
    esp32_p4_nano
    esp_lcd_touch
    esp_lcd_touch_gt911
    bsp_extra
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES ${REQUIRED_COMPONENTS}
    EMBED_FILES "card.pcm"
)
