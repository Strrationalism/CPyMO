#include <cpymo_prelude.h>
#include <stdint.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
static uint64_t millis(void)
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    return st.wMilliseconds + st.wSecond * 1000 + st.wMinute * 1000 * 60 + st.wHour * 1000 * 60 * 60;
}

void get_winsize(size_t *w, size_t *h) 
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    *w = (size_t)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    *h = (size_t)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);

    if (*w <= 5 || *w >= 8000) *w = 80;
    if (*h <= 5 || *h >= 8000) *h = 25;
}

#else

#include <time.h>

static uint64_t millis(void)
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return ((uint64_t) now.tv_sec) * 1000 + ((uint64_t) now.tv_nsec) / 1000000;
}

#include <sys/ioctl.h>
#include <unistd.h>
void get_winsize(size_t *w, size_t *h)
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *w = (size_t)ws.ws_col;
    *h = (size_t)ws.ws_row;

    if (*w <= 1) *w = 80;
    if (*h <= 1) *h = 25;
}

#endif

float get_delta_time(void) 
{
    static uint64_t prev = 0;
    uint64_t now = millis();
    float delta = (now - prev) / 1000.0f;
    prev = now;
    return delta;
}


/********** Unsupported ***************/
// Masktrans
#include <cpymo_backend_masktrans.h>

error_t cpymo_backend_masktrans_create(
    cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h)
{ return CPYMO_ERR_UNSUPPORTED; }

void cpymo_backend_masktrans_free(cpymo_backend_masktrans m) {}

void cpymo_backend_masktrans_draw(cpymo_backend_masktrans m, float t, bool is_fade_in) {}


// Text
#include <cpymo_backend_text.h>
error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_str utf8_string, 
    float single_character_size_in_logical_screen)
{
    *out = (cpymo_backend_text)1;
    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_free(cpymo_backend_text t) {}

void cpymo_backend_text_draw(
    cpymo_backend_text t,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type) {}

float cpymo_backend_text_width(
    cpymo_str t,
    float single_character_size_in_logical_screen) 
{ return t.len * single_character_size_in_logical_screen; }


