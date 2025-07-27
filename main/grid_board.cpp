#include "grid_board.hpp"
#include "esp_log.h"
#include "esp_random.h"
#include <algorithm>
#include <random>
#include <cstring>

static const char *TAG = "LVGL";

// Static character sets
const char *GridBoard::card_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.,:;!?@#$%&*()-+=/\\\"'<>[]{}|_^~ °±•…×÷−≠≤≥€£¥™®©";
const char *GridBoard::emoji_chars[] = {
    "✅", "✔", "✖", "❌", "❤", "️", "📀", "📁", "📂", "📃", "📄", "📅", "📆", "📇", "📈", "📉", "📊", "📋", "📌", "📍", "📎", "📏", "📐", "📑", "📒", "📓", "📔", "📕", "📖", "📗", "📘", "📙", "📚", "📛", "📜", "📝", "📞", "📟", "📠", "📡", "📢", "📣", "📤", "📥", "📦", "📧", "📨", "📩", "📪", "📫", "📬", "📭", "📮", "📯", "📰", "📱", "📲", "📳", "📴", "📵", "📶", "📷", "📸", "📹", "📺", "📻", "📼", "📽", "📿", "😀", "😁", "😂", "😃", "😄", "😅", "😆", "😇", "😈", "😉", "😊", "😋", "😌", "😍", "😎", "😏", "😐", "😑", "😒", "😓", "😔", "😕", "😖", "😗", "😘", "😙", "😚", "😛", "😜", "😝", "😞", "😟", "😠", "😡", "😢", "😣", "😤", "😥", "😦", "😧", "😨", "😩", "😪", "😫", "😬", "😭", "😮", "😯", "😰", "😱", "😲", "😳", "😴", "😵", "😶", "😷", "😸", "😹", "😺", "😻", "😼", "😽", "😾", "😿", "🙀", "🙁", "🙂", "🙃", "🙄", "🙅", "🙆", "🙇", "🙈", "🙉", "🙊", "🙋", "🙌", "🙍", "🙎", "🙏", "🚀", "🚁", "🚂", "🚃", "🚄", "🚅", "🚆", "🚇", "🚈", "🚉", "🚊", "🚋", "🚌", "🚍", "🚎", "🚏", "🚐", "🚑", "🚒", "🚓", "🚔", "🚕", "🚖", "🚗", "🚘", "🚙", "🚚", "🚛", "🚜", "🚝", "🚞", "🚟", "🚠", "🚡", "🚢", "🚣", "🚤", "🚥", "🚦", "🚧", "🚨", "🚩", "🚪", "🚫", "🚬", "🚭", "🚮", "🚯", "🚰", "🚱", "🚲", "🚳", "🚴", "🚵", "🚶", "🚷", "🚸", "🚹", "🚺", "🚻", "🚼", "🚽", "🚾", "🚿", "🛀", "🛁", "🛂", "🛃", "🛄", "🛅", "🛋", "🛌", "🛍", "🛎", "🛏", "🛐", "🛑", "🛒", "🛕", "🛖", "🛗", "🛜", "🛝", "🛞", "🛟", "🛠", "🛡", "🛢", "🛣", "🛤", "🛥", "🛩", "🛫", "🛬", "🛰", "🛳", "🛴", "🛵", "🛶", "🛷", "🛸", "🛹", "🛺", "🛻", "🛼"};

const int GridBoard::total_emoji_cards = sizeof(GridBoard::emoji_chars) / sizeof(GridBoard::emoji_chars[0]);
const int GridBoard::total_cards = strlen(GridBoard::card_chars);

static std::random_device rd;
static std::mt19937 g(rd());

static GridBoard *g_grid_instance = nullptr;

GridBoard *get_grid_board_instance()
{
    return g_grid_instance;
}

// Utility function for typographic quotes replacement
static void replace_typographic_quotes(char *str)
{
    if (!str)
        return;
    unsigned char *p = (unsigned char *)str;
    char *out = str;
    while (*p)
    {
        // UTF-8 for ' (U+2018): 0xE2 0x80 0x98
        if (p[0] == 0xE2 && p[1] == 0x80 && p[2] == 0x98)
        {
            *out++ = '\'';
            p += 3;
        }
        // UTF-8 for ' (U+2019): 0xE2 0x80 0x99
        else if (p[0] == 0xE2 && p[1] == 0x80 && p[2] == 0x99)
        {
            *out++ = '\'';
            p += 3;
        }
        else
        {
            *out++ = *p++;
        }
    }
    *out = 0;
}

