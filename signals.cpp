#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
    cout << "smash: got ctrl-Z" << endl << flush;
}

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    cout << "smash: got ctrl-C" << endl << flush;
    SmallShell &smash = SmallShell::getInstance();
    int pid = smash.getFgJobPID();
    if (smash.getJobList()->getJobByPID(pid) != nullptr) {
        int fg_job_id = smash.getJobList()->getJobByPID(pid)->getJobId();
        kill(pid, sig_num);
        smash.setFgJobPID(-1);
        cout << "smash: process " << pid << " was killed" << endl << flush;
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

