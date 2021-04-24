#include <iostream>
#include <signal.h>
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

}

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    cout << "smash: got ctrl-C" << endl << flush;
    SmallShell &smash = SmallShell::getInstance();
    int pid = smash.getFgJobPID();
    if (smash.getJobList()->getJobByPID(pid) != nullptr) {
        int fg_job_id = smash.getJobList()->getJobByPID(pid)->getJobId();
        if(kill(pid, sig_num) <0) {
				perror("smash error: kill failed");
		}
        smash.setFgJobPID(-1);
        cout << "smash: process " << pid << " was killed" << endl << flush;
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

