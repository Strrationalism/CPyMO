#include <cpymo_prelude.h>
#include <cpymo_error.h>
#include <cpymo_color.h>
#include <cpymo_utils.h>
#include <cpymo_backend_software.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stb_ds.h>

const static char ascii_table[70] = 
	" .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8B@$";

const static size_t ascii_table_length = 69;

static char *framebuffer_ascii = NULL;

static error_t cpymo_backend_ascii_write_string(const char *str)
{
    while (*str) {
        arrput(framebuffer_ascii, *str);
        str++;
    }

	return CPYMO_ERR_SUCC;
}

void cpymo_backend_ascii_clean(void)
{
    arrfree(framebuffer_ascii);
}

#ifdef _WIN32
#include <windows.h>
#endif

void cpymo_backend_ascii_submit_framebuffer(
    const cpymo_backend_software_image *framebuffer)
{
    arrsetlen(framebuffer_ascii, 0);
    
    char buf[32];

    for (size_t y = 0; y < framebuffer->h; ++y) {
        for (size_t x = 0; x < framebuffer->w; ++x) {
            cpymo_color col;
            col.r = *CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(framebuffer, x, y, r);
            col.g = *CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(framebuffer, x, y, g);
            col.b = *CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(framebuffer, x, y, b);

            float brightness =
                (float)col.r / 255.0f * 0.2126f +
                (float)col.g / 255.0f * 0.7152f +
                (float)col.b / 255.0f * 0.0722f;
            brightness = cpymo_utils_clampf(brightness, 0.0f, 1.0f);

            char ascii = 
                ascii_table[(size_t)(brightness * (ascii_table_length - 1))];

            sprintf(buf, "\033[38;2;%u;%u;%um%c\033[0m", 
                col.r, col.g, col.b, ascii);
            cpymo_backend_ascii_write_string(buf);
        }

        if (y != framebuffer->h - 1)
            cpymo_backend_ascii_write_string("\n");
    }

    arrput(framebuffer_ascii, '\0');

    #ifdef _WIN32
    WriteConsoleA(
        GetStdHandle(STD_OUTPUT_HANDLE), 
        framebuffer_ascii, 
        arrlenu(framebuffer_ascii) - 1, 
        NULL, 
        NULL);

    #else
    printf("%s", framebuffer_ascii);
    #endif

    printf("\033[%dA\033[%dD", 
        (int)framebuffer->w,
        (int)framebuffer->h);
}

