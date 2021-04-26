#include <iostream>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
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
                return;
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
    // TODO: Add your implementation
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
  // TODO: Add your implementation
    cout << "smash: got an alarm" << endl;
    SmallShell &smash = SmallShell::getInstance();
    std::list<TimeOutList::TimeOutEntry*> *timeout_list = smash.getTimeOutList()->time_out_entry_list;
    if(timeout_list->empty()) return;
    int length = timeout_list->size();
    int counter = 0;
    int res;
    //cout << "fuck list size " << timeout_list->size() << endl;
    std::list<TimeOutList::TimeOutEntry*>::iterator timeout_it = timeout_list->begin();
    //auto timeout_it = timeout_list->begin();
    //cout << "fuck2" << endl;
    while (counter < length){
        counter++;
        //cout << "fuck3" << endl;
        //cout << "fun 9000 " << *timeout_it << " end " << *timeout_list->end() << endl;
        //cout << "not the problem" << endl;
        //cout << (*timeout_it)->kill_time << " tim e " << time(nullptr) << endl;
        if(difftime((*timeout_it)->kill_time,time(nullptr)) >= 0){
            //cout << "fuck4" << endl;
            int pid = (*timeout_it)->pid;
            //cout << "sgnal pid " << pid << endl;
            res = waitpid(pid, nullptr, WNOHANG); // in case e.g. timeout 5 sleep 1& dont print cmd_line. check if process already finished
            //cout << " res " << res << endl;
            if(res == 0){ // child didnt finish yet
                //if(counter == length) break;
                if(kill(pid, sig_num) < 0) {
                    perror("smash error: kill failed");
                    return;
                }
                cout << "smash: " << (*timeout_it)->cmd_line << " timed out!" << endl;
                //cout << "fuck5" << endl;
                smash.getJobList()->removeJobByPID(pid);
                //cout << "fuck6" << endl;
            }
            smash.getTimeOutList()->removeTimeOutEntry(pid);
        }
        timeout_it++;
        //cout << "fuck7" << endl;
    }
    return;
}
