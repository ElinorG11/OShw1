#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <climits>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

const int BUFFER_SIZE = 1024;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

JobsList::JobEntry::JobEntry(string cmd_line, int jobId, int pid, bool isStopped, bool is_background) : cmd_line(cmd_line), job_id(jobId),
                            pid(pid), isStopped(isStopped), isBackgroundJob(is_background), time_added(time(nullptr)) {}

JobsList::JobsList()  {
    this->job_entry_list = new std::list<JobEntry*>({});
}

JobsList::~JobsList() {
    for(auto j : *job_entry_list){
        delete j;
    }
    delete job_entry_list;
}

bool sortJobEntries(JobsList::JobEntry *job1, JobsList::JobEntry *job2){
    return job1->getJobId() < job2->getJobId();
}

void JobsList::addJob(Command *cmd, bool isStopped, int pid, int job_id) {
    int jobId;
    bool is_background;

    if(job_entry_list->empty()) jobId = 1;

    // in case the job is added for the first time
    else if(job_id == -1) {
        JobsList::getLastJob(&jobId);
        jobId += 1;
    } else { // job is continued
        jobId = job_id;
    }

    is_background = _isBackgroundComamnd(cmd->getCmdLine().c_str());
    JobEntry *job_entry = new JobsList::JobEntry(cmd->getCmdLine(), jobId, pid, isStopped, is_background);
    job_entry_list->push_back(job_entry);
    this->job_entry_list->sort(sortJobEntries);
}

void JobsList::printJobsList() {
    JobsList::removeFinishedJobs();
    std::list<JobEntry*>::iterator job_iterator = job_entry_list->begin();
    while (job_iterator != job_entry_list->end()){
        std::string addend;
        if((*job_iterator)->isJobStopped()){
            addend = " secs (stopped)";
        }
        else {
            addend = " secs";
        }
        cout << "[" << (*job_iterator)->getJobId() << "] " << (*job_iterator)->getCmdLine() << " : " << (*job_iterator)->getJobPid() << " " <<
        difftime(static_cast<long int>(time(nullptr)) ,static_cast<long int>((*job_iterator)->getStartTime())) << addend << endl;
        job_iterator++;
    }
}

/* only quit uses this function - by the pdf no need to remove jobs from list */
void JobsList::killAllJobs() {
    JobsList::removeFinishedJobs();
    cout << "smash: sending SIGKILL signal to " << job_entry_list->size() << " jobs:" << endl;
    std::list<JobEntry*>::iterator job_iterator = job_entry_list->begin();
    while(job_iterator != job_entry_list->end()){
        cout << (*job_iterator)->getJobPid() << ": " << (*job_iterator)->getCmdLine() << endl;
        if(kill((*job_iterator)->getJobPid(),SIGKILL) == -1) {
            perror("smash error: kill failed");
            return;
        }
        job_iterator++;
    }
}

void JobsList::removeJobByPID(int pid){
    if(job_entry_list->empty()) return;
    JobEntry *temp = getJobByPID(pid);
    if(temp == nullptr) return;
    job_entry_list->remove(temp);
    delete temp;
}

void JobsList::removeFinishedJobs(){
    if(job_entry_list->empty()) return;
    int res = waitpid(-1, nullptr, WNOHANG);
    while (res > 0){
        removeJobByPID(res);
        res = waitpid(-1, nullptr, WNOHANG);
    }
}

/*
 * function returns JobEntry * or null if not found
 * it doesn't remove the item from the list!
 * */
