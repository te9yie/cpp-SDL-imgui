#pragma once

namespace task {

class JobObserver {
 public:
  virtual ~JobObserver() = default;
  virtual void on_job_done() = 0;
};

class Job {
 public:
  enum class State {
    NONE,
    WAIT_EXEC,
    EXEC,
    WAIT_DONE,
    DONE,
  };

 private:
  State state_ = State::NONE;
  JobObserver* observer_ = nullptr;
  SDL_atomic_t prerequisite_count_;
  std::vector<std::shared_ptr<Job>> prerequisites_;
  std::vector<Job*> dependencies_;
  SDL_atomic_t child_count_;
  Job* parent_ = nullptr;

 public:
  virtual ~Job() = default;

  bool can_submit() const;
  void submit(JobObserver* observer);

  bool can_exec() const;
  void exec();

  bool can_done() const;
  void done();

  bool add_prerequisite(const std::shared_ptr<Job>& job);
  bool add_child(const std::shared_ptr<Job>& job);

  State state() const {
    return state_;
  }

 private:
  void dec_prerequisite_count_();
  void dec_child_count_();

 protected:
  virtual bool on_can_exec() const {
    return true;
  }
  virtual void on_exec() {}
};

struct DestroyMutex {
  void operator()(SDL_mutex* m) const {
    SDL_DestroyMutex(m);
  }
};
using MutexPtr = std::unique_ptr<SDL_mutex, DestroyMutex>;

struct DestroyCondition {
  void operator()(SDL_cond* c) const {
    SDL_DestroyCond(c);
  }
};
using ConditionPtr = std::unique_ptr<SDL_cond, DestroyCondition>;

class JobSystem : private JobObserver {
 private:
  JobSystem(const JobSystem&) = delete;
  JobSystem& operator=(const JobSystem&) = delete;

 private:
  std::vector<SDL_Thread*> threads_;
  MutexPtr mutex_;
  ConditionPtr condition_;
  SDL_atomic_t is_quit_;
  std::list<std::shared_ptr<Job>> jobs_;

 public:
  JobSystem();
  virtual ~JobSystem();

  bool init(std::size_t thread_count);
  void quit();

  bool add_job(std::shared_ptr<Job> job);
  bool insert_job(std::shared_ptr<Job> job);

 private:
  // JobObserver.
  virtual void on_job_done() override;

 private:
  void exec_jobs_();

 private:
  static int job_thread_(void* args);
};

}  // namespace task
