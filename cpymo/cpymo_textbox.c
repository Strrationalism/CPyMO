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
    cpymo_str text,
    cpymo_backlog *backlog)
{
    o->max_lines = (size_t)(height / character_size);
    if (o->max_lines < 1) o->max_lines = 1;

    o->chars_pool_max_size = text.len;
    o->chars_pool_size = 0;

    o->backlog_buf_size = 0;
    o->backlog_buf_max_size = 0;
    o->backlog_buf = NULL;

    if (backlog) {
        o->backlog_buf_max_size = text.len + o->max_lines + 1;
        o->backlog_buf = (char *)malloc(o->backlog_buf_max_size);
        if (o->backlog_buf == NULL) return CPYMO_ERR_OUT_OF_MEM;
    }

    uint8_t *mem = (uint8_t *)malloc(
        o->max_lines * sizeof(cpymo_textbox_line) +
        o->chars_pool_max_size * (sizeof(cpymo_backend_text) + sizeof(float)));
    if (mem == NULL) {
        if (o->backlog_buf) free(o->backlog_buf);
        return CPYMO_ERR_OUT_OF_MEM;
    }

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
    o->backlog = backlog;

    o->lines[0].begin_pool_index = 0;
    o->lines[0].pool_slice_size = 0;
    
    for (size_t i = 0; i < o->max_lines; ++i) {
        o->lines[i].y = o->y + o->char_size * (i + 1);
    }

    o->timer = 0;
    o->draw_cursor = false;

    #ifdef ENABLE_TEXT_EXTRACT_ANDROID_ACCESSIBILITY
    cpymo_android_play_sound(SOUND_ENTER);
    #endif

    return CPYMO_ERR_SUCC;
}

static void cpymo_textbox_clear_chars_pool_and_lines(cpymo_textbox *tb)
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
    if (tb->backlog_buf) free(tb->backlog_buf);
    tb->backlog_buf = NULL;
    tb->lines = NULL;
    tb->chars_pool = NULL;
    tb->chars_x_pool = NULL;
}

void cpymo_textbox_draw(
    const struct cpymo_engine *e,
    const cpymo_textbox *tb, 
    enum cpymo_backend_image_draw_type drawtype)
{
    if (tb->lines == NULL || tb->chars_pool == NULL || tb->chars_x_pool == NULL) 
        return;

    if (tb->draw_cursor && e->say.msg_cursor) {
        float x = tb->w + tb->x - tb->char_size;
        float y = tb->lines[tb->max_lines - 1].y - tb->char_size;
        float wh = tb->char_size;

        float draw_w = e->say.msg_cursor_w;
        float draw_h = e->say.msg_cursor_h;

        #ifndef DISABLE_IMAGE_SCALING
        if (draw_w > wh || draw_h > wh) {
            float scale = (draw_w > draw_h ? draw_w : draw_h) / wh;
            draw_w /= scale;
            draw_h /= scale;
        }
        #endif
        
        float x1 = wh / 2 - draw_w / 2;
        float y1 = wh / 2 - draw_h / 2;

        cpymo_backend_image_draw(
            x + x1, y + y1, 
            draw_w, draw_h,
            e->say.msg_cursor,
            0, 0, e->say.msg_cursor_w, e->say.msg_cursor_h,
            1.0f, drawtype);
    }

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
    return CPYMO_ERR_SUCC;
}

static bool cpymo_textbox_nextline(cpymo_textbox *tb)
{
    if (tb->active_line >= tb->max_lines - 1) return false;
    const cpymo_textbox_line *last_line = tb->lines + tb->active_line;
    tb->active_line++;
    tb->lines[tb->active_line].begin_pool_index = 
        last_line->begin_pool_index + last_line->pool_slice_size;
    tb->lines[tb->active_line].pool_slice_size = 0;
    tb->typing_x = tb->x;

    if (tb->backlog_buf) {
        cpymo_str_copy(
            tb->backlog_buf + tb->backlog_buf_size, 
            tb->backlog_buf_max_size - tb->backlog_buf_size, 
            cpymo_str_pure("\n"));
        tb->backlog_buf_size++;
        if (tb->backlog_buf_size > tb->backlog_buf_max_size)
            tb->backlog_buf_size = tb->backlog_buf_max_size;
    }

    return true;
}

static char *cpymo_textbox_get_backlog_text(cpymo_textbox *tb)
{
    if (tb->backlog_buf == NULL) return NULL;
    assert(tb->backlog_buf_size <= tb->backlog_buf_max_size);
    size_t set_to_zero_offset = tb->backlog_buf_size;
    tb->backlog_buf_size = 0;
    if (tb->backlog_buf_max_size == set_to_zero_offset)
        set_to_zero_offset--;
        
    if (set_to_zero_offset == 0) return NULL;
    tb->backlog_buf[set_to_zero_offset] = '\0';
    
    char *next_backlog_buf = (char *)malloc(tb->backlog_buf_max_size);
    if (next_backlog_buf == NULL) return NULL;

    char *ret_backlog_text = tb->backlog_buf;
    tb->backlog_buf = next_backlog_buf;
    return ret_backlog_text;
}

