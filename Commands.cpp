#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

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

JobsList::JobsList() {
    this->job_entry_list = new std::list<JobEntry*>({});
}

JobsList::~JobsList() {
    std::list<JobEntry*>::iterator jobs_iterator = job_entry_list->begin();
    while (*jobs_iterator != nullptr){
        delete *jobs_iterator;
        jobs_iterator++;
    }
    delete job_entry_list;
}

bool sortJobEntries(JobsList::JobEntry *job1, JobsList::JobEntry *job2){
    return job1->getJobId() < job2->getJobId();
}

void JobsList::addJob(Command *cmd, bool isStopped, int pid) {
    int jobId;
    bool is_background;
    if(job_entry_list->empty()) jobId = 1;
    else {
        JobsList::getLastJob(&jobId);
    }
    is_background = _isBackgroundComamnd(cmd->getCmdLine());
    JobEntry *job_entry = new JobEntry(cmd->getCmdLine(), jobId, pid, isStopped, is_background);
    job_entry_list->push_back(job_entry);
    this->job_entry_list->sort(sortJobEntries);
}

void JobsList::printJobsList() {
    JobsList::removeFinishedJobs();
    std::list<JobEntry*>::iterator job_iterator = job_entry_list->begin();
    while (*job_iterator != nullptr){
        std::string addend;
        if((*job_iterator)->isJobStopped()){
            addend = " secs (stopped)";
        }
        else {
            addend = " secs";
        }
        cout << (*job_iterator)->getJobId() << (*job_iterator)->getCmdLine() << " : " << (*job_iterator)->getJobPid() <<
             difftime(time(nullptr),(*job_iterator)->getStartTime()) << addend << endl;
        job_iterator++;
    }
}

void JobsList::killAllJobs() {
    JobsList::removeFinishedJobs();
    cout << "smash: sending SIGKILL signal to " << job_entry_list->size() << " jobs:" << endl;
    std::list<JobEntry*>::iterator job_iterator = job_entry_list->begin();
    while(*job_iterator != nullptr){
        cout << (*job_iterator)->getJobPid() << ": " << (*job_iterator)->getCmdLine() << endl;
        if(kill((*job_iterator)->getJobPid(),SIGKILL) == -1) perror("smash error: kill failed");
    }
}

void JobsList::removeFinishedJobs(){
    std::list<JobEntry*>::iterator jobs_iterator = job_entry_list->begin();
    while (*jobs_iterator != nullptr){
        int res = waitpid((*jobs_iterator)->getJobPid(), nullptr, WNOHANG);
        if (res != 0) {
            delete *jobs_iterator;
        }
    }
}

/*
 * function returns JobEntry * or null if not found
 * it doesn't remove the item from the list!
 * */
JobsList::JobEntry * JobsList::getJobById(int jobId) {
    std::list<JobEntry*>::iterator jobs_iterator = job_entry_list->begin();
    while (*jobs_iterator != nullptr){
        if((*jobs_iterator)->getJobId() == jobId) return (*jobs_iterator);
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    std::list<JobEntry*>::iterator jobs_iterator = job_entry_list->begin();
    while (*jobs_iterator != nullptr){
        if((*jobs_iterator)->getJobId() == jobId) delete (*jobs_iterator);
    }
}

JobsList::JobEntry * JobsList::getLastJob(int *lastJobId) {
    std::list<JobEntry*>::iterator last_job = job_entry_list->end();
    last_job--;
    *lastJobId = (*last_job)->getJobId();
    return (*last_job);
}

JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId) {
    std::list<JobEntry*>::iterator jobs_iterator = job_entry_list->end();
    jobs_iterator--;
    while (*jobs_iterator != nullptr){
        if((*jobs_iterator)->isJobStopped()) {
            *jobId = (*jobs_iterator)->getJobId();
            return (*jobs_iterator);
        }
    }
}

SmallShell::SmallShell() : smashPID(getpid()) {
    // TODO: add your implementation
    JobsList *jobsList = new JobsList();
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
    } else {
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

  this->job_list->removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line);
  if(cmd != nullptr) cmd->execute();
  delete cmd;

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

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute() {
    bool is_background = _isBackgroundComamnd(this->getCmdLine());

    SmallShell &sm = SmallShell::getInstance();

    pid_t child_pid;

    //bg
    if(is_background){
        sm.getJobList()->addJob(this);

        char *cmd_str = new char[COMMAND_ARGS_MAX_LENGTH];
        strcpy(cmd_str,this->getCmdLine());

        pid_t p = fork();
        if (p < 0) {
            perror("smash error: fork failed");
        }
        else if (p == 0) { // son
            setpgrp();
            char* argv[] = {(char*)"/bin/bash", (char*)"-c", cmd_str, NULL};
            execv(argv[0], argv);
            perror("smash error: execv failed");
        } else { // parent

        }

        delete cmd_str;
    }
}