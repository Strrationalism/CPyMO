#include <cpymo_backend_input.h>
#include <string.h>

#ifdef _WIN32

#include <conio.h>

#else

#include <sys/ioctl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

bool kbhit()
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

char getch(void)
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

        if (key == '\033') {
            key = getch();
            if (key == '[') {
                key = getch();
                switch (key) {
                    case 'A':
                        ret.up = true;
                        break;
                    case 'B':
                        ret.down = true;
                        break;
                    case 'C':
                        ret.right = true;
                        break;
                    case 'D':
                        ret.left = true;
                        break;
                    case 27:
                        ret.cancel = true;
                        break;
                };
            }
        }
        else if (key == '\n' || key == '\r' || key == ' ' || key == 'z' || key == 'Z') {
            ret.ok = true;
        }
        else if (key == 27 || key == 'x' || key == 'X') {
            ret.cancel = true;
        }
        else if (key == 'c' || key == 'C') {
            ret.skip = true;
        }
        else if (key == 'v' || key == 'V') {
            ret.hide_window = true;
        }
    }

    return ret;
}


