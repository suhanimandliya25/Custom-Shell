#ifndef SHELL_H
#define SHELL_H

#include <string>

#include "commandline.h"
#include "userinput.h"
#include "commands.h"
#include "utils.h"

class Shell
{
typedef void(Command::*func_ptr)(std::vector<std::string>); 

public:
    Shell();
    ~Shell() = default;
    void run();
    void rawMode(bool enable);
    void print(std::string str) const;
    void print(const char *str) const;
    void print(char c) const;
    void print(int i) const;
    std::string getWorkingDirectory() const;
    std::string getHostname() const;
    std::vector<std::string> getPath() const;
    std::string getComputedLineInterface() const;
    int getLineInterfaceSize() const;
    struct winsize getTerminalSize();
    void addJob(pid_t pid);
    void removeJob(pid_t pid);
    Command &getCommand();
    bool execCommand(std::string cmd, const std::vector<std::string> &args);

private:
    struct termios old, new1;
    struct winsize window_size;
    Command command;
    CommandLine commandLine;
    UserInput user;

    std::vector<std::string> path = {
        "/usr/local/sbin",
        "/usr/local/bin",
        "/usr/sbin",
        "/usr/bin",
        "/sbin",
        "/bin"
    };

    std::string workingDirectory;
    std::string hostname;

    std::list<pid_t> bg_jobs;

};

#endif