JobsList::JobEntry * JobsList::getJobById(int jobId) {
	if(job_entry_list->empty()) return nullptr;
    std::list<JobEntry*>::iterator jobs_iterator = job_entry_list->begin();
    while (jobs_iterator != job_entry_list->end()){
        if((*jobs_iterator)->getJobId() == jobId) return (*jobs_iterator);
        jobs_iterator++;
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
	if(job_entry_list->empty()) return;
    std::list<JobEntry*>::iterator jobs_iterator = job_entry_list->begin();
    while (jobs_iterator != job_entry_list->end()){
        if((*jobs_iterator)->getJobId() == jobId) {
            job_entry_list->erase(jobs_iterator);
            delete (*jobs_iterator);
        }
        jobs_iterator++;
    }
}

JobsList::JobEntry * JobsList::getLastJob(int *lastJobId) {
	if(job_entry_list->empty()) return nullptr;
    std::list<JobEntry*>::iterator last_job = job_entry_list->end();
    last_job--;
    if(*last_job != nullptr){
        *lastJobId = (*last_job)->getJobId();
        return (*last_job);
    }
    return nullptr;
}

JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId) {
  if(job_entry_list->empty()) return nullptr;
  std::list<JobEntry*>::iterator jobs_iterator = job_entry_list->end();
    jobs_iterator--;
    while (jobs_iterator != job_entry_list->end()){
        if((*jobs_iterator)->isJobStopped()) {
            *jobId = (*jobs_iterator)->getJobId();
            return (*jobs_iterator);
        }
        jobs_iterator--;
    }
    return nullptr;
}

JobsList::JobEntry * JobsList::getJobByPID(int job_pid) {
    if(job_entry_list->empty()) return nullptr;
    std::list<JobEntry*>::iterator jobs_iterator = job_entry_list->begin();
    while (jobs_iterator != job_entry_list->end()){
        if((*jobs_iterator)->getJobPid() == job_pid) {
            return (*jobs_iterator);
        }
        jobs_iterator++;
    }
    return nullptr;
}

SmallShell::SmallShell() : smashPID(getpid()) {
    this->job_list = new JobsList();
    this->timeout_list = new TimeOutList();
}

SmallShell::~SmallShell() {
    delete job_list;
    delete timeout_list;
}

bool isBuiltInCmd(string firstWord){
    return ((firstWord=="chprompt")||(firstWord=="showpid")||(firstWord=="pwd")||(firstWord=="cd")||(firstWord=="jobs")||
            (firstWord=="kill")||(firstWord=="fg")||(firstWord=="bg")||(firstWord=="quit"));
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

    string cmd_s = _trim(string(cmd_line));
    string first_word = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    string firstWord;

    // need to get rid of & in builtin commands somehow
    char *first_word_to_check = new char[COMMAND_ARGS_MAX_LENGTH];
    int len = first_word.length();
    strcpy(first_word_to_check, first_word.c_str());

    if(_isBackgroundComamnd(cmd_line) && isBuiltInCmd(first_word.substr(0,first_word.length() - 1))){
        _removeBackgroundSign(first_word_to_check);
        firstWord = string(first_word_to_check).substr(0,len - 1);
    } else {
        firstWord = first_word_to_check;
    }

    Command* cmd; // create cmd pointer, will be allocated below

    if(firstWord == "timeout") {
        cmd = new TimeOutCommand(cmd_line);
    } else if (strstr(cmd_line, ">>") || strstr(cmd_line, ">")) {
        cmd = new RedirectionCommand(cmd_line);
    } else if (strstr(cmd_line, "|") || strstr(cmd_line, "|&")) {
        cmd = new PipeCommand(cmd_line);
    } else if (firstWord == "showpid") {
        cmd = new ShowPidCommand(cmd_line);
    } else if (firstWord == "pwd") {
        cmd = new GetCurrDirCommand(cmd_line);
    } else if (firstWord == "cd") {
        cmd = new ChangeDirCommand(cmd_line);
    } else if (firstWord == "jobs") {
        cmd = new JobsCommand(cmd_line);
    } else if (firstWord == "kill") {
        cmd = new KillCommand(cmd_line);
    } else if (firstWord == "fg") {
        cmd = new ForegroundCommand(cmd_line);
    } else if (firstWord == "bg") {
        cmd = new BackgroundCommand(cmd_line);
    } else if (firstWord == "quit") {
        cmd = new QuitCommand(cmd_line);
    } else if (firstWord == "chprompt") {
        cmd = new ChpromptCommand(cmd_line);
    } else if (firstWord == "cat") {
        cmd = new CatCommand(cmd_line);
    } else {
        cmd = new ExternalCommand(cmd_line);
    }

    delete[] first_word_to_check;

    return cmd;

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  this->job_list->removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line);
  if(cmd != nullptr) {

      cmd->execute();

      delete cmd;
  }
}

Command::Command(const char* cmd_line) : args(new char *[COMMAND_MAX_ARGS]) {
    this->cmd_line = cmd_line;
    this->args_count = _parseCommandLine(cmd_line, this->args);
    this->isBackground = _isBackgroundComamnd(cmd_line);
}