// UTF-8 helper function
static uint32_t utf8_next(const char **txt)
{
    const unsigned char *ptr = (const unsigned char *)(*txt);
    uint32_t codepoint = 0;

    if (*ptr < 0x80)
    {
        codepoint = *ptr;
        (*txt)++;
    }
    else if ((*ptr & 0xE0) == 0xC0)
    {
        codepoint = ((*ptr & 0x1F) << 6) | (*(ptr + 1) & 0x3F);
        (*txt) += 2;
    }
    else if ((*ptr & 0xF0) == 0xE0)
    {
        codepoint = ((*ptr & 0x0F) << 12) | ((*(ptr + 1) & 0x3F) << 6) | (*(ptr + 2) & 0x3F);
        (*txt) += 3;
    }
    else if ((*ptr & 0xF8) == 0xF0)
    {
        codepoint = ((*ptr & 0x07) << 18) | ((*(ptr + 1) & 0x3F) << 12) |
                    ((*(ptr + 2) & 0x3F) << 6) | (*(ptr + 3) & 0x3F);
        (*txt) += 4;
    }
    else
    {
        codepoint = *ptr;
        (*txt)++;
    }
    return codepoint;
}

GridBoard::GridBoard() : running_animations(0), start_card_flip_sound_task(nullptr), stop_card_flip_sound_task(nullptr)
{
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            slots[row][col] = nullptr;
        }
    }
    g_grid_instance = this;
}

GridBoard::~GridBoard()
{
    if (g_grid_instance == this)
    {
        g_grid_instance = nullptr;
    }
}

void GridBoard::initialize(lv_obj_t *parent)
{
    create_grid(parent);
}

void GridBoard::set_sound_callback(void (*on_start)(), void (*on_end)())
{
    start_card_flip_sound_task = on_start;
    stop_card_flip_sound_task = on_end;
}

void GridBoard::create_grid(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x1A1A1A), 0);
    int total_width = GRID_COLS * GRID_SLOT_WIDTH + (GRID_COLS - 1) * GRID_GAP;
    int total_height = GRID_ROWS * GRID_SLOT_HEIGHT + (GRID_ROWS - 1) * GRID_GAP;
    int x_start = (GRID_SCREEN_WIDTH - total_width) / 2;
    int y_start = (GRID_SCREEN_HEIGHT - total_height) / 2;

    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            int x = x_start + col * (GRID_SLOT_WIDTH + GRID_GAP);
            int y = y_start + row * (GRID_SLOT_HEIGHT + GRID_GAP);
            lv_obj_t *slot = lv_obj_create(parent);
            lv_obj_set_size(slot, GRID_SLOT_WIDTH, GRID_SLOT_HEIGHT);
            lv_obj_set_pos(slot, x, y);
            lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_layout(slot, LV_LAYOUT_NONE);
            lv_obj_set_style_pad_all(slot, 0, 0);
            lv_obj_set_style_border_width(slot, 1, 0);
            lv_obj_set_style_border_color(slot, lv_color_hex(0x3A3A3A), 0);
            lv_obj_set_style_bg_color(slot, lv_color_hex(0x2A2A2A), 0);
            lv_obj_set_style_radius(slot, 0, 0);
            slots[row][col] = slot;
        }
    }
}

lv_obj_t *GridBoard::create_card(lv_obj_t *slot, const char *text, const lv_font_t *font)
{
    if (slot == nullptr)
    {
        ESP_LOGE(TAG, "Attempted to create card on NULL slot for '%s'", text);
        return nullptr;
    }

    lv_obj_t *card = lv_obj_create(slot);
    lv_obj_set_size(card, GRID_SLOT_WIDTH, GRID_SLOT_HEIGHT);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(card, lv_color_hex(0x121212), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, -GRID_SLOT_HEIGHT);

    lv_obj_t *label = lv_label_create(card);

    lv_label_set_text(label, text);

    // Set emoji color
    if (strcmp(text, "❤") == 0 || strcmp(text, "❤️") == 0)
    {
        lv_obj_set_style_text_color(label, lv_color_hex(0xFF4444), 0); // Red heart
    }
    else if (font == &NotoEmoji64)
    {
        // Random color for other emojis
        uint8_t r = esp_random() % 200 + 55;
        uint8_t g = esp_random() % 200 + 55;
        uint8_t b = esp_random() % 200 + 55;
        lv_obj_set_style_text_color(label, lv_color_make(r, g, b), 0);
    }
    else
    {
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
    }

    lv_obj_center(label);
    lv_obj_set_style_text_font(label, font, 0);

    return card;
}

bool GridBoard::is_emoji(const char *utf8_char)
{
    const char *tmp = utf8_char;
    uint32_t cp = utf8_next(&tmp);
    return (
        (cp >= 0x1F600 && cp <= 0x1F64F) ||
        (cp >= 0x1F680 && cp <= 0x1F6FF) ||
        (cp >= 0x1F4C0 && cp <= 0x1F4FF) ||
        cp == 0x2764 || cp == 0x2705 || cp == 0x2714 || cp == 0x274C || cp == 0x2716);
}

