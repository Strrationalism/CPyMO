#include "../../cpymo/cpymo_prelude.h"
#include "../include/cpymo_backend_input.h"
#include <string.h>
#include <ctype.h>

#ifdef _WIN32

#include <conio.h>

#else

#include <sys/ioctl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

static bool kbhit()
{
    struct termios term;
    tcgetattr(0, &term);

    struct termios term2 = term;
    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);

    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);

    tcsetattr(0, TCSANOW, &term);

    return byteswaiting > 0;
}

static char getch(void)
{
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

#endif

cpymo_input cpymo_input_snapshot() 
{ 
    cpymo_input ret;
    memset(&ret, 0, sizeof(ret));


    if (kbhit()) {
        int key = getch();

        switch (toupper(key)) {
        case 'W': ret.up = true; break;
        case 'S': ret.down = true; break;
        case 'A': ret.left = true; break;
        case 'D': ret.right = true; break;
        case ' ':
        case 'J': ret.ok = true; break;
        case 'K': ret.cancel = true; break;
        case 'L': ret.skip = true; break;
        };
    }

    return ret;
}



