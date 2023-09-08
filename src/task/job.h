#pragma once

namespace task {

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
  SDL_atomic_t prerequisite_count_;
  std::vector<std::shared_ptr<Job>> prerequisites_;
  std::vector<Job*> dependencies_;
  SDL_atomic_t child_count_;
  Job* parent_ = nullptr;
  std::string name_;

 public:
  explicit Job(std::string_view name);
  virtual ~Job() = default;

  void reset();

  bool can_submit() const;
  void submit();

  bool can_exec() const;
  void exec();

  bool can_done() const;
  void done();

  bool add_prerequisite(const std::shared_ptr<Job>& job);
  bool add_child(const std::shared_ptr<Job>& job);

  State state() const {
    return state_;
  }

  const std::string& name() const {
    return name_;
  }

 private:
  void dec_prerequisite_count_();
  void dec_child_count_();
  bool is_prerequisite_(const std::shared_ptr<Job>& job) const;

 protected:
  virtual bool on_can_exec() const {
    return true;
  }
  virtual void on_exec() {}
};

}  // namespace task
