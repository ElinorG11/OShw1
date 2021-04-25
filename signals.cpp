#include <iostream>
#include <signal.h>
#include <time.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
    cout << "smash: got ctrl-Z" << endl << flush;
    SmallShell &smash = SmallShell::getInstance();

    /* if we stop a resumed job (which was once on the job list) we need to use it's old job_id
     * which is stored in smash.getFgJobID(). case id == -1 it's a new job. case id != -1
     * addJob function will use it's old id. If the old Id is taken, it will set its index to max_index + 1
     */
    // smash.getJobList()->addJob(job->getCmdLine(), job->getPid(), smash.getFgJobID());
    // smash.setFgJobID(-1)

    if (smash.getFgJobPID() != -1) {
        int fgJobId = smash.getFgJobID();
        if(smash.getJobList()->getJobById(fgJobId) != nullptr){
            int pid = smash.getJobList()->getJobById(fgJobId)->getJobPid();
            if(kill(pid, SIGSTOP) == -1){
                perror("smash error: kill failed");
                exit(1); // do we need to exit(1)?
            }
            smash.getJobList()->removeJobById(fgJobId);
            smash.setFgJobID(-1);
            smash.setFgJobPID(-1);
            cout << "smash: process " << pid << " was stopped" << endl << flush;
        }

    }

}

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    cout << "smash: got ctrl-C" << endl << flush;
    SmallShell &smash = SmallShell::getInstance();
    int pid = smash.getFgJobPID();
    if (smash.getJobList()->getJobByPID(pid) != nullptr) {
        int fg_job_id = smash.getJobList()->getJobByPID(pid)->getJobId();
        if(kill(pid, sig_num) < 0) {
				perror("smash error: kill failed");
                exit(1);
		}
        smash.setFgJobPID(-1);
        cout << "smash: process " << pid << " was killed" << endl << flush;
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
    cout << "smash: got an alarm" << endl;
    SmallShell &smash = SmallShell::getInstance();
    std::list<TimeOutList::TimeOutEntry*> *timeout_list = smash.getTimeOutList()->time_out_entry_list;
    std::list<TimeOutList::TimeOutEntry*>::iterator timeout_it = timeout_list->begin();
    while (timeout_it != timeout_list->end()){
        if(difftime((*timeout_it)->kill_time,time(nullptr)) >= 0){
            int pid = (*timeout_it)->pid;
            if(kill(pid, sig_num) < 0) {
                perror("smash error: kill failed");
                exit(1);
            }
            cout << "smash: " << (*timeout_it)->cmd_line << " timed out!" << endl;
            smash.getTimeOutList()->removeTimeOutEntry(pid);
            smash.getJobList()->removeJobByPID(pid);
        }
    }
}