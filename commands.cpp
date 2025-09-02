#include "commands.h"
#include "shell.h"

using namespace std;

typedef void(Command::*func_ptr)(std::vector<std::string>);

Command::Command(Shell &shell) : shell(shell)
{

}

void Command::help(vector<string> args)
{
    cout_mutex.lock();
    cout << "\t- echo : affiche le texte qui suit" << endl;
    cout << "\t- cd : change le dossier courant" << endl;
    cout << "\t- ls : liste le contenu d'un répertoire" << endl;
    cout << "\t- shell : lance un shell" << endl;
    cout << "\t- pwd : affiche le répertoire de travail" << endl;
    cout << "\t- delay [sec] [command]: lance une commande à retardement" << endl;
    cout << "\t- help : affiche cette aide" << endl;
    cout << "\t- exit : quitte le shell courant" << endl;
    cout_mutex.unlock();
}

void Command::echo(vector<string> args)
{
    cout_mutex.lock();

    for(string str : args)
        cout << str << " ";

    cout << endl;
    cout_mutex.unlock();
}

void Command::cd(vector<string> args)
{
    string dirName = "";

    for(string str : args)
        dirName += str + " ";

    dirName = Utils::clearEscapedString(dirName);

    dirName.erase(dirName.end()-1);

    if(!args.size())
        chdir(getenv("HOME"));

    else if(chdir(dirName.c_str()) == -1)
    {
        cout_mutex.lock();
        switch(errno)
        {
            case ENOENT:
            case ENOTDIR:
                cout << "The folder does not exist." << endl;
                break;

            case EACCES:
                cout << "Access Denied." << endl;
                break;

            default:
                cout << "An error has occurred." << endl;
                break;
        }
        cout_mutex.unlock();
    }
}

void Command::pwd(vector<string> args)
{
    cout_mutex.lock();

    if(shell.getWorkingDirectory() != "")
        cout << shell.getWorkingDirectory() << endl;
    else
        cout << "pwd error" << endl;

    cout_mutex.unlock();
}

void Command::delay(vector<string> args)
{
    if(args.size() >= 2)
    {
        if(!isInteger(args.at(0)))
        {
            cout_mutex.lock();
            cout << "2nd arg has to be an integer" << endl;
            cout_mutex.unlock();
        }

        else
        {
            thread t(&Command::delay_t, this, args);
            t.detach();
        }
    }

    else
    {
        cout_mutex.lock();
        cout << "Usage : delay [seconds] [command]" << endl;
        cout_mutex.unlock();
    }
}

void Command::delay_t(vector<string> args)
{
    sleep(atoi(args.at(0).c_str()));
    vector<string> args_cmd;

    for(unsigned int i = 2; i < args.size(); i++)
        args_cmd.push_back(args.at(i));

    shell.execCommand(args.at(1), args_cmd);
}

bool Command::isInteger(const std::string & s)
{
   if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

   char * p ;
   strtol(s.c_str(), &p, 10) ;

   return (*p == 0) ;
}

void Command::exit_prog(vector<string> args)
{
    shell.rawMode(false);
    exit(0);
}

void Command::runshell(vector<string> args)
{
    shell.rawMode(false);
    system(getenv("SHELL"));
    shell.rawMode(true);
}

void Command::clear(vector<string> args)
{
    cout_mutex.lock();
    write(STDOUT_FILENO, "\033[2J\033[1;1H", sizeof("\033[2J\033[1;1H")); 
    cout_mutex.unlock();
}

void Command::exec(string filename, vector<string> args)
{
    bool backgroundProcess = false;

    if(args.size() > 0 && args[args.size()-1] == "&")
    {
        args.erase(args.end()-1);
        backgroundProcess = true;
    }

    char **params = new char* [args.size()+2];

    params[0] = const_cast<char*>(filename.c_str());

    for(int i = 0; i < (int)args.size(); i++)
        params[i+1] = const_cast<char*>(args[i].c_str());

    params[args.size()+1] = NULL;

    shell.rawMode(false);
    pid_t pid = fork();
    shell.addJob(pid);

    if(pid == 0)
    {
        execv(filename.c_str(), params);
        exit(1); 
    }

    if(!backgroundProcess)
    {
        int ret;
        waitpid(pid, &ret, 0);
        ret = WEXITSTATUS(ret);

        cout_mutex.lock();

        if(ret != 0)
            printf("The program returned an error code : %i", ret);

        printf("\n");

        cout_mutex.unlock();
    }

    shell.rawMode(true);
    delete [] params;
}

bool Command::pipeProcesses(string line)
{
    vector<string> progs;
    string current = "";

    int i = 0;
    while(i <  (int)line.size())
    {
        current = "";

        while(i < (int)line.size() && line[i] != '|')
            current += line[i++];

        while(current[0] == ' ') 
            current.erase(0, 1);

        while(current[current.size()-1] == ' ') 
            current.erase(current.size()-1, 1);

        progs.push_back(current);
        i++;
    }

    cout_mutex.lock();

    if(progs.size() > 2)
    {
        cout << "Sorry, multiple piping is not yet implemented." << endl;
        cout_mutex.unlock();
        return false;
    }

    FILE *from = popen(progs.at(0).c_str(), "r");

    if(from == NULL)
    {
        cout << "popen error!!" << endl;
        cout_mutex.unlock();
        return false;
    }

    FILE *to = popen(progs.at(1).c_str(), "w");

    if(to == NULL)
    {
        cout << "popen error!!" << endl;
        cout_mutex.unlock();
        return false;
    }

    cout_mutex.unlock();

    char buffer[256] = {0};
    int read_bytes = 0;

    while((read_bytes = fread(buffer, sizeof(char), sizeof(buffer), from)) > 0)
        fwrite(buffer, sizeof(char), read_bytes, to);

    pclose(from);
    pclose(to);

    return true;

}

bool Command::redirectOutputFile(string line)
{
    string progInput;
    string fileOutput;
    bool append = false;

    int i = 0;
    while(i < (int)line.size() && line[i] != '>')
        progInput += line[i++];

    if(progInput.size() == 0)
        return false;

    if(line[i+1] == '>')
    {
        append = true;
        i++;
    }

    while(line[++i] == ' '); 


    for(; i < (int)line.size(); i++)
        fileOutput += line[i];

    if(fileOutput.size() == 0)
        return false;

    FILE *prog = popen(progInput.c_str(), "r");
    cout_mutex.lock();

    if(prog == NULL)
    {
        vector<string> splited = Utils::parse(progInput);
        cout << "\"" + splited.at(0) << "\" : command not found." << endl;
        cout_mutex.unlock();
        return false;
    }

    if(Utils::fileExists(fileOutput.c_str()) && access(fileOutput.c_str(), W_OK))
    {
        cout << "Permission denied." << endl;
        cout_mutex.unlock();
        return false;
    }

    ofstream output(fileOutput, append ? ios::out | ios::app : ios::out);

    char buffer[256] = {0};
    int read_bytes = 0;

    while((read_bytes = fread(buffer, sizeof(char), sizeof(buffer), prog)) > 0)
    {
        output << string(buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    return true;
}

map<string, func_ptr>& Command::getAvailableCommands()
{
    return commands;
}
