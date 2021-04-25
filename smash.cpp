#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = alarmHandler;

    if (sigaction(SIGALRM, &sa, nullptr)) {
        perror("smash error: failed to set alarm handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        int pid = smash.getFgJobPID();
        if(smash.getFgJobPID() == -1) {
            std::cout << smash.getPromptName() << "> ";
            std::string cmd_line;
            std::getline(std::cin, cmd_line);
            smash.executeCommand(cmd_line.c_str());
        } else {
            waitpid(pid, nullptr, WUNTRACED);
        }
    }
    return 0;
}
