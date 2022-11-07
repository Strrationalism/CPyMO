#include "cpymo_prelude.h"
#include "cpymo_textbox.h"
#include "cpymo_engine.h"
#include <assert.h>
#include <stdlib.h>

#ifdef ENABLE_TEXT_EXTRACT_ANDROID_ACCESSIBILITY
#include <cpymo_android.h>
#endif

typedef struct cpymo_textbox_line {
    size_t begin_pool_index, pool_slice_size;
    float y;
} cpymo_textbox_line;

error_t cpymo_textbox_init(
    cpymo_textbox *o, 
    float x, float y, 
    float width, float height, 
    float character_size, 
    cpymo_color col, float alpha, 
    cpymo_str text)
{
    o->max_lines = height / character_size;
    if (o->max_lines < 1) o->max_lines = 1;

    o->chars_pool_max_size = text.len;
    o->chars_pool_size = 0;

    uint8_t *mem = (uint8_t *)malloc(
        o->max_lines * sizeof(cpymo_textbox_line) +
        o->chars_pool_max_size * (sizeof(cpymo_backend_text) + sizeof(float)));
    if (mem == NULL) return CPYMO_ERR_OUT_OF_MEM;

    o->chars_pool = (cpymo_backend_text *)mem;
    o->chars_x_pool = (float *)
        (mem + o->chars_pool_max_size * sizeof(cpymo_backend_text));

    o->lines = (cpymo_textbox_line *)
        (mem + o->chars_pool_max_size * (sizeof(cpymo_backend_text) + sizeof(float)));

    if (width < character_size * 1.5f)
        width = character_size * 1.5f;

    o->active_line = 0;
    o->x = x;
    o->y = y;
    o->w = width;
    o->h = height;
    o->char_size = character_size;
    o->alpha = alpha;
    o->typing_x = x;
    o->col = col;
    o->remain_text = text;
    o->text_showing.begin = o->remain_text.begin;
    o->text_showing.len = 0;

    o->lines[0].begin_pool_index = 0;
    o->lines[0].pool_slice_size = 0;
    
    for (size_t i = 0; i < o->max_lines; ++i) {
        o->lines[i].y = o->y + o->char_size * (i + 1);
    }

    return CPYMO_ERR_SUCC;
}

static inline void cpymo_textbox_clear_chars_pool_and_lines(cpymo_textbox *tb)
{
    for (size_t i = 0; i < tb->chars_pool_size; ++i)
        if (tb->chars_pool[i])
            cpymo_backend_text_free(tb->chars_pool[i]);
    tb->chars_pool_size = 0;

    tb->active_line = 0;
    if (tb->lines) {
        tb->lines[0].begin_pool_index = 0;
        tb->lines[0].pool_slice_size = 0;
    }
    tb->typing_x = tb->x;
}

void cpymo_textbox_free(cpymo_textbox *tb, cpymo_backlog *write_to_backlog)
{
    cpymo_textbox_clear_chars_pool_and_lines(tb);
    if (tb->chars_pool) free((void *)tb->chars_pool);
}

void cpymo_textbox_draw(
    const struct cpymo_engine *e,
    const cpymo_textbox *tb, 
    enum cpymo_backend_image_draw_type drawtype)
{
    if (tb->lines == NULL || tb->chars_pool == NULL || tb->chars_x_pool == NULL) 
        return;

    for (size_t line_id = 0; line_id <= tb->active_line; ++line_id) {
        const cpymo_textbox_line *line = tb->lines + line_id;
        for (size_t char_id = 0; char_id < line->pool_slice_size; ++char_id) {
            size_t char_index = char_id + line->begin_pool_index;
            assert(char_index < tb->chars_pool_size);
            float x_pos = tb->chars_x_pool[char_index];
            cpymo_backend_text ch = tb->chars_pool[char_index];
            cpymo_backend_text_draw(
                ch, x_pos, line->y, tb->col, tb->alpha, drawtype);
        }
    }
}

error_t cpymo_textbox_clear_page(cpymo_textbox *tb, cpymo_backlog *write_to_backlog)
{
    cpymo_textbox_clear_chars_pool_and_lines(tb);
    tb->text_showing.begin = tb->remain_text.begin;
    tb->text_showing.len = 0;
    return CPYMO_ERR_SUCC;
}

static inline bool cpymo_textbox_nextline(cpymo_textbox *tb)
{
    if (tb->active_line >= tb->max_lines - 1) return false;
    const cpymo_textbox_line *last_line = tb->lines + tb->active_line;
    tb->active_line++;
    tb->lines[tb->active_line].begin_pool_index = 
        last_line->begin_pool_index + last_line->pool_slice_size;
    tb->lines[tb->active_line].pool_slice_size = 0;
    tb->typing_x = tb->x;
    return true;
}

static error_t cpymo_textbox_add_char(cpymo_textbox *tb)
{
    assert(tb->lines);
    assert(tb->chars_pool);
    assert(tb->chars_x_pool);
    if (tb->remain_text.len == 0) return CPYMO_ERR_NO_MORE_CONTENT;

    if (cpymo_str_starts_with_str(tb->remain_text, "\n") || 
        cpymo_str_starts_with_str(tb->remain_text, "\\n") ||
        cpymo_str_starts_with_str(tb->remain_text, "\\r")) {
        return cpymo_textbox_nextline(tb) ?
                CPYMO_ERR_SUCC :
                CPYMO_ERR_NO_MORE_CONTENT;
    }

    cpymo_str remain_text = tb->remain_text;
    cpymo_str ch = cpymo_str_utf8_try_head(&remain_text);

    float ch_w = cpymo_backend_text_width(ch, tb->char_size);
    if (tb->x + tb->w - tb->typing_x < ch_w) {
        if (!cpymo_textbox_nextline(tb)) 
            return CPYMO_ERR_NO_MORE_CONTENT;

        return cpymo_textbox_add_char(tb);            
    }

    assert(tb->chars_pool_size < tb->chars_pool_max_size);
    error_t err = cpymo_backend_text_create(
        tb->chars_pool + tb->chars_pool_size, &ch_w, ch, tb->char_size);
    CPYMO_THROW(err);

    tb->chars_x_pool[tb->chars_pool_size] = tb->typing_x;
    tb->chars_pool_size++;
    tb->lines[tb->active_line].pool_slice_size++;
    tb->typing_x += ch_w;
    tb->remain_text = remain_text;
    tb->text_showing.len += ch.len;
    return CPYMO_ERR_SUCC;
}

void cpymo_textbox_finalize(cpymo_textbox *tb)
{
    while (cpymo_textbox_add_char(tb) == CPYMO_ERR_SUCC);
}

bool cpymo_textbox_wait_text_fadein(cpymo_engine *e, float dt, cpymo_textbox *which_textbox)
{
    cpymo_textbox_finalize(which_textbox);
    cpymo_engine_request_redraw(e);
    return true;
}

bool cpymo_textbox_wait_text_reading(cpymo_engine *e, float dt, cpymo_textbox *tb)
{
    bool go =
        CPYMO_INPUT_JUST_RELEASED(e, ok) 
        || cpymo_engine_skipping(e)
        || CPYMO_INPUT_JUST_PRESSED(e, skip) 
        || CPYMO_INPUT_JUST_RELEASED(e, mouse_button)
        || CPYMO_INPUT_JUST_RELEASED(e, down)
        || e->input.mouse_wheel_delta < 0;

    return go;
}