void GridBoard::utf8_to_upper_ascii(char *utf8_char)
{
    // Only handle single-byte ASCII lowercase letters
    if ((uint8_t)utf8_char[0] >= 'a' && (uint8_t)utf8_char[0] <= 'z')
    {
        utf8_char[0] = utf8_char[0] - 32;
    }
}

int GridBoard::utf8_char_len(unsigned char c)
{
    if ((c & 0x80) == 0x00)
        return 1; // ASCII
    if ((c & 0xE0) == 0xC0)
        return 2; // 2-byte UTF-8
    if ((c & 0xF0) == 0xE0)
        return 3; // 3-byte UTF-8
    if ((c & 0xF8) == 0xF0)
        return 4; // 4-byte UTF-8
    return 1;     // fallback
}

void GridBoard::animate_card_to_slot(lv_obj_t *card, const char *target)
{
    if (start_card_flip_sound_task)
    {
        start_card_flip_sound_task();
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, card);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_time(&a, 333);
    lv_coord_t start_y = -GRID_SLOT_HEIGHT * 2;
    lv_coord_t end_y = GRID_SLOT_HEIGHT * 1.2; // go beyond bottom

    lv_anim_set_values(&a, start_y, end_y);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&a, animation_ready_callback);

    lv_anim_start(&a);
}

void GridBoard::animation_ready_callback(lv_anim_t *a)
{
    lv_obj_t *card = (lv_obj_t *)a->var;
    lv_obj_set_y(card, 0); // reset to top before next retry

    GridBoard *instance = get_grid_board_instance();
    if (instance)
    {
        instance->on_card_dropped(card);
    }
}

void GridBoard::on_card_dropped(lv_obj_t *card)
{
    if (!card)
        return;

    lv_obj_t *slot = lv_obj_get_parent(card);
    GridCharacterSlot *slot_info = (GridCharacterSlot *)lv_obj_get_user_data(card);
    if (!slot_info || !slot)
        return;

    const char *target = slot_info->utf8_char;
    const char *txt = lv_label_get_text(lv_obj_get_child(card, 0));
    if (!target || !txt)
        return;

    const int MAX_RETRY_PER_CARD = 30;

    // 1. If match: show final static card, finish
    if (strcmp(txt, target) == 0)
    {
        running_animations--;
        lv_obj_t *final_card;
        if (is_emoji(target))
        {
            final_card = create_card(slot, target, &NotoEmoji64);
        }
        else
        {
            final_card = create_card(slot, target, &ShareTech140);
        }
        lv_obj_set_y(final_card, 0);
        delete slot_info;
        lv_obj_del(card);

        if (running_animations <= 0 && animation_queue.empty())
        {

            if (stop_card_flip_sound_task)
            {
                stop_card_flip_sound_task();
            }
        }
        start_animation_batch();

        return;
    }

    // 2. If retry count exceeded: force correct answer and finish
    if (slot_info->retry_index > MAX_RETRY_PER_CARD)
    {
        running_animations--;
        lv_obj_t *final_card;
        if (is_emoji(target))
        {
            final_card = create_card(slot, target, &NotoEmoji64);
        }
        else
        {
            final_card = create_card(slot, target, &ShareTech140);
        }
        lv_obj_set_y(final_card, 0);
        delete slot_info;
        lv_obj_del(card);

        if (running_animations <= 0 && animation_queue.empty())
        {

            if (stop_card_flip_sound_task)
            {
                stop_card_flip_sound_task();
            }
        }

        start_animation_batch();
        return;
    }

    // 3. Not matched, keep spinning with next candidate
    lv_obj_del(card);
    lv_obj_t *next_card;
    if (is_emoji(target))
    {
        // Use per-slot shuffled emoji order if present, otherwise random
        if (!slot_info->shuffled_emojis.empty())
        {
            int i = slot_info->retry_index % slot_info->shuffled_emojis.size();
            next_card = create_card(slot, slot_info->shuffled_emojis[i], &NotoEmoji64);
        }
        else
        {
            next_card = create_card(slot, emoji_chars[esp_random() % total_emoji_cards], &NotoEmoji64);
        }
    }
    else
    {
        // Use per-slot shuffled char order if present, otherwise random
        if (!slot_info->shuffled_chars.empty())
        {
            int i = slot_info->retry_index % slot_info->shuffled_chars.size();
            char buf[2] = {slot_info->shuffled_chars[i], '\0'};
            next_card = create_card(slot, buf, &ShareTech140);
        }
        else
        {
            int i = slot_info->retry_index % total_cards;
            char buf[2] = {card_chars[i], '\0'};
            next_card = create_card(slot, buf, &ShareTech140);
        }
    }
    slot_info->retry_index++;

    lv_obj_set_user_data(next_card, slot_info);
    animate_card_to_slot(next_card, target);
}

