#include "perf.h"

#include "t9/color.h"
#include "t9/hash.h"

namespace debug {

bool Profiler::init() {
  t9::sdl2::MutexPtr mutex(SDL_CreateMutex());
  if (!mutex) {
    return false;
  }
  return true;
}

void Profiler::setup_for_this_thread(std::string_view name) {
  SDL_assert(!is_swapped_);
  auto id = SDL_ThreadID();
  SDL_Log("Setup thread: (%u)%s", id, name.data());
  {
    t9::sdl2::ScopedLock lock(mutex_.get());
    threads_.emplace(id, ThreadData{std::string(name), nullptr});
  }
}

void Profiler::swap() {
  if (!is_swapped_) {
    is_swapped_ = true;
  }

  auto frame = std::make_unique<FrameData>();
  frame->begin_count = SDL_GetPerformanceCounter();
  frame->frame_count = ++frame_count_;
  frame->timelines.reserve(threads_.size());
  for (auto& it : threads_) {
    auto& thread = it.second;
    auto& timeline = frame->timelines.emplace_back();
    timeline.name = thread.name.c_str();
    thread.timeline = &timeline;
  }

  frame_index_ = (frame_index_ + 1) % FRAME_N;
  frames_[frame_index_] = std::move(frame);
}

void Profiler::begin_tag(std::string_view name) {
  if (!is_swapped_) {
    return;
  }
  auto it = threads_.find(SDL_ThreadID());
  if (it == threads_.end()) {
    return;
  }

  SDL_assert(it->second.timeline);

  TagData tag;
  tag.type = TagData::Type::BEGIN;
  tag.begin.count = SDL_GetPerformanceCounter();
  tag.begin.name = name.data();

  auto& tags = it->second.timeline->tags;
  tags.emplace_back(std::move(tag));
}

void Profiler::end_tag() {
  if (!is_swapped_) {
    return;
  }
  auto it = threads_.find(SDL_ThreadID());
  if (it == threads_.end()) {
    return;
  }

  SDL_assert(it->second.timeline);

  TagData tag;
  tag.type = TagData::Type::END;
  tag.begin.count = SDL_GetPerformanceCounter();

  auto& tags = it->second.timeline->tags;
  tags.emplace_back(std::move(tag));
}

void Profiler::show_debug_gui() {
  auto frame_index = (frame_index_ + FRAME_N - 1) % FRAME_N;
  auto frame = frames_[frame_index].get();
  if (!frame) {
    return;
  }

  auto drawer = ImGui::GetWindowDrawList();

  for (auto& timeline : frame->timelines) {
    if (ImGui::CollapsingHeader(timeline.name)) {
      std::stack<Profiler::TagData> tags;

      auto offset = ImGui::GetCursorScreenPos();
      auto region = ImGui::GetContentRegionAvail();
      auto height = ImGui::GetTextLineHeightWithSpacing();

      ImGui::Dummy(ImVec2(region.x, height * 3));

      auto count2coord = [](Uint64 x) { return x / 1000.0f; };

      for (auto tag : timeline.tags) {
        if (tag.type == Profiler::TagData::Type::BEGIN) {
          tags.push(tag);
        } else if (tag.type == Profiler::TagData::Type::END) {
          if (tags.empty()) continue;
          auto& start_tag = tags.top();
          auto name_hash = t9::fxhash(start_tag.begin.name,
                                      std::strlen(start_tag.begin.name));
          auto begin = start_tag.begin.count;
          auto end = tag.end.count;
          tags.pop();

          auto depth = tags.size();
          auto h = 1.0f * name_hash / std::numeric_limits<std::uint32_t>::max();
          auto rgb = t9::hsv2rgb(h, 0.95f, 0.95f);
          auto p0 = ImVec2(offset.x + count2coord(begin - frame->begin_count),
                           offset.y + depth * height);
          auto p1 = ImVec2(offset.x + count2coord(end - frame->begin_count),
                           offset.y + (depth + 1) * height);
          drawer->AddRectFilled(p0, p1, ImColor(rgb.r, rgb.g, rgb.b));
          if (ImGui::IsMouseHoveringRect(p0, p1)) {
            ImGui::SetTooltip("%s", start_tag.begin.name);
          }
        }
      }
    }
  }
}

}  // namespace debug
