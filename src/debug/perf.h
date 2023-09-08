#pragma once

#include "t9/singleton.h"

namespace debug {

class Profiler : public t9::Singleton<Profiler> {
 public:
  struct TagData {
    enum class Type {
      BEGIN,
      END,
    } type;
    union {
      struct {
        Uint64 count;
        const char* name;
      } begin;
      struct {
        Uint64 count;
      } end;
    };
  };

  static constexpr std::size_t FRAME_N = 2;

  struct TimelineData {
    const char* name = nullptr;
    std::deque<TagData> tags;
  };

  struct FrameData {
    std::size_t frame_count = 0;
    Uint64 begin_count = 0;
    std::vector<TimelineData> timelines;
  };

  struct ThreadData {
    std::string name;
    TimelineData* timeline = nullptr;
  };

 private:
  std::array<std::unique_ptr<FrameData>, FRAME_N> frames_;
  std::unordered_map<SDL_threadID, ThreadData> threads_;
  std::size_t frame_index_ = 0;
  std::size_t frame_count_ = 0;
  bool is_swapped_ = false;

 public:
  void setup_for_this_thread(std::string_view name);
  void swap();
  void begin_tag(std::string_view name);
  void end_tag();
  void show_debug_gui();
};

class ScopedPerfTag {
 public:
  explicit ScopedPerfTag(std::string_view name) {
    Profiler::instance()->begin_tag(name);
  }
  ~ScopedPerfTag() {
    Profiler::instance()->end_tag();
  }
};

}  // namespace debug

#define PERF_SETUP(name) \
  debug::Profiler::instance()->setup_for_this_thread(name)
#define PERF_TAG_NAME_(name, line) tag##line(name)
#define PERF_TAG_NAME(name, line) PERF_TAG_NAME_(name, line)
#define PERF_TAG(name) debug::ScopedPerfTag PERF_TAG_NAME(name, __LINE__)