static error_t cpymo_textbox_add_char(cpymo_textbox *tb)
{
    assert(tb->lines);
    assert(tb->chars_pool);
    assert(tb->chars_x_pool);
    if (tb->remain_text.len == 0) goto TEXT_FADEIN_FINISHED;

    #define EAT_NEWLINE(NEW_LINE_HEADER) \
        if (cpymo_str_starts_with_str(tb->remain_text, NEW_LINE_HEADER)) { \
            tb->remain_text.begin += sizeof(NEW_LINE_HEADER) - 1; \
            tb->remain_text.len -= sizeof(NEW_LINE_HEADER) - 1; \
            if (cpymo_textbox_nextline(tb)) return CPYMO_ERR_SUCC; \
            else goto TEXT_FADEIN_FINISHED; \
        }

    EAT_NEWLINE("\n");
    EAT_NEWLINE("\\n");
    EAT_NEWLINE("\\r");

    cpymo_str remain_text = tb->remain_text;
    cpymo_str ch = cpymo_str_utf8_try_head(&remain_text);

    float ch_w = cpymo_backend_text_width(ch, tb->char_size);

    float tbw = tb->w;
    if (tb->active_line == tb->max_lines - 1)
        tbw -= tb->char_size;
    
    if (tb->x + tbw - tb->typing_x < ch_w) {
        if (!cpymo_textbox_nextline(tb)) 
            goto TEXT_FADEIN_FINISHED;

        return cpymo_textbox_add_char(tb);            
    }

    if (tb->backlog_buf) {
        cpymo_str_copy(
            tb->backlog_buf + tb->backlog_buf_size, 
            tb->backlog_buf_max_size - tb->backlog_buf_size, 
            ch);
        tb->backlog_buf_size += ch.len;
        if (tb->backlog_buf_size > tb->backlog_buf_max_size)
            tb->backlog_buf_size = tb->backlog_buf_max_size;
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
    return CPYMO_ERR_SUCC;

TEXT_FADEIN_FINISHED:
    {
        char *backlog_text = cpymo_textbox_get_backlog_text(tb);
        if (backlog_text) {
            assert(tb->backlog);
            cpymo_backlog_record_write_text(
                tb->backlog, backlog_text, tb->char_size);
        }
    }
    return CPYMO_ERR_NO_MORE_CONTENT;
}

void cpymo_textbox_finalize(cpymo_textbox *tb)
{
    while (cpymo_textbox_add_char(tb) == CPYMO_ERR_SUCC);
    tb->timer = 0;
    tb->draw_cursor = true;
}

bool cpymo_textbox_wait_text_fadein(cpymo_engine *e, float dt, cpymo_textbox *which_textbox)
{
#ifdef LOW_FRAME_RATE
    cpymo_textbox_finalize(which_textbox);
    cpymo_engine_request_redraw(e);
    which_textbox->draw_cursor = true;
    return true;
#else 
    if (cpymo_engine_skipping(e)) {
        cpymo_textbox_finalize(which_textbox);
        cpymo_engine_request_redraw(e);
        return true;
    }

    which_textbox->timer += dt;
    float speed = 0.05f;
    switch (e->gameconfig.textspeed) {
    case 0: speed = 0.1f; break;
    case 1: speed = 0.075f; break;
    case 2: speed = 0.05f; break;
    case 3: speed = 0.025f; break;
    case 4: speed = 0.0125f; break;
    default: speed = 0.0f; break;
    };

    error_t err = CPYMO_ERR_SUCC;
    while (which_textbox->timer >= speed) {
        which_textbox->timer -= speed;
        err = cpymo_textbox_add_char(which_textbox);
        if (err != CPYMO_ERR_SUCC) break;
        cpymo_engine_request_redraw(e);
    }

    if (cpymo_input_foward_key_just_released(e)) {
        cpymo_engine_request_redraw(e);
        cpymo_textbox_finalize(which_textbox);
    }

    if (err == CPYMO_ERR_NO_MORE_CONTENT) {
        cpymo_engine_request_redraw(e);
        which_textbox->timer = 0;
        which_textbox->draw_cursor = true;
        return true;
    }

    return false;
#endif
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

#ifndef LOW_FRAME_RATE
    tb->timer += dt;
    while (tb->timer >= 0.5f) {
        tb->timer -= 0.5f;
        tb->draw_cursor = !tb->draw_cursor;
        cpymo_engine_request_redraw(e);
    }
#endif

    if (go) {
        tb->timer = 0;
        tb->draw_cursor = false;
    }

    return go;
}


