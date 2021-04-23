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

// TODO: Add your implementation for classes in Commands.h

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

void JobsList::addJob(Command *cmd, bool isStopped, int pid) {
    int jobId;
    bool is_background;
    if(job_entry_list->empty()) jobId = 0;
    else {
        JobsList::getLastJob(&jobId);
    }
    is_background = _isBackgroundComamnd(cmd->getCmdLine().c_str());
    JobEntry *job_entry = new JobsList::JobEntry(cmd->getCmdLine(), jobId + 1, pid, isStopped, is_background);
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
        cout << "[" << (*job_iterator)->getJobId() << "] " << (*job_iterator)->getCmdLine() << " : " << (*job_iterator)->getJobPid() <<
             difftime(time(nullptr),(*job_iterator)->getStartTime()) << addend << endl;
        job_iterator++;
    }
}

void JobsList::killAllJobs() {
    //if(job_entry_list->empty()) return;
    JobsList::removeFinishedJobs();
    cout << "smash: sending SIGKILL signal to " << job_entry_list->size() << " jobs:" << endl;
    std::list<JobEntry*>::iterator job_iterator = job_entry_list->begin();
    while(job_iterator != job_entry_list->end()){
        cout << (*job_iterator)->getJobPid() << ": " << (*job_iterator)->getCmdLine() << endl;
        if(kill((*job_iterator)->getJobPid(),SIGKILL) == -1) perror("smash error: kill failed");
        job_iterator++;
    }
}

void JobsList::removeJobByPID(int pid){
    if(job_entry_list->empty()) return;
    //cout << "list not empty" << endl;
    JobEntry *temp = getJobByPID(pid);
    //cout << "got job by pid" << pid << endl;
    if(temp == nullptr) return;
    job_entry_list->remove(temp);
    //cout << "removed job" << endl;
    delete temp;
    //cout << "job deleted successfully" << endl;
}

void JobsList::removeFinishedJobs(){
    if(job_entry_list->empty()) return;
    pid_t res = waitpid(-1, nullptr, WNOHANG);
    //cout << "first res" << res << endl;
    while (res > 0){
        //cout << "removing finished jobs, still hasnt crushed" << endl;
        removeJobByPID(res);
        //cout << "passed remove job by pid" << endl;
        res = waitpid(-1, nullptr, WNOHANG);
        //cout << "res now is" << res << endl;
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
    //cout << "inside get job by pid - list not empty" << endl;
    std::list<JobEntry*>::iterator jobs_iterator = job_entry_list->begin();
    //cout << "got job iterator" << endl;
    while (jobs_iterator != job_entry_list->end()){
        //cout << "searching for job by pid inside while" << endl;
        if((*jobs_iterator)->getJobPid() == job_pid) {
            //cout << "found job, job pid: "<< endl;
            return (*jobs_iterator);
        }
        jobs_iterator++;
    }
    //cout << "found no job, returning null" << endl;
    return nullptr;
}

SmallShell::SmallShell() : smashPID(getpid()) {
    // TODO: add your implementation

    this->job_list = new JobsList();
}

SmallShell::~SmallShell() {
    // TODO: add your implementation

    delete job_list;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    Command* cmd; // create cmd pointer will be allocated below

    if (strstr(cmd_line, ">>") || strstr(cmd_line, ">")) {
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
        //cout << "this is an external command!" << endl;
        cmd = new ExternalCommand(cmd_line);
    }

    return cmd;

	// For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here

  //cout << "in SM::exec" << endl;
  this->job_list->removeFinishedJobs();
  //cout << "removed jobs from job list b4 executing anothe command";
  Command* cmd = CreateCommand(cmd_line);
  if(cmd != nullptr) {
      //cout << "executing: " << cmd->getCmdLine() << endl;
      cmd->execute();
      //cout << "done executing" << endl;
      delete cmd;
      //cout << "delete successful" << endl;
  }

  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
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
    //if(is_background) cout << "this is background command "  << this->getCmdLine() << endl;

    SmallShell &sm = SmallShell::getInstance();

    /*  cast const char* to char* for execv() function in line 336 */
    char *cmd_str = new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_str,this->getCmdLine().c_str());

    if(is_background) _removeBackgroundSign(cmd_str);

    //cout << "command w.o bg sign " << cmd_str << endl;

    // TODO: try updating job-list b4 forking

    int p = fork();
    //cout << p << endl;
    if (p < 0) {
        perror("smash error: fork failed");
    }
    else if (p == 0) { // son
        //cout << "calling bash, C ya! pid child " << getpid() << " father pid " << getppid() << endl;
        setpgrp();
        char* argv[] = {(char*)"/bin/bash", (char*)"-c", cmd_str, NULL};
        execv(argv[0], argv);
        perror("smash error: execv failed");
    } else { // parent
        //bg
        if(is_background){
            //cout << "I'm bg, job pid " << getpid() << endl;
            sm.getJobList()->addJob(this, false, p);
            //cout << "I was added to the job list" << endl;
        } else { // is foreground
            sm.setFgJobPID(p);
            //cout << "job pid " << sm.getFgJobPID() << " child pid " << p << " father pid " << getpid() << endl;
            waitpid(p,NULL,0 | WUNTRACED);
            sm.setFgJobPID(-1);
            //cout << "done waiting, bye!" << endl;
        }
    }

    delete[] cmd_str;

    //cout << "delete was successful" << endl;
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
    cout << "smash pid is: " << smash.getSmashId() << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    cout << cwd << endl;
}

