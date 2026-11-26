# include "cli/keyboard.h"
# include <stdbool.h>
# include <stdlib.h>

# if defined(_WIN32) || defined(_WIN64)
#   include <conio.h>
# else
#   include <sys/select.h>
#   include <termios.h>
#   include <unistd.h>
static struct termios originalSettings;
static bool rawEnabled = false;
# endif

void restoreKeyboardInput(void)
{
# if !defined(_WIN32) && !defined(_WIN64)
    if (rawEnabled)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings);
        rawEnabled = false;
    }
# endif
}

void enableRawKeyboardInput(void)
{
# if !defined(_WIN32) && !defined(_WIN64)
    if (rawEnabled || tcgetattr(STDIN_FILENO, &originalSettings) != 0) return;

    struct termios raw = originalSettings;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);   // ISIG stays on: Ctrl+C still works
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    rawEnabled = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;

    static bool registered = false;
    if (!registered) { atexit(restoreKeyboardInput); registered = true; }  // also covers exit() paths
# endif
}

int tryReadKey(void)
{
# if defined(_WIN32) || defined(_WIN64)
    return _kbhit() ? _getch() : -1;
# else
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    struct timeval timeout = { 0, 0 };
    if (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) <= 0) return -1;

    unsigned char key;
    return read(STDIN_FILENO, &key, 1) == 1 ? key : -1;
# endif
}