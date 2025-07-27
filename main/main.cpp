/**
 * @file main.cpp
 * @brief Main firmware entry point for the Grid Board project.
 *
 * Grid Board is an interactive 12x5 grid display system built with ESP32-P4 Nano with 10.1-inch display (Waveshare)
 * and LVGL, featuring animated characters, emoji support, and Bluetooth Low Energy (BLE)
 * communication with a Flutter-based mobile controller.
 *
 * This project is designed for creative message display, 
 * and mobile-to-device communication over BLE.
 *
 * Author: Eric Nam
 * YouTube: https://youtube.com/@thatproject
 * Repository: https://github.com/0015/Grid_Board
 */

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "ble_server.h"
#include "esp_heap_caps.h"
#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"
#include "bsp/display.h"
#include "lvgl.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

#include "grid_board.hpp"

extern const uint8_t card_pcm_start[] asm("_binary_card_pcm_start");
extern const uint8_t card_pcm_end[] asm("_binary_card_pcm_end");

// Global grid board instance
static GridBoard grid_board;

static const char *device_name = "Grid_Board";
static std::string target_text = "              WELCOME😀       TO     📌GRID BOARD❤            ";
static QueueHandle_t sfx_queue = nullptr;

void card_flip_sfx_task(void *param)
{
    static int64_t last_sfx_time = 0;
    const int MIN_SFX_INTERVAL_MS = 33;
    size_t pcm_size = card_pcm_end - card_pcm_start;
    size_t bytes_written = 0;
    bool active = true;

    while (1)
    {
        uint8_t msg;
        if (xQueueReceive(sfx_queue, &msg, portMAX_DELAY) == pdTRUE)
        {
            if (msg == 0)
            {
                // "Pause" - just go idle until new non-zero request arrives
                active = false;
                continue;
            }
            // Only play if in active mode
            int64_t now = esp_timer_get_time() / 1000; // ms
            if (now - last_sfx_time >= MIN_SFX_INTERVAL_MS)
            {
                last_sfx_time = now;
                bsp_extra_i2s_write((void *)card_pcm_start, pcm_size, &bytes_written, 200);
            }
            active = true;
        }
    }
}

void start_sfx_task()
{
    if (!sfx_queue)
        sfx_queue = xQueueCreate(8, sizeof(uint8_t));
    xTaskCreate(card_flip_sfx_task, "card_flip_sfx_task", 4096, nullptr, 3, nullptr);
}

void start_card_flip_sound_task()
{
    if (sfx_queue)
    {
        uint8_t msg = 1;
        xQueueSend(sfx_queue, &msg, 0);
    }
}

void stop_card_flip_sound_task()
{
    if (sfx_queue)
    {
        // Flush any queued SFX requests
        uint8_t dummy;
        while (xQueueReceive(sfx_queue, &dummy, 0) == pdTRUE)
            ;
        uint8_t exit_msg = 0;
        xQueueSend(sfx_queue, &exit_msg, 0);
    }
}

// Main UI initialization function
void ui_gridboard_animation_start(lv_obj_t *parent)
{
    // Set up the sound callback
    grid_board.set_sound_callback(start_card_flip_sound_task, stop_card_flip_sound_task);

    // Initialize the grid board
    grid_board.initialize(parent);

    if (target_text.empty())
    {
        ESP_LOGI(device_name, "Target text is empty, skipping animation.");
        return;
    }

    lv_timer_create([](lv_timer_t *t)
                    {
        lv_timer_del(t);
        
        // Process and animate the welcome text
        grid_board.process_text_and_animate(target_text); }, 5000, NULL);
}

// Audio system initialization
void app_audio_init()
{
    ESP_ERROR_CHECK(bsp_extra_codec_init());
    ESP_ERROR_CHECK(bsp_extra_player_init());

    // Set volume and unmute
    bsp_extra_codec_volume_set(80, NULL);
    bsp_extra_codec_mute_set(false);

    // Enable power amplifier
    gpio_set_direction(BSP_POWER_AMP_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(BSP_POWER_AMP_IO, 1);

    ESP_LOGI("AUDIO", "Audio system initialized");
}

// BLE callback functions
static void on_connect(bool connected)
{
    ESP_LOGI(device_name, "BLE %s", connected ? "Connected" : "Disconnected");
}

static void on_data_received(const uint8_t *data, uint16_t len)
{
    // Convert received data to string
    std::string received_text;
    received_text.reserve(len + 1);

    for (int i = 0; i < len; i++)
    {
        received_text += (char)data[i];
    }

    ESP_LOGI(device_name, "Received text: '%s' (length: %d)", received_text.c_str(), len);

    // Update the target text and animate
    target_text = received_text;

    // Wait for any current animations to finish before starting new ones
    while (grid_board.is_animation_running())
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    bsp_display_lock(0);
    // Process and animate the new text
    grid_board.process_text_and_animate(target_text);
    bsp_display_unlock();
}

extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ble_server_register_callbacks(on_connect, on_data_received);
    ble_server_start(device_name);

    // Initialize audio system
    app_audio_init();

    start_sfx_task();

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        //.buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .buffer_size = 800 * 10, // 10 lines buffer (16kB at RGB565)
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = true,
        }};
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

    lv_disp_t *disp = lv_disp_get_default();
    bsp_display_rotate(disp, LV_DISP_ROTATION_90);

    bsp_display_lock(0);
    ui_gridboard_animation_start(lv_screen_active());
    bsp_display_unlock();
}