void Command::delete_args() {
    for (int i = 0; i < this->args_count; ++i) {
        free(args[i]);
    }
}

Command::~Command(){
    delete_args();
    delete[] this->args;
}

bool Command::isBuiltin() {
    char* x = this->args[1];
    return ((x=="chprompt")||(x=="showpid")||(x=="pwd")||(x=="cd")||(x=="jobs")||(x=="kill")||(x=="fg")||(x=="bg")||(x=="quit"));
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line), smash(SmallShell::getInstance()) {}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute() {
    bool is_background = _isBackgroundComamnd(this->getCmdLine().c_str());

    SmallShell &sm = SmallShell::getInstance();

    /*  cast const char* to char* for execv() function in line 336 */
    char *cmd_str = new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_str,this->getCmdLine().c_str());

    if(is_background) _removeBackgroundSign(cmd_str);

    int p = fork();

    if (p < 0) {
        perror("smash error: fork failed");
        delete[] cmd_str;
        return;
    }
    else if (p == 0) { // son
        setpgrp();
        char* argv[] = {(char*)"/bin/bash", (char*)"-c", cmd_str, NULL};
        if(execv(argv[0], argv) < 0) {
            perror("smash error: execv failed");
            exit(1);
        }
    } else { // parent
        if(this->isTimeOutCmd()){
            sm.getTimeOutList()->addTimeOutEntry(this->getCmdLine().c_str(), p,this->getKillTime());
        }
        //bg
        if(is_background){
            sm.getJobList()->addJob(this, false, p);

        } else { // is foreground
            sm.setFgJobPID(p);

            waitpid(p,NULL,0 | WUNTRACED);

            sm.setFgJobPID(-1);
        }
    }

    delete[] cmd_str;
}

/*
 *
 *              BUILTIN COMMANDS!
 *
 * */

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() {
    if (this->getNumArgs() < 2) {
        smash.setPromptName("smash");
        return;
    }
    smash.setPromptName(this->getArgs()[1]);
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << smash.getSmashId() << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char cwd[PATH_MAX];
    if(getcwd(cwd, sizeof(cwd)) == nullptr){
        perror("smash error: getcwd failed");
        return;
    }
    cout << cwd << endl;
}

// changed the ctor - not sure why we want to pass additional argument to the ctor?
ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute() {
    char cwd[PATH_MAX], path[PATH_MAX];
    if(getcwd(cwd, sizeof(cwd)) == nullptr){
        perror("smash error: getcwd failed");
        return;
    }

    if (this->getNumArgs() > 2) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    if (this->getNumArgs() == 2 && strcmp(this->getArgs()[1], "-") == 0 && this->smash.getLastDir().empty()) {
        cerr << "smash error: cd: OLDPWD not set" << endl;
        return;
    }

    if (this->getNumArgs() == 1) {
        // strcpy(path, "/"); // Not sure if we need to do nothing or go to root dir https://piazza.com/class/kmeyq2ecrv940z?cid=89
        return;
    } else if (this->getNumArgs() == 2 && strcmp(this->getArgs()[1], "-") == 0) {
        strcpy(path, this->smash.getLastDir().c_str()); // go to last dir
    } else {
        strcpy(path, this->getArgs()[1]); // go to the path specified in args[1]
    }

    int res = chdir(path);
    if (res != 0) {
        perror("smash error: chdir failed");
        return;
    }
    this->smash.setLastDir(string(cwd));
}

JobsCommand::JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void JobsCommand::execute() {
    SmallShell &sm = SmallShell::getInstance();
    sm.getJobList()->printJobsList();
}

KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

