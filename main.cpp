#include "shell.h"

void preventParentAbort(int signal)
{
    // CTRL+C is handled by rawmode in userinput.cpp,
    // but this function only prevents the shell from exiting when
    // CTRL+C is pressed during a fork
}

int main(int argc, char **argv)
{
    signal(SIGINT, &preventParentAbort);

    Shell shell;
    shell.run();

    return 0;
}
