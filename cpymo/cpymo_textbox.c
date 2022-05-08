#include "cpymo_textbox.h"
#include "cpymo_engine.h"
#include <assert.h>
#include <stdlib.h>

error_t cpymo_textbox_init(cpymo_textbox *o, float x, float y, float width, float height, float character_size, cpymo_color col, cpymo_parser_stream_span text)
{
    o->x = x;
    o->y = y;
    o->width = width;
    o->character_size = character_size;
    o->max_lines = (size_t)(height / o->character_size);
    if (o->max_lines < 1) o->max_lines = 1;
    
    o->lines = (cpymo_backend_text *)malloc(o->max_lines * sizeof(cpymo_backend_text));

    if (o->lines == NULL) return CPYMO_ERR_OUT_OF_MEM;

    for (size_t i = 0; i < o->max_lines; ++i)
        o->lines[i] = NULL;

    o->active_line = 0;
    o->text_curline_and_remaining = text;
    o->color = col;
    o->active_line_current_width = 0;
    o->text_curline_size = 0;

    o->timer = 0;
    o->msg_cursor_visible = false;

    return CPYMO_ERR_SUCC;
}

void cpymo_textbox_free(cpymo_textbox *tb, cpymo_backlog *write_to_backlog)
{
    if (write_to_backlog) {
        cpymo_backlog_record_write_text(write_to_backlog, &tb->lines, tb->max_lines);
        assert(tb->lines == NULL);
    }
    else {
        cpymo_textbox_clear_page(tb, NULL);
        if (tb->lines) {
            free(tb->lines); 
            tb->lines = NULL;
        }
    }
}

void cpymo_textbox_draw(
    const struct cpymo_engine *e,
    const cpymo_textbox *tb, 
    enum cpymo_backend_image_draw_type drawtype)
{
    for (size_t i = 0; i < tb->max_lines; ++i) {
        cpymo_backend_text line = tb->lines[i];
        if (line) {
            cpymo_backend_text_draw(
                line, 
                tb->x,
                tb->y + tb->character_size * (1 + i),
                tb->color,
                1.0f,
                drawtype);
        }
    }

    if (tb->msg_cursor_visible && e->say.msg_cursor) {
        float ratio = 
            tb->character_size
            / (float)(e->say.msg_cursor_w > e->say.msg_cursor_h ? e->say.msg_cursor_w : e->say.msg_cursor_h);

        if (ratio > 1) ratio = 1;

        ratio *= 0.75f;

        float cursor_w = ratio * (float)e->say.msg_cursor_w;
        float cursor_h = ratio * (float)e->say.msg_cursor_h;

        cpymo_backend_image_draw(
            tb->x + tb->width - cursor_w,
            tb->y + (tb->max_lines) * tb->character_size - cursor_h,
            cursor_w, cursor_h,
            e->say.msg_cursor, 0, 0, e->say.msg_cursor_w, e->say.msg_cursor_h,
            1.0f, drawtype);
    }
}

static bool cpymo_textbox_line_full(cpymo_textbox * tb)
{
    cpymo_parser_stream_span span;
    span.begin = tb->text_curline_and_remaining.begin;
    span.len = tb->text_curline_size;
    float cur_w = cpymo_backend_text_width(span, tb->character_size) + tb->character_size;

    if (tb->active_line == tb->max_lines - 1)
        cur_w += tb->character_size * 0.9f;

    return cur_w > tb->width;
}

void cpymo_textbox_refresh_curline(cpymo_textbox *tb)
{
    if (tb->active_line >= tb->max_lines) return;

    cpymo_parser_stream_span span;
    span.begin = tb->text_curline_and_remaining.begin;
    span.len = tb->text_curline_size;

    if (span.len >= 2) {
        if (span.begin[span.len - 2] == '\\' 
            && (span.begin[span.len - 1] == 'n' || span.begin[span.len - 1] == 'r'))
            span.len -= 2;
    }

    float width;
    cpymo_backend_text refreshed_line;
    error_t err = cpymo_backend_text_create(
        &refreshed_line, &width, span, tb->character_size);
    if (err != CPYMO_ERR_SUCC) return;


    cpymo_backend_text *line = &tb->lines[tb->active_line];
    if (*line) cpymo_backend_text_free(*line);
    *line = refreshed_line;

    tb->active_line_current_width = width;
}

static void cpymo_textbox_show_next_line(cpymo_textbox *tb)
{
    assert(tb->text_curline_size <= tb->text_curline_and_remaining.len);

    cpymo_textbox_refresh_curline(tb);

    tb->active_line++;
    tb->text_curline_and_remaining.begin += tb->text_curline_size;
    tb->text_curline_and_remaining.len -= tb->text_curline_size;
    tb->text_curline_size = 0;
}