// changed the ctor - not sure why we want to pass additional argument to the ctor?
ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute() {
    char cwd[PATH_MAX], path[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    if (this->getNumArgs() > 2) {
        cout << "smash error: cd: too many arguments" << endl;
        return;
    }
    if (this->getNumArgs() == 2 && strcmp(this->getArgs()[1], "-") == 0 && this->smash.getLastDir().empty()) {
        cout << "smash error: cd: OLDPWD not set" << endl;
        return;
    }

    if (this->getNumArgs() == 1) {
        strcpy(path, "/"); // stay at the same location
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
    if (this->getNumArgs() != 3 || this->getArgs()[1][0] != '-' || !checkNumber(string(this->getArgs()[1] + 1)) || !checkNumber(string(this->getArgs()[2]))) {
        cout << "smash error: kill: invalid arguments" << endl;
        return;
    }

    int job_id = atoi(this->getArgs()[2]);
    int signal_number = atoi(this->getArgs()[1] + 1);

    SmallShell &sm = SmallShell::getInstance();

    JobsList *job_list = sm.getJobList();

    JobsList::JobEntry *job = job_list->getJobById(job_id);

    if (job == nullptr) {
        cout << "smash error: kill: job-id " << job_id << " does not exist" << endl;
        return;
    }

    string pid;
    pid += std::to_string(job->getJobPid());
    pid += ":";
    pid += job->getCmdLine();
    if (kill(job->getJobPid(), signal_number) < 0) {
        perror("smash error: kill failed");
        return;
    } else {
        cout << "signal number " << signal_number << " was sent to pid " << pid << endl;
    }
    
    switch(signal_number) {
        case SIGSTOP:
            job->setStopped(true);
            break;
        case SIGCONT:
            job->setStopped(false);
        case SIGKILL:
			job_list->removeJobByPID(job->getJobPid());
    }
}

ForegroundCommand::ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ForegroundCommand::execute() {
    if (this->getNumArgs() > 2 || (this->getNumArgs() == 2 && !checkNumber(string(this->getArgs()[1])))) {
        cout << "smash error: fg: invalid arguments" << endl;
        return;
    }

    if (smash.getJobList()->isEmpty()) {
        cout << "smash error: fg: jobs list is empty" << endl;
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
            cout << "smash error: fg: job-id " << job_id << " does not exist" << endl;
            return;
        }
    }

    cout << job->getCmdLine() << " : " << job->getJobPid() << endl;

    job->setStopped(false);

    smash.setFgJobPID(job->getJobPid());
	
	//cout << "new smash pid: " << smash.getFgJobPID() << endl;

    if(kill(job->getJobPid(),SIGCONT) < 0){
        perror("smash error: kill failed");
        return;
    }
	
	//cout << "kill signal SIGCONT sent" << endl;

    
	
	//cout << "removed job successfully" << endl;

    if(waitpid(job->getJobPid(),NULL,0 | WUNTRACED) < 0){
        perror("smash error: waitpid failed");
        return;
    }
    
    smash.getJobList()->removeJobByPID(job->getJobPid());
	
	//cout << "done waiting" << endl;

    smash.setFgJobPID(-1);
	
	//cout << "fg job pid was reset" << endl;
}

BackgroundCommand::BackgroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void BackgroundCommand::execute() {
    if (this->getNumArgs() > 2 || (this->getNumArgs() == 2 && !checkNumber(string(this->getArgs()[1])))) {
        cout << "smash error: bg: invalid arguments" << endl;
        return;
    }

    int job_id;
    JobsList::JobEntry *job;
    if (this->getNumArgs() == 1) {
        job = smash.getJobList()->getLastStoppedJob(&job_id);
        if (job == nullptr) {
            cout << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    } else {
        job_id = atoi(this->getArgs()[1]);
        job = smash.getJobList()->getJobById(job_id);
        if (job == nullptr) {
            cout << "smash error: bg: job-id " << job_id << " does not exist" << endl;
            return;
        } else if (!job->isJobStopped()) {
            cout << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
            return;
        }
    }

    cout << job->getCmdLine() << " : " << job->getJobPid() << endl;

    // DO_SYS(kill(job->getJobPid(),SIGCONT)); // macro to checks syscall integrity

    job->setStopped(false);

    if(kill(job->getJobPid(),SIGCONT)==-1){
        perror("smash error: kill failed");
        return;
    }
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

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {
    char cmd_str[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_str, cmd_line);

    char *line1;
    char *line2 = cmd_str;

    line1 = strsep(&line2, "|");

    if (line2 && *line2 == '&') {
        ++line2;
        this->operation = "|&";
    } else {
        this->operation = "|";
    }

    SmallShell &sm = SmallShell::getInstance();

    this->cmd1 = sm.CreateCommand(line1);
    this->cmd2 = sm.CreateCommand(line2);
}

PipeCommand::~PipeCommand() {
    delete this->cmd1;
    delete this->cmd2;
}

enum {
    RD = 0, WT = 1
};

void PipeCommand::execute() {
    cout << this->operation << endl;

    int channel = (this->operation == "|") ? 1 : 2; // stdout = 1, stderr = 2
    int fd[2];
    pipe(fd);
    pid_t pid1, pid2;

    cout << "fd[1] " << fd[1] << " fd[0] " << fd[0] << endl;

    // pipe commands are always fg (ignore &) so need to set it as fg command
    SmallShell &sm = SmallShell::getInstance();

    sm.setFgJobPID(getpid());

    cout << "pid " << getpid() << endl;
    /* Instructor's answer from piazza: two ways for pipe implementation
     * 1. either redirect IO for builtin commands but don't forget to redirect it back (so smash will function properly)
     * 2. you may fork builtin commands
     * */
    cout << "cmd1 is " << cmd1->getCmdLine() << endl;
    cout << "cmd1 is builtin " << cmd1->isBuiltin() << endl;

    cout << "cmd2 is " << cmd2->getCmdLine() << endl;
    cout << "cmd2 is builtin " << cmd2->isBuiltin() << endl;
    if (!cmd1->isBuiltin()) {
        cout << "cmd1 is " << cmd1->getCmdLine() << endl;
        pid1 = fork();
        if (pid1 < 0) {
            perror("smash error: fork failed");
            return;
        } else if (pid1 == 0) { // child
            cout << "redirection In child " << getpid() << endl;
            // no need for setpgrp - we want the child to recieve our signals (?)
            dup2(fd[WT], channel);
            // now stdout/stderr point to same file object as fd[WT]. we can close both pipe channels
            close(fd[RD]);
            close(fd[WT]);
            cmd1->execute();
            exit(0);
        }
    }

    cout << "in father  " << getpid() << endl;
    // even if both commands are builtin commands, need to fork since they can't run together in the same smash
    pid2 = fork();
    if (pid2 < 0) {
        perror("smash error: fork failed");
        return;
    } else if (pid2 == 0) {
        cout << "redirection child 2 cmd2 is " << cmd2->getCmdLine() << " pid is " << getpid() << endl;
        dup2(fd[RD], 0);
        close(fd[RD]);
        close(fd[WT]);
        cmd2->execute();
        exit(0);
    } else if (cmd1->isBuiltin()) {
        cout << "cmd1 is builtin " << cmd1->getCmdLine() << " pid " << getpid() << endl;
        int temp = dup(channel);
        dup2(fd[WT], channel);
        close(fd[RD]);
        close(fd[WT]);
        cmd1->execute();
        dup2(temp, channel);
    }
    cout << "finished, in father pid " << getpid() << endl;
    close(fd[RD]);
    close(fd[WT]);
    waitpid(pid2, NULL, 0);

    sm.setFgJobPID(-1);

}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {
    char cmd_str[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_str, cmd_line);

    char *command_name;
    char *file_name = cmd_str;

    command_name = strsep(&file_name, ">");

    if (file_name && *file_name == '>') {
        ++file_name;
        operation = ">>";
    } else {
        operation = ">";
    }

    std::istringstream iss(_trim(string(file_name)).c_str());
    iss >> output_file;

    SmallShell &sm = SmallShell::getInstance();

    cmd = sm.CreateCommand(command_name);
}

RedirectionCommand::~RedirectionCommand() noexcept {
    delete cmd;
}

void RedirectionCommand::execute() {
    cout << "dummy exec" << endl;
}

CatCommand::CatCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void CatCommand::execute() {
    if (this->getNumArgs() < 2) {
        cout << "smash error: cat: not enough arguments" << endl;
        return;
    }

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
            return;
        }

        while (read_res > 0) { // if res == 0 we're done. if res == -1 - call Houston cause we have a problem.

            // write file to stdout
            int write_res = write(1, buffer, read_res); // 1 = fd of stdout

            // write error
            if (write_res == -1) {
                perror("smash error: write failed");
                return;
            }

            // flush buffer

            // read from file
            read_res = read(fd, (void *) buffer, BUFFER_SIZE);

            // read error
            if (read_res == -1) {
                perror("smash error: read failed");
                return;
            }
        }

        // read error
        if (read_res == -1) {
            perror("smash error: read failed");
            return;
        }

        delete buffer;

        // close
        int clos_res = close(fd);

        // check close (?)
        if (clos_res == -1) {
            perror("smash error: close failed");
            return;
        }

        // continue to next file
        file_ptr++;
    }
}