bool checkNumber(string str) {
    for (char c : str) {
        if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

void KillCommand::execute() {
	/* corner case: 
	 * invalid num of arguments
	 * no '-' before signal
	 * signal is not a number i.e. kill -<char> job_id
	 * */
	if((this->getNumArgs() != 3) || (this->getArgs()[1][0] != '-')){
		cerr << "smash error: kill: invalid arguments" << endl;
        return;
	}
	
	
	/* corner case:
	 * negative job-id e.g. kill -9 -10
	 * */
	if((strlen(this->getArgs()[2]) > 1) && (this->getArgs()[2][0] == '-') && (checkNumber(string(this->getArgs()[2] + 1)))){
        if(!checkNumber(string(this->getArgs()[1] + 1))) {
			cerr << "smash error: kill: invalid arguments" << endl;
			return;
		}
        cerr << "smash error: kill: job-id " << this->getArgs()[2] << " does not exist" << endl;
        return;
    }
    
    /* corner case:
	 * job-id is not a number e.g. kill -9 abd
	 * */
    if(!checkNumber(string(this->getArgs()[2]))){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    
    /* corner case:
     * signal is not a number e.g. kill - 5 or kill -- 1
     * */
     if(strlen(this->getArgs()[1]) < 2 || 
		(strlen(this->getArgs()[1]) == 2 && !isdigit(*(this->getArgs()[1] + 1)))) {
			cerr << "smash error: kill: invalid arguments" << endl;
			return;
	  }
    
    if(((strlen(this->getArgs()[1]) > 1) && (this->getArgs()[1][1] == '-') &&
		(checkNumber(string(this->getArgs()[1] + 2)))) || (checkNumber(string(this->getArgs()[1] + 1)))) {
	
		int job_id = atoi(this->getArgs()[2]);
		int signal_number = atoi(this->getArgs()[1] + 1);

		SmallShell &sm = SmallShell::getInstance();

		JobsList *job_list = sm.getJobList();

		JobsList::JobEntry *job = job_list->getJobById(job_id);

		if (job == nullptr) {
			cerr << "smash error: kill: job-id " << job_id << " does not exist" << endl;
			return;
		}

		string pid;
		pid += std::to_string(job->getJobPid());
	   
		if (kill(job->getJobPid(), signal_number) < 0) {
			perror("smash error: kill failed");
			return;
		} else {
			cout << "signal number " << signal_number << " was sent to pid " << pid << endl;
		}

		switch (signal_number) {
			case SIGSTOP:
				job->setStopped(true);
				break;
			case SIGCONT:
				job->setStopped(false);
				break;
			case SIGKILL:
				job_list->removeJobByPID(job->getJobPid());
				break;
			case SIGALRM:
				job_list->removeJobByPID(job->getJobPid());
				break;
			//case SIGEMT:
			//    job_list->removeJobByPID(job->getJobPid());
			 //   break;
			case SIGHUP:
				job_list->removeJobByPID(job->getJobPid());
				break;
			case SIGIO:
				job_list->removeJobByPID(job->getJobPid());
				break;
			//case SIGLOST: // declared unused by manpage
				//job_list->removeJobByPID(job->getJobPid());
				//break;
			case SIGPIPE:
				job_list->removeJobByPID(job->getJobPid());
				break;
			case SIGPROF:
				job_list->removeJobByPID(job->getJobPid());
				break;
			case SIGPWR:
				job_list->removeJobByPID(job->getJobPid());
				break;
			case SIGSTKFLT:
				job_list->removeJobByPID(job->getJobPid());
				break;
			case SIGTERM:
				job_list->removeJobByPID(job->getJobPid());
				break;
			case SIGVTALRM:
				job_list->removeJobByPID(job->getJobPid());
				break;
			case SIGTTIN:
				job->setStopped(true);
				break;
			case SIGTTOU:
				job->setStopped(true);
				break;
		}
	} else {
		cerr << "smash error: kill: invalid arguments" << endl;
        return;
	}
}

ForegroundCommand::ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ForegroundCommand::execute() {

    if(smash.getJobList()->isEmpty() && this->getNumArgs() == 2 && checkNumber(string(this->getArgs()[1]))) {
        cerr << "smash error: fg: job-id " << this->getArgs()[1] << " does not exist" << endl;
        return;
    }
    
    if(this->getNumArgs() == 2 && this->getArgs()[1][0] == '-') {
        cerr << "smash error: fg: job-id " << this->getArgs()[1] << " does not exist" << endl;
        return;
    }

    if(smash.getJobList()->isEmpty() && this->getNumArgs() == 1){
        cerr << "smash error: fg: jobs list is empty" << endl;
        return;
    }
    
    if (this->getNumArgs() > 2 || (this->getNumArgs() == 2 && !checkNumber(string(this->getArgs()[1])))) {
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }

    int job_id;
    JobsList::JobEntry *job;
    if (this->getNumArgs() == 1) {
        job = smash.getJobList()->getLastJob(&job_id);
    } else {
        job_id = atoi(this->getArgs()[1]);
        job = smash.getJobList()->getJobById(job_id);
        if (job == nullptr) {
            cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
            return;
        }
    }
    

    cout << job->getCmdLine() << " : " << job->getJobPid() << endl;

    //DO_SYS(kill(job->getJobPid(),SIGCONT));

    if(kill(job->getJobPid(),SIGCONT) < 0){
        perror("smash error: kill failed");
        return;
    }

    job->setStopped(false);
    job->setBackground(false);

    smash.setFgJobPID(job->getJobPid());

    // mark job as FG
    smash.setFgJobID(job_id);

    if(waitpid(job->getJobPid(),NULL,0 | WUNTRACED) < 0){
        perror("smash error: waitpid failed");
        smash.setFgJobPID(-1);
        smash.setFgJobID(-1);
        return;
    }

    // check if stopped or finished
    if(!job->isJobStopped()) {
		smash.getJobList()->removeJobByPID(job->getJobPid());
	}

    smash.setFgJobPID(-1);

    smash.setFgJobID(-1);
}

BackgroundCommand::BackgroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void BackgroundCommand::execute() {
	if((this->getNumArgs() == 2) && (string(this->getArgs()[1]).substr(0,1) == "-")) {
        cerr << "smash error: bg: job-id " << this->getArgs()[1] << " does not exist" << endl;
        return;
    }

    if(this->getNumArgs() == 2 && !checkNumber(string(this->getArgs()[1]))) {
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    
    if(smash.getJobList()->isEmpty() && this->getNumArgs() == 2 && checkNumber(string(this->getArgs()[1]))) {
        cerr << "smash error: bg: job-id " << this->getArgs()[1] << " does not exist" << endl;
        return;
    }

    int job_id;
    JobsList::JobEntry *job;

    if(this->getNumArgs() == 2 && checkNumber(string(this->getArgs()[1]))) {
        job_id = atoi(this->getArgs()[1]);
        job = smash.getJobList()->getJobById(job_id);
        if (job == nullptr) {
            cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
            return;
        } else if (!job->isJobStopped()) {
            cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
            return;
        }
    } else if (this->getNumArgs() == 1) {
        job = smash.getJobList()->getLastStoppedJob(&job_id);
        if (job == nullptr) {
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    } else {
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }

    cout << job->getCmdLine() << " : " << job->getJobPid() << endl;

    // DO_SYS(kill(job->getJobPid(),SIGCONT)); // macro to checks syscall integrity

    if(kill(job->getJobPid(),SIGCONT)==-1){
        perror("smash error: kill failed");
        return;
    }

    job->setStopped(false);
}

QuitCommand::QuitCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void QuitCommand::execute() {
    if (this->getNumArgs() >= 2 && strcmp(this->getArgs()[1], "kill") == 0) {
        smash.getJobList()->killAllJobs();
    }
    exit(0);
}


/*
 *
 *              SPECIAL COMMANDS!
 *
 * */


/**
 * Pipe constructor
 * @param cmd_line - the command string
 */
PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {
    char cmd_str[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_str, cmd_line);

//  separate the first and second commands by the pipe symbol
    char *line1;
    char *line2 = cmd_str;
    line1 = strsep(&line2, "|");

//  check what type of pipe to create and set the operation field of the command accordingly
    if (line2 && *line2 == '&') {
        ++line2;
        this->operation = "|&";
    } else {
        this->operation = "|";
    }

//  create command object for each command and set the appropriate PipeCommand fields accordingly
    SmallShell &sm = SmallShell::getInstance();

    this->cmd1 = sm.CreateCommand(line1);
    this->cmd2 = sm.CreateCommand(line2);
}


/**
 * Pipe destructor
 */
PipeCommand::~PipeCommand() {
    delete this->cmd1;
    delete this->cmd2;
}

// Read and Write enums for the FD returned by the pipe
enum {
    RD = 0, WT = 1
};

/**
 * Pipe execute command.
 *
 */
void PipeCommand::execute() {
// what is this for?
    //cout << this->operation << endl;

//  chooses the right FD according to the pipe command (stdout or stderr)
    int channel = (this->operation == "|") ? 1 : 2; // stdout = 1, stderr = 2

//  create the pipe
    int fd[2];
    if(pipe(fd) == -1){
        perror("smash error: pipe failed");
        return;
    }
    pid_t pid1, pid2;

// what is this for?
    //cout << "fd[1] " << fd[1] << " fd[0] " << fd[0] << endl;

// pipe commands are always fg (ignore &) so need to set it as fg command
    SmallShell &sm = SmallShell::getInstance();
    sm.setFgJobPID(getpid());

// what is this for?
    //cout << "pid " << getpid() << endl;
    /* Instructor's answer from piazza: two ways for pipe implementation
     * 1. either redirect IO for builtin commands but don't forget to redirect it back (so smash will function properly)
     * 2. you may fork builtin commands
     * */
    //cout << "cmd1 is " << cmd1->getCmdLine() << endl;
    //cout << "cmd1 is builtin " << cmd1->isBuiltin() << endl;

    //cout << "cmd2 is " << cmd2->getCmdLine() << endl;
    //cout << "cmd2 is builtin " << cmd2->isBuiltin() << endl;


// runs  cmd1 with redirection of output if cmd1 is external
    if (!cmd1->isBuiltin()) {
        //cout << "cmd1 is " << cmd1->getCmdLine() << endl;
        pid1 = fork();
        if (pid1 < 0) {
            perror("smash error: fork failed");
            sm.setFgJobPID(-1);
            return;
        } else if (pid1 == 0) { // child
            //cout << "redirection In child " << getpid() << endl;
            // no need for setpgrp - we want the child to recieve our signals (?)
            if(dup2(fd[WT], channel) == -1){
                perror("smash error: dup2 failed");
                sm.setFgJobPID(-1);
                return;
            }
            // now stdout/stderr point to same file object as fd[WT]. we can close both pipe channels
            if(close(fd[RD]) == -1){
                perror("smash error: close failed");
                sm.setFgJobPID(-1);
                return;
            }
            if(close(fd[WT]) == -1){
                perror("smash error: close failed");
                sm.setFgJobPID(-1);
                return;
            }
            cmd1->execute();
            exit(0);
        }
    }



// runs cmd2 with redirection of input
    //cout << "in father  " << getpid() << endl;
    // even if both commands are builtin commands, need to fork since they can't run together in the same smash
    pid2 = fork();
    if (pid2 < 0) {
        perror("smash error: fork failed");
        sm.setFgJobPID(-1);
        return;
    } else if (pid2 == 0) {
        //cout << "redirection child 2 cmd2 is " << cmd2->getCmdLine() << " pid is " << getpid() << endl;
        if(dup2(fd[RD], 0) == -1){
            perror("smash error: dup2 failed");
            sm.setFgJobPID(-1);
            return;
        }
        if(close(fd[RD]) == -1){
            perror("smash error: close failed");
            sm.setFgJobPID(-1);
            return;
        }
        if(close(fd[WT]) == -1){
            perror("smash error: close failed");
            sm.setFgJobPID(-1);
            return;
        }
        cmd2->execute();
        exit(0);
    } else if (cmd1->isBuiltin()) {
        //cout << "cmd1 is builtin " << cmd1->getCmdLine() << " pid " << getpid() << endl;
        int temp = dup(channel);
        if(temp == -1){
            perror("smash error: dup failed");
            sm.setFgJobPID(-1);
            return;
        }
        if(dup2(fd[WT], channel) == -1){
            perror("smash error: dup2 failed");
            sm.setFgJobPID(-1);
            return;
        }
        if(close(fd[RD]) == -1){
            perror("smash error: close failed");
            sm.setFgJobPID(-1);
            return;
        }
        if(close(fd[WT]) == -1){
            perror("smash error: close failed");
            sm.setFgJobPID(-1);
            return;
        }
        cmd1->execute();
        if(dup2(temp, channel) == -1){
            perror("smash error: dup2 failed");
            sm.setFgJobPID(-1);
            return;
        }
    }
    //cout << "finished, in father pid " << getpid() << endl;
    if(close(fd[RD]) == -1){
        perror("smash error: close failed");
        sm.setFgJobPID(-1);
        return;
    }
    if(close(fd[WT]) == -1){
        perror("smash error: close failed");
        sm.setFgJobPID(-1);
        return;
    }
    if(waitpid(pid2, NULL, 0) == -1){
        perror("smash error: close failed");
        sm.setFgJobPID(-1);
        return;
    }

    sm.setFgJobPID(-1);

}


/**
 * Redirection command constructor
 * @param cmd_line
 */
RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {
    char cmd_str[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_str, cmd_line);

    char *command_name;
    char *file_name = cmd_str;
//  redirection is not allowed to run in bg
    _removeBackgroundSign(file_name);

//  check the type of redirection (append/override)
    if(command_name = strsep(&file_name, ">")) {
        if (file_name && *file_name == '>') {
            ++file_name;
            operation = ">>";
        } else {
            operation = ">";
        }
    } else{
        cout << 'There is a problem with redirection' << endl; // what is it? no errors were specified in the pdf, in any case, errors are printed to cerr. should return?
    }


//  save file name
    std::istringstream iss(_trim(string(file_name)).c_str());
    iss >> output_file;


//  create the command to be redirected
    SmallShell &sm = SmallShell::getInstance();
    cmd = sm.CreateCommand(command_name);


}

/**
 *  Redirection command destructor
 */
RedirectionCommand::~RedirectionCommand() {
    delete cmd;
}

/**
 * Redirection execute command
 */
void RedirectionCommand::execute() {
// set the channel to output
    int channel = 1;
    int temp_fd = dup(channel);
    if(temp_fd == -1){
        perror("smash error: dup failed");
        return;
    }


//  redirection commands are always fg (ignore &) so we need to set it as fg command
    SmallShell &sm = SmallShell::getInstance();
    sm.setFgJobPID(getpid());

    if(this->operation == ">"){
//      open output file in overwrite
        int fd = open(this->output_file.c_str(),O_RDWR|O_CREAT,  S_IRWXO|S_IRWXU|S_IRWXG);
        cout <<  'fd is: '  << std::to_string(channel) << endl;
        if(fd == -1){
            perror("smash error: open failed");
            sm.setFgJobPID(-1);
            return;
        }

//      redirect stdout to the output file
        if(dup2(fd, channel) == -1){
            perror("smash error: dup2 failed");
            sm.setFgJobPID(-1);
            return;
        }
//      execute the command with redirected output

        this->cmd->execute();

//      change the stdout to its original file
        if(dup2(temp_fd, channel) == -1){
            perror("smash error: dup2 failed");
            sm.setFgJobPID(-1);
            return;
        }

//      close unnecessary fd's
        if(close(fd) == -1 || close(temp_fd) == -1){
            perror("smash error: close failed");
            sm.setFgJobPID(-1);
            return;
        }
    } else{
//      open output file in append
        int fd = open(this->output_file.c_str(),O_RDWR | O_CREAT | O_APPEND,  S_IRWXO|S_IRWXU|S_IRWXG);
        if(fd == -1){
            perror("smash error: open failed");
            sm.setFgJobPID(-1);
            return;
        }
//      redirect stdout to the output file
        if(dup2(fd, channel) == -1){
            perror("smash error: dup2 failed");
            sm.setFgJobPID(-1);
            return;
        }
//      execute the command with redirected output
        this->cmd->execute();

//      change the stdout to its original file
        if(dup2(temp_fd, channel) == -1){
            perror("smash error: dup2 failed");
            sm.setFgJobPID(-1);
            return;
        }

//      close unnecessary fd's
        if(close(fd) == -1 || close(temp_fd) == -1){
            perror("smash error: close failed");
            sm.setFgJobPID(-1);
            return;
        }
    }
    sm.setFgJobPID(-1);
}

CatCommand::CatCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void CatCommand::execute() {
    if (this->getNumArgs() < 2) {
        cerr << "smash error: cat: not enough arguments" << endl;
        return;
    }

    // cat command is always fg (ignore &) so need to set it as fg command
    SmallShell &sm = SmallShell::getInstance();
    sm.setFgJobPID(getpid());

    char **file_ptr = this->getArgs();

    // skip cmd name arg[0] = cat
    file_ptr++;

    int counter = 0;

    // args is not null-terminated. need to know where to stop - use num of arguments (-1 since we only need the paths)
    while (counter < (this->getNumArgs() - 1)) {
        counter++;

        // open - no O_CREATE!
        int fd = open((*file_ptr), O_RDONLY);

        // check open
        if (fd == -1) {
            perror("smash error: open failed");
            sm.setFgJobPID(-1);
            return;
        }

        // allocate buffer
        char *buffer = new char[BUFFER_SIZE];

        //buffer = nullptr;

        // read from file
        int read_res = read(fd, (void *) buffer, BUFFER_SIZE);

        // read error
        if (read_res == -1) {
            perror("smash error: read failed");
            sm.setFgJobPID(-1);
            close(fd);
            return;
        }

        while (read_res > 0) { // if res == 0 we're done. if res == -1 - call Houston cause we have a problem.

            // write file to stdout
            int write_res = write(1, buffer, read_res); // 1 = fd of stdout

            // write error
            if (write_res == -1) {
                perror("smash error: write failed");
                sm.setFgJobPID(-1);
                close(fd);
                return;
            }

            // flush buffer

            // read from file
            read_res = read(fd, (void *) buffer, BUFFER_SIZE);

            // read error
            if (read_res == -1) {
                perror("smash error: read failed");
                sm.setFgJobPID(-1);
                close(fd);
                return;
            }
        }

        // read error
        if (read_res == -1) {
            perror("smash error: read failed");
            sm.setFgJobPID(-1);
            close(fd);
            return;
        }

        delete[] buffer;

        // close
        int clos_res = close(fd);

        // check close (?)
        if (clos_res == -1) {
            perror("smash error: close failed");
            sm.setFgJobPID(-1);
            return;
        }

        // continue to next file
        file_ptr++;
    }
    sm.setFgJobPID(-1);
}

TimeOutCommand::TimeOutCommand(const char* cmd_line) : Command(cmd_line) {}

TimeOutCommand::~TimeOutCommand() {}

char* parsedInnerCommand(const char* cmd_line) {
    char cmd_str[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_str, cmd_line);

	char* parsed_cmd;
    char* parsed_cmd_1;
    char* parsed_cmd_2 = cmd_str;

    parsed_cmd_1 = strsep(&parsed_cmd_2, " ");
    parsed_cmd = parsed_cmd_2;
    parsed_cmd_1 = strsep(&parsed_cmd, " ");

    return parsed_cmd;
}

void TimeOutCommand::execute() {
    if(this->getNumArgs() < 3 || !checkNumber(this->getArgs()[1])){
        return;
    }
    SmallShell &smash = SmallShell::getInstance();

    string inner_cmd_str = parsedInnerCommand(this->getCmdLine().c_str());

    Command *inner_cmd = smash.CreateCommand(inner_cmd_str.c_str());

    (*inner_cmd).setTimeOutCmd(true);

    (*inner_cmd).setKillTime(stoi(this->getArgs()[1]));

    inner_cmd->execute();

    delete inner_cmd;
}


TimeOutList::TimeOutEntry::TimeOutEntry(const char* cmd_line, int pid, long int duration) : cmd_line(cmd_line), pid(pid) {
    kill_time = duration + static_cast<long int>(time(nullptr));
    alarm(duration);
}

TimeOutList::TimeOutEntry::~TimeOutEntry() {}

TimeOutList::TimeOutList() {
    this->time_out_entry_list = new std::list<TimeOutEntry*>({});
}

TimeOutList::~TimeOutList() {

    for(auto j : *time_out_entry_list){
        delete j;
    }

    delete time_out_entry_list;
}

void TimeOutList::addTimeOutEntry(const char* cmd_line, int pid, long int kill_time){
    TimeOutEntry *time_out_entry = new TimeOutList::TimeOutEntry(cmd_line, pid, kill_time);
    this->time_out_entry_list->push_back(time_out_entry);
}

void TimeOutList::removeTimeOutEntry(int pid) {
    if(time_out_entry_list->empty()) return;
    auto entry = time_out_entry_list->begin();
    while(entry != time_out_entry_list->end()) {
        if((*entry)->pid == pid){
            time_out_entry_list->remove(*entry);
            delete *entry;
            break;
        }
    }
}
