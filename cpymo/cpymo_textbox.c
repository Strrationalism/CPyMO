#include "cpymo_textbox.h"
#include <assert.h>
#include <stdlib.h>

error_t cpymo_textbox_init(cpymo_textbox *o, float x, float y, float width, float height, float character_size, cpymo_color col, cpymo_parser_stream_span text)
{
    o->x = x;
    o->y = y;
    o->width = width;
    o->character_size = character_size;
    o->max_lines = height / o->character_size;
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

    return CPYMO_ERR_SUCC;
}

void cpymo_textbox_free(cpymo_textbox *tb)
{
    cpymo_textbox_clear_page(tb);
    free(tb->lines);
}

void cpymo_textbox_draw(const cpymo_textbox *tb, enum cpymo_backend_image_draw_type drawtype)
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
}

static bool cpymo_textbox_line_full(cpymo_textbox * tb)
{
    cpymo_parser_stream_span span;
    span.begin = tb->text_curline_and_remaining.begin;
    span.len = tb->text_curline_size;
    return cpymo_backend_text_width(span, tb->character_size) + tb->character_size > tb->width;
}

void cpymo_textbox_refresh_curline(cpymo_textbox *tb)
{
    if (tb->active_line >= tb->max_lines) return;

    cpymo_parser_stream_span span;
    span.begin = tb->text_curline_and_remaining.begin;
    span.len = tb->text_curline_size;

    if (span.len >= 2) {
        if (span.begin[span.len - 2] == '\\' && span.begin[span.len - 1] == 'n')
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
    assert(!cpymo_textbox_page_full(tb));
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
        if (cpymo_parser_stream_span_equals_str(next_char, "n")) {
            tb->text_curline_size += next_char.len;
            cpymo_textbox_show_next_line(tb);
        }
    }
}

void cpymo_textbox_clear_page(cpymo_textbox *tb)
{
    tb->active_line = 0;
    tb->active_line_current_width = 0;
    for (size_t i = 0; i < tb->max_lines; i++) {
        if (tb->lines[i] != NULL) 
            cpymo_backend_text_free(tb->lines[i]);
        tb->lines[i] = NULL;
    }
}

void cpymo_textbox_finalize(cpymo_textbox *tb)
{
    while (!cpymo_textbox_all_finished(tb) && !cpymo_textbox_page_full(tb)) {
        cpymo_textbox_show_next_char(tb);
    }

    cpymo_textbox_refresh_curline(tb);
}