void GridBoard::timer_callback(lv_timer_t *t)
{
    lv_obj_t *card = (lv_obj_t *)lv_timer_get_user_data(t);
    GridCharacterSlot *info = (GridCharacterSlot *)lv_obj_get_user_data(card);

    GridBoard *instance = get_grid_board_instance();
    if (instance)
    {
        instance->animate_card_to_slot(card, info->utf8_char);
    }
    lv_timer_del(t);
}

void GridBoard::start_animation_batch()
{
    while (running_animations < MAX_PARALLEL_ANIMATIONS && !animation_queue.empty())
    {
        GridCharacterSlot slot_info = animation_queue.back();
        animation_queue.pop_back();
        running_animations++;

        GridCharacterSlot *info = new GridCharacterSlot(slot_info); // allocate per-slot

        lv_obj_t *slot = slots[info->row][info->col];
        if (!slot)
        {
            delete info;
            continue;
        }

        lv_obj_t *card;
        if (is_emoji(info->utf8_char))
        {
            card = create_card(slot, emoji_chars[esp_random() % total_emoji_cards], &NotoEmoji64);
        }
        else
        {
            int i = info->retry_index++ % total_cards;
            char buf[2] = {card_chars[i], '\0'};
            card = create_card(slot, buf, &ShareTech140);
        }

        lv_obj_set_user_data(card, info);

        int delay_ms = (esp_random() % 200) + 100; // between 100–300ms delay

        lv_timer_t *timer = lv_timer_create_basic();
        lv_timer_set_repeat_count(timer, 1); // one-shot
        lv_timer_set_period(timer, delay_ms);
        lv_timer_set_user_data(timer, card);
        lv_timer_set_cb(timer, timer_callback);
    }
}

bool GridBoard::is_animation_running() const
{
    return running_animations > 0 || !animation_queue.empty();
}

void GridBoard::clear_display()
{
    animation_queue.clear();
    running_animations = 0;

    // Clear grid
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            if (slots[row][col])
            {
                lv_obj_clean(slots[row][col]);
            }
        }
    }
}

void GridBoard::process_text_and_animate(const std::string &new_text)
{
    clear_display();

    if (new_text.empty())
    {
        ESP_LOGI(TAG, "New text is empty, clearing display.");
        return;
    }

    int i = 0;
    int char_index = 0;
    while (i < new_text.size() && char_index < GRID_ROWS * GRID_COLS)
    {
        int char_len = utf8_char_len((unsigned char)new_text[i]);
        if (char_len < 1 || i + char_len > new_text.size())
            break;

        char utf8[8] = {0};
        memcpy(utf8, &new_text[i], char_len);
        replace_typographic_quotes(utf8);
        i += char_len;

        int row = char_index / GRID_COLS;
        int col = char_index % GRID_COLS;
        char_index++;

        GridCharacterSlot slot{};
        memcpy(slot.utf8_char, utf8, sizeof(slot.utf8_char));
        slot.row = row;
        slot.col = col;
        slot.retry_index = esp_random() % total_cards;

        // Spaces are static/transparent
        if (strcmp(slot.utf8_char, " ") == 0)
        {
            if (slots[row][col])
            {
                lv_obj_t *space_card = lv_obj_create(slots[row][col]);
                lv_obj_set_size(space_card, GRID_SLOT_WIDTH, GRID_SLOT_HEIGHT);
                lv_obj_clear_flag(space_card, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_bg_opa(space_card, LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_width(space_card, 0, 0);
                lv_obj_set_y(space_card, 0);
            }
            continue;
        }

        // Prepare animation sequence for letters/emojis
        if (is_emoji(slot.utf8_char))
        {
            std::vector<const char *> emojis(emoji_chars, emoji_chars + total_emoji_cards);
            std::shuffle(emojis.begin(), emojis.end(), g);
            slot.shuffled_emojis = emojis;
        }
        else
        {
            std::vector<char> chars(card_chars, card_chars + total_cards);
            std::shuffle(chars.begin(), chars.end(), g);
            slot.shuffled_chars = chars;
        }

        animation_queue.push_back(slot);
    }

    // Fill any missing slots with static blank
    for (; char_index < GRID_ROWS * GRID_COLS; char_index++)
    {
        int row = char_index / GRID_COLS;
        int col = char_index % GRID_COLS;
        if (slots[row][col])
        {
            lv_obj_t *space_card = lv_obj_create(slots[row][col]);
            lv_obj_set_size(space_card, GRID_SLOT_WIDTH, GRID_SLOT_HEIGHT);
            lv_obj_clear_flag(space_card, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_opa(space_card, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(space_card, 0, 0);
            lv_obj_set_y(space_card, 0);
        }
    }

    std::shuffle(animation_queue.begin(), animation_queue.end(), g);
    start_animation_batch();
}
