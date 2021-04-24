#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (30) // just in case 20 doesn't include the cmd name

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
  // TODO: Add your extra methods if needed

  void delete_args();

  std::string getCmdLine(){
      return cmd_line.c_str();
  }

  void setCmdLine(std::string cmd_line){
      this->cmd_line = cmd_line;
  }

  int getNumArgs(){
      return this->args_count;
  }

  char** getArgs(){
      return this->args;
  }

  bool isBuiltin();
};

class SmallShell;

class BuiltInCommand : public Command {
protected:
    SmallShell &smash;
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
  Command *cmd1;
  Command *cmd2;
  std::string operation;
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand();
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 Command *cmd;
 std::string output_file;
 std::string operation;
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand();
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
  // TODO: Add your data members public:
private:
    //std::string plastPwd;
public:
  // ChangeDirCommand(const char* cmd_line, char** plastPwd);
  ChangeDirCommand(const char* cmd_line);
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
// TODO: Add your data members
public:
  // QuitCommand(const char* cmd_line, JobsList* jobs);
  QuitCommand(const char* cmd_line);
  virtual ~QuitCommand() {}
  void execute() override;
};

class ChpromptCommand : public BuiltInCommand{
public:
    ChpromptCommand(const char* cmd_line);
    virtual ~ChpromptCommand() {}
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

      ~JobEntry() = default;

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

      void setStopped(bool status){
          this->isStopped = status;
      }

      bool isJobStopped(){
          return this->isStopped;
      }

      void setBackground(bool status) {
          this->isBackgroundJob = status;
      }

      bool getBackground() const {
          return this->isBackgroundJob;
      }

      bool operator==(const JobsList::JobEntry& job) {
          return this->getJobId() == job.job_id;
      }

      bool operator!=(const JobEntry& jb) {
          return !(*this == jb);
      }
  };
 // TODO: Add your data members

 private:
    std::list<JobEntry*> *job_entry_list;

 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, bool isStopped = false, int pid = -1, int job_id = -1);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed

  friend bool sortJobEntries(JobEntry *job1, JobEntry *job2);

  JobEntry* getJobByPID(int job_pid);

  void removeJobByPID(int pid);

  bool isEmpty(){
      return job_entry_list->empty();
  }
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  // JobsCommand(const char* cmd_line, JobsList* jobs);
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  // KillCommand(const char* cmd_line, JobsList* jobs);
  KillCommand(const char* cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  // ForegroundCommand(const char* cmd_line, JobsList* jobs);
  ForegroundCommand(const char* cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  // BackgroundCommand(const char* cmd_line, JobsList* jobs);
  BackgroundCommand(const char* cmd_line);
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
  std::string prompt_name = "smash";
  int smashPID;
  int fg_job_pid = -1;
    int fg_job_id = -1;
  std::string plastPwd;
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

  void setPromptName(std::string new_name){
      this->prompt_name = new_name;
  }

  std::string getPromptName(){
      return this->prompt_name;
  }

  int getSmashId(){
      return this->smashPID;
  }

  int getFgJobPID(){
      return fg_job_pid; // update in 2 places: ExtenalCommed::execute, ForegroungCommand (fg/bg)
  }

  void setFgJobPID(int id){
      fg_job_pid = id;
  }

  int getFgJobID() const {
      return fg_job_id;
  }

  void setFgJobID(int id){
      fg_job_id = id;
  }

  JobsList* getJobList(){
      return job_list;
  }
  std::string getLastDir(){
      return this->plastPwd;
  }
  void setLastDir(std::string new_path){
      this->plastPwd = new_path;
  }
};

#endif //SMASH_COMMAND_H_
