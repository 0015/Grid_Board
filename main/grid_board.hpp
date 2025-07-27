#pragma once

#include "lvgl.h"
#include <string>
#include <vector>

// Grid dimensions
#define GRID_COLS 12
#define GRID_ROWS 5
#define GRID_SLOT_WIDTH 96
#define GRID_SLOT_HEIGHT 150
#define GRID_GAP 10
#define GRID_SCREEN_WIDTH 1280
#define GRID_SCREEN_HEIGHT 800

// Animation constants
#define MAX_PARALLEL_ANIMATIONS 10

// Forward declarations
LV_FONT_DECLARE(ShareTech140);
LV_FONT_DECLARE(NotoEmoji64);

// Character slot structure for animation management
typedef struct
{
    char utf8_char[8];
    int row;
    int col;
    int retry_index;
    std::vector<char> shuffled_chars;         
    std::vector<const char *> shuffled_emojis;
} GridCharacterSlot;

class GridBoard {
public:
    // Constructor/Destructor
    GridBoard();
    ~GridBoard();
    
    // Main interface functions
    void initialize(lv_obj_t *parent);
    void process_text_and_animate(const std::string& text);
    void clear_display();
    
    // Animation control
    void start_animation_batch();
    bool is_animation_running() const;
    
    // Callback for external sound triggering
    void set_sound_callback(void (*on_start)(), void (*on_end)());
    
private:
    // Grid management
    void create_grid(lv_obj_t *parent);
    lv_obj_t* create_card(lv_obj_t *slot, const char *text, const lv_font_t *font);
    
    // Animation functions
    void animate_card_to_slot(lv_obj_t *card, const char *target);
    static void animation_ready_callback(lv_anim_t *a);
    static void timer_callback(lv_timer_t *t);
    
    // Card dropping logic
    void on_card_dropped(lv_obj_t *card);
    
    // Utility functions
    bool is_emoji(const char *utf8_char);
    void utf8_to_upper_ascii(char *utf8_char);
    int utf8_char_len(unsigned char c);
    
    // Member variables
    lv_obj_t *slots[GRID_ROWS][GRID_COLS];
    std::vector<GridCharacterSlot> animation_queue;
    int running_animations;

    // SFX callback functions
    void (*start_card_flip_sound_task)();
    void (*stop_card_flip_sound_task)();
    
    // Static character sets
    static const char *card_chars;
    static const char *emoji_chars[];
    static const int total_emoji_cards;
    static const int total_cards;
};

extern GridBoard* get_grid_board_instance();
