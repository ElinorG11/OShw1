#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
  std::string cmd_line;
  char **args;
  int args_count;
  bool isBackground = false;
 public:
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
  void delete_args();
  const char* getCmdLine(){
      return cmd_line.c_str();
  }
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

class ChpromptCommand : public BuiltInCommand{
public:
    ChpromptCommand(const char *cmd_line);
    virtual ~ChpromptCommand();
    void execute() override;
};


class ShowProcessIdCommand : public BuiltInCommand{
public:
    ShowPidCommand(const char *cmd_line);
    virtual ~ShowProcessIdCommand();
    void execute() override;
};


class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
  private:
      std::string cmd_line;
      int job_id;
      int pid;
      bool isStopped;
      bool isBackgroundJob;
      time_t time_added;
  public:
      JobEntry(std::string cmd_line, int jobId, int pid, bool isStopped, bool is_background);

      JobEntry();

      ~JobEntry();

      int getJobId(){
          return this->job_id;
      }

      int getJobPid(){
          return this->pid;
      }

      std::string getCmdLine(){
          return this->cmd_line;
      }

      time_t getStartTime(){
          return this->time_added;
      }

      bool isJobStopped(){
          return this->isStopped;
      }

  };
 // TODO: Add your data members
 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, bool isStopped = false, int pid = -1);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed

  friend bool sortJobEntries(JobEntry *job1, JobEntry *job2);

private:
    std::list<JobEntry*> *job_entry_list;
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
  JobsList *job_list;
  int smashPID;
  int fg_job_id = -1;
  SmallShell();
 public:
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

  int getFgJobId(){
      return fg_job_id; // update in 2 places: ExtenalCommed::execute, ForegroungCommand (fg/bg)
  }

  void setFgJobId(int id){
      fg_job_id = id;
  }

  JobsList* getJobList(){
      return job_list;
  }
};

#endif //SMASH_COMMAND_H_
