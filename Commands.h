#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <ostream>
#include <time.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

#define MAX_COMMAND_LENGTH (128)


class Command {
// TODO: Add your data members
/* store the command to execute and pid of the process */
 private:
  std::string cmd_line;

protected:
  int args_count;       // holds number of arguments received
  char **args_val;      // parsed command-line. each cell hold another argument of the command
  std::string exec_cmd; // string holds the command to execute

 public:
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed


};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
  ChangeDirCommand(const char* cmd_line, char** plastPwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

/*  Built-in commands implementation  */

/*
 * chprompt command - change prompt name
 * it's a built in command => inherits from BuiltInCommand class
 * */

class ChangaPromptDisplayCommand : public BuiltInCommand{
public:
    ChangaPromptDisplayCommand(const char *cmd_line);

    virtual ~ChangaPromptDisplayCommand();

    void execute() override;
};


class JobsList {
 public:

  class JobEntry {
   // TODO: Add your data members
   int job_id;
   int pid;
   Command *cmd;
   time_t time_added;
   bool is_running = true;
   bool is_stopped = false;
   public:
      JobEntry(int jobID, Command *cmd) : job_id(jobID), cmd(cmd), time_added(time(nullptr)), is_stopped(false){
          // if we for here we can get cmd pid, if we fork elswhere - need to update pid
      };

      ~JobEntry() {};

      bool getRunningStatus() {
          return this->is_running; // should be cmd.is_running?
      }

      bool getStoppedStatus(){
          return this->is_stopped;
      }

      void setStoppedStatus(bool status){
          this->is_stopped = status;
      }

      int getJobId() const {
          return this->job_id;
      }

      int getPid() const {
          return this->pid;
      }

      void restartTimer() {
          this->time_added = time(nullptr);
      }


  };

 // TODO: Add your data members
private:
    std::vector<JobEntry*> jobsEntryArray;

 public:
  JobsList() {
      jobsEntryArray.push_back(nullptr);
  };
  ~JobsList(){};
  void addJob(Command* cmd, bool isStopped = false); // TODO: make sure to clean job list before adding new commend
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify existing ones as needed

  // get updated number of running jobs
  int jobsCount() {
      int count = 0;
      for (JobEntry *job_e : jobsEntryArray){
          if (!job_e->getRunningStatus() && !job_e->getStoppedStatus()) continue;
          count++;
      }
      return count;
  }

  std::ostream& print(std::ostream& os) const {
      for(JobEntry *job : jobsEntryArray){
          if(!job->getStoppedStatus() && !job->getRunningStatus()){
              os << fmtJob(job) << std::endl;          }
      }
      return os;
  }

  friend std::ostream &operator<<(std::ostream &os, const JobsList &j) {
      return j.print(os);
  }

  const JobEntry *operator[](int id) const { // do I really want const here?
      if (id < jobsEntryArray.size() && id > 0)
          return jobsEntryArray[id];
      else return nullptr;
  }

};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class CatCommand : public BuiltInCommand {
 public:
  CatCommand(const char* cmd_line);
  virtual ~CatCommand() {}
  void execute() override;
};


class SmallShell {
 private:
  // TODO: Add your data members
  JobsList jobsList;

  std::string prompt_name = "smash";
  int fg_job_id = -1;

  SmallShell() : smash_pid(getpid()) {};

 public:
  const int smash_pid;
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed

  std::string getPrompt(){
      return this->prompt_name;
  }

  void setPrompt(const std::string &updated_prompt){
      this->prompt_name = updated_prompt;
  }

  void setFgJobId(int jobID){
      this->fg_job_id = jobID;
  }

  int getFgJobId(int jobID) const {
      return this->fg_job_id;
  }

  void execute(const char *cmd_line);

  void killAllJobs(std::ostream &os){
      jobsList.killAllJobs();
  }

  std::ostream& print(std::ostream& os) const {
      return os << this->jobsList;
  }

  friend std::ostream &operator<<(std::ostream &os, const SmallShell &sm) {
      return sm.print(os);
  }

  JobsList::JobEntry* getLastJobIndex(bool isStopped, int *job_index){
      if(isStopped){
          return this->jobsList.getLastJob(job_index);
      } else{
          return this->jobsList.getLastJob(job_index);
      }
  }

  JobsList::JobEntry* getJobByID(int job_id) const {
      return this->jobsList[job_id];
  }

  bool checkJobExists(int job_id) const {
      // should check validity of ID? try-catch?
      JobsList::JobEntry *jobEntry = getJobByID(job_id);
      if(jobEntry == nullptr) return false;
      return jobEntry->getRunningStatus();
  }

  bool checkJobStopped(int job_id) const {
      JobsList::JobEntry *jobEntry = getJobByID(job_id);
      if(jobEntry == nullptr) return false;
      return jobEntry->getStoppedStatus();
  }
};

#endif //SMASH_COMMAND_H_
