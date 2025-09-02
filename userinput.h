#ifndef USERINPUT_H
#define USERINPUT_H

#include <list>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "commandline.h"
#include "utils.h"

class Shell;

class UserInput
{
public:
    UserInput(Shell &shell, CommandLine &commandLine);
    std::string processInput();
    void resetArrows(); 

private:
    Shell &shell;
    CommandLine &commandLine;
    bool arrow_up, arrow_down; 
    int key_code_history[2];
    std::string line;

    void handleCtrlC();

};

#endif 
