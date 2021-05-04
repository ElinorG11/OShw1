#include <iostream>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl << flush;
    SmallShell &smash = SmallShell::getInstance();

    if (smash.getFgJobPID() != -1) {
        int fgJobId = smash.getFgJobID();
        
        // External FG job which is stopped for the first time
        if(fgJobId == -1){
			// add new job
			Command *cmd = smash.CreateCommand(smash.getCmdLineSM()); 
			smash.getJobList()->addJob(cmd, true, smash.getFgJobPID());
			delete cmd;
			
			// send signal
			if(kill(smash.getFgJobPID(), SIGSTOP) == -1){
                perror("smash error: kill failed");
                return;
            }
            
            cout << "smash: process " << smash.getFgJobPID() << " was stopped" << endl << flush;
		}
        JobsList::JobEntry *jobEntry = smash.getJobList()->getJobById(fgJobId);
        if(jobEntry != nullptr){
            int pid = jobEntry->getJobPid();

            if(kill(pid, SIGSTOP) == -1){
                perror("smash error: kill failed");
                return;
            }

            if(!jobEntry->getBackground() && !jobEntry->isJobStopped()){
				jobEntry->resetTimer();
			}

            smash.getJobList()->getJobByPID(pid)->setStopped(true);
            smash.setFgJobID(-1);
            smash.setFgJobPID(-1);
            cout << "smash: process " << pid << " was stopped" << endl << flush;
        }
    }
    return;
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl << flush;
    SmallShell &smash = SmallShell::getInstance();
    int pid = smash.getFgJobPID();
    if (pid != -1) {
        if(kill(pid, SIGKILL) < 0) {
            perror("smash error: kill failed");
            return;
        }
        JobsList::JobEntry *jobEntry = smash.getJobList()->getJobById(smash.getFgJobID());
		if(jobEntry != nullptr){
			jobEntry->setStopped(true);
			smash.getJobList()->removeJobByPID(pid);
		}
        smash.setFgJobPID(-1);
        smash.setFgJobID(-1);
        cout << "smash: process " << pid << " was killed" << endl << flush;
    }
    return;
}

void alarmHandler(int sig_num) {
    cout << "smash: got an alarm" << endl;
    SmallShell &smash = SmallShell::getInstance();
    std::list<TimeOutList::TimeOutEntry*> *timeout_list = smash.getTimeOutList()->time_out_entry_list;
    if(timeout_list->empty()) return;
    int length = timeout_list->size();
    int counter = 0;
    int res;
    std::list<TimeOutList::TimeOutEntry*>::iterator timeout_it = timeout_list->begin();
    while (counter < length){
        counter++;
        if(difftime((*timeout_it)->kill_time,time(nullptr)) >= 0){
            int pid = (*timeout_it)->pid;
            res = waitpid(pid, nullptr, WNOHANG);
            if(res == 0){ // child didnt finish yet
                if(kill(pid, sig_num) < 0) {
                    perror("smash error: kill failed");
                    return;
                }
                cout << "smash: " << (*timeout_it)->cmd_line << " timed out!" << endl;
            }
            smash.getTimeOutList()->removeTimeOutEntry(pid);
            smash.getJobList()->removeJobByPID(pid);
        }
        timeout_it++;
    }
    return;
}