void cpymo_textbox_show_next_char(cpymo_textbox *tb)
{
    assert(!cpymo_textbox_all_finished(tb));
    assert(tb->text_curline_size <= tb->text_curline_and_remaining.len);

    cpymo_parser_stream_span span = tb->text_curline_and_remaining;
    span.begin += tb->text_curline_size;
    span.len -= tb->text_curline_size;
    cpymo_parser_stream_span next_char = 
        cpymo_parser_stream_span_utf8_try_head(&span);

    tb->text_curline_size += next_char.len;

    if (cpymo_parser_stream_span_equals_str(next_char, "\n") || cpymo_textbox_line_full(tb)) {
        cpymo_textbox_show_next_line(tb);
    }

    else if (cpymo_parser_stream_span_equals_str(next_char, "\\")) {
        next_char = cpymo_parser_stream_span_utf8_try_head(&span);
        if (cpymo_parser_stream_span_equals_str(next_char, "n") 
            || cpymo_parser_stream_span_equals_str(next_char, "r")) {
            tb->text_curline_size += next_char.len;
            cpymo_textbox_show_next_line(tb);
        }
    }
}

error_t cpymo_textbox_clear_page(cpymo_textbox *tb, cpymo_backlog *write_to_backlog)
{
    tb->msg_cursor_visible = false;
    tb->timer = 0;
    tb->active_line = 0;
    tb->active_line_current_width = 0;

    if (write_to_backlog) {
        cpymo_backlog_record_write_text(write_to_backlog, &tb->lines, tb->max_lines);
        assert(tb->lines == NULL);
        tb->lines = (cpymo_backend_text *)malloc(sizeof(cpymo_backend_text) * tb->max_lines);
        for (size_t i = 0; i < tb->max_lines; ++i)
            tb->lines[i] = NULL;
    }
    else {
        if (tb->lines) {
            for (size_t i = 0; i < tb->max_lines; i++) {
                if (tb->lines[i] != NULL)
                    cpymo_backend_text_free(tb->lines[i]);
                tb->lines[i] = NULL;
            }
        }
    }

    return CPYMO_ERR_SUCC;
}

void cpymo_textbox_finalize(cpymo_textbox *tb)
{
    while (!cpymo_textbox_all_finished(tb) && !cpymo_textbox_page_full(tb)) {
        cpymo_textbox_show_next_char(tb);
    }

    cpymo_textbox_refresh_curline(tb);
}

bool cpymo_textbox_wait_text_fadein(cpymo_engine *e, float dt, cpymo_textbox *tb)
{
    tb->timer += dt;

    if (cpymo_input_foward_key_just_released(e)) {
        cpymo_engine_request_redraw(e);
        cpymo_textbox_finalize(tb);
        tb->msg_cursor_visible = true;
        tb->timer = 0;
        return true;
    }

#ifndef LOW_FRAME_RATE
    float speed = 0.05f;
    switch (e->gameconfig.textspeed) {
    case 0: speed = 0.1f; break;
    case 1: speed = 0.075f; break;
    case 2: speed = 0.05f; break;
    case 3: speed = 0.025f; break;
    case 4: speed = 0.0125f; break;
    default: speed = 0.0f; break;
    };
#else
    const float speed = 0;
#endif

    while (tb->timer >= speed) {
        if (cpymo_textbox_page_full(tb) || cpymo_textbox_all_finished(tb)) {
            tb->msg_cursor_visible = true;
            tb->timer = speed;
            return true;
        }

        tb->timer -= speed;
        cpymo_engine_request_redraw(e);
        cpymo_textbox_show_next_char(tb);
        cpymo_textbox_refresh_curline(tb);
    }

    if (cpymo_textbox_page_full(tb) || cpymo_textbox_all_finished(tb)) {
        tb->msg_cursor_visible = true;
        tb->timer = 0;
        return true;
    }
    else return false;
}

bool cpymo_textbox_wait_text_reading(cpymo_engine *e, float dt, cpymo_textbox *tb)
{
    tb->timer += dt;

    bool go =
        CPYMO_INPUT_JUST_RELEASED(e, ok) 
        || e->input.skip 
        || CPYMO_INPUT_JUST_RELEASED(e, mouse_button)
        || CPYMO_INPUT_JUST_RELEASED(e, down)
        || e->input.mouse_wheel_delta < 0;

#ifndef LOW_FRAME_RATE
    if (tb->timer >= 0.5f) {
        tb->timer -= 0.5f;
        tb->msg_cursor_visible = !tb->msg_cursor_visible;
        cpymo_engine_request_redraw(e);
    }
#endif

    return go;
}

