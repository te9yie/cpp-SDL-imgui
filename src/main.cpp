#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "task/prelude.h"

namespace {

struct DestroyWindow {
  void operator()(SDL_Window* w) const {
    SDL_DestroyWindow(w);
  }
};
using WindowPtr = std::unique_ptr<SDL_Window, DestroyWindow>;

struct DestroyRenderer {
  void operator()(SDL_Renderer* r) const {
    SDL_DestroyRenderer(r);
  }
};
using RendererPtr = std::unique_ptr<SDL_Renderer, DestroyRenderer>;

class LogJob : public task::Job {
 private:
  int id_ = 0;
  task::JobSystem* jobs_ = nullptr;

 public:
  LogJob(int id) : Job("LogJob"), id_(id) {}
  LogJob(int id, task::JobSystem* jobs) : Job("LogJob"), id_(id), jobs_(jobs) {}

 protected:
  virtual void on_exec() override {
    SDL_Log("#%d: in %u", id_, SDL_GetThreadID(nullptr));
    SDL_Delay(1);
    if (jobs_) {
      for (int i = 0; i < 2; ++i) {
        auto job = std::make_shared<LogJob>(100 * id_ + i);
        add_child(job);
        jobs_->insert_job(job);
      }
    }
  }
};

struct Counter {
  int count = 0;
};

struct DebugGui;

}  // namespace

int main(int /*argc*/, char* /*argv*/[]) {
#if defined(_MSC_VER)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDL_Init: %s", SDL_GetError());
    return EXIT_FAILURE;
  }
  atexit(SDL_Quit);

  if (TTF_Init() < 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "TTF_Init: %s", TTF_GetError());
    return EXIT_FAILURE;
  }
  atexit(TTF_Quit);

  const char* TITLE = "Game";
  const int SCREEN_WIDTH = 16 * 60;
  const int SCREEN_HEIGH = 9 * 60;
  WindowPtr window(SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                    SCREEN_HEIGH, SDL_WINDOW_RESIZABLE));
  if (!window) {
    SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "SDL_CreateWindow: %s",
                    SDL_GetError());
    return EXIT_FAILURE;
  }

  RendererPtr renderer(SDL_CreateRenderer(
      window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
  if (!renderer) {
    SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "SDL_CreateRenderer: %s",
                    SDL_GetError());
    return EXIT_FAILURE;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui_ImplSDL2_InitForSDLRenderer(window.get(), renderer.get());
  ImGui_ImplSDLRenderer2_Init(renderer.get());

  task::JobSystem jobs;
  if (!jobs.init(2)) {
    return EXIT_FAILURE;
  }

  task::TaskSystem tasks;
  tasks.set_context(&jobs);

  tasks
      .add_task("handle events",
                [](DebugGui*, task::TaskSystemData* tasks) {
                  SDL_Event e;
                  while (SDL_PollEvent(&e)) {
                    ImGui_ImplSDL2_ProcessEvent(&e);
                    switch (e.type) {
                      case SDL_QUIT:
                        tasks->is_loop = false;
                        break;
                      default:
                        break;
                    }
                  }
                })
      ->set_exec_on_current_thread();

  tasks.add_task("counter 1", [](task::WorkRef<Counter> counter) {
    ImGui::InputInt("count 1", &counter->count);
  });
  tasks.add_task("counter 2", [](task::WorkRef<Counter> counter) {
    ImGui::InputInt("count 2", &counter->count);
  });
  tasks.add_task("add job", [](task::JobSystem* jobs) {
    if (ImGui::Button("Add Jobs")) {
      std::vector<std::shared_ptr<LogJob>> logs;
      for (int i = 0; i < 10; ++i) {
        logs.emplace_back(
            std::make_shared<LogJob>(i + 1, i == 1 ? jobs : nullptr));
      }
      logs[2]->add_prerequisite(logs[1]);
      logs[3]->add_prerequisite(logs[5]);
      logs[4]->add_prerequisite(logs[5]);
      logs[4]->add_prerequisite(logs[7]);
      for (auto it : logs) {
        jobs->add_job(std::move(it));
      }
    }
  });

  bool loop = true;
  while (loop) {
    {}

    /*
        int w, h;
        SDL_GetRendererOutputSize(renderer.get(), &w, &h);
        */

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Render();

    SDL_SetRenderDrawColor(renderer.get(), 0x12, 0x34, 0x56, 0xff);
    SDL_RenderClear(renderer.get());

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

    SDL_RenderPresent(renderer.get());
  }

  jobs.quit();

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
