#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

namespace {

struct DestroyWindow {
  void operator()(SDL_Window* w) const {
    SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "destroy window.");
    SDL_DestroyWindow(w);
  }
};
using WindowPtr = std::unique_ptr<SDL_Window, DestroyWindow>;

struct DestroyRenderer {
  void operator()(SDL_Renderer* r) const {
    SDL_LogDebug(SDL_LOG_CATEGORY_RENDER, "destroy renderer.");
    SDL_DestroyRenderer(r);
  }
};
using RendererPtr = std::unique_ptr<SDL_Renderer, DestroyRenderer>;

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

  if (IMG_Init(IMG_INIT_PNG) < 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "IMG_Init: %s", IMG_GetError());
    return EXIT_FAILURE;
  }
  atexit(IMG_Quit);

  if (TTF_Init() < 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "TTF_Init: %s", TTF_GetError());
    return EXIT_FAILURE;
  }
  atexit(TTF_Quit);

  if (SDLNet_Init() < 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDLNet_Init: %s",
                    SDLNet_GetError());
    return EXIT_FAILURE;
  }
  atexit(SDLNet_Quit);

  const char* TITLE = "Game";
  const int SCREEN_WIDTH = 16 * 60;
  const int SCREEN_HEIGH = 9 * 60;
  SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "create window. (%d x %d)", SCREEN_WIDTH,
               SCREEN_HEIGH);
  WindowPtr window(SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                    SCREEN_HEIGH, SDL_WINDOW_RESIZABLE));
  if (!window) {
    SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "SDL_CreateWindow: %s",
                    SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_LogDebug(SDL_LOG_CATEGORY_RENDER, "create renderer.");
  RendererPtr renderer(SDL_CreateRenderer(
      window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
  if (!renderer) {
    SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "SDL_CreateRenderer: %s",
                    SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_LogDebug(SDL_LOG_CATEGORY_SYSTEM, "initialize ImGui.");
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplSDL2_InitForSDLRenderer(window.get(), renderer.get());
  ImGui_ImplSDLRenderer2_Init(renderer.get());

  bool loop = true;
  while (loop) {
    {
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL2_ProcessEvent(&e);
        switch (e.type) {
          case SDL_QUIT:
            loop = false;
            break;
          case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_CLOSE &&
                e.window.windowID == SDL_GetWindowID(window.get())) {
              loop = false;
            }
            break;
        }
      }
    }

    int w, h;
    SDL_GetRendererOutputSize(renderer.get(), &w, &h);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Render();

    SDL_SetRenderDrawColor(renderer.get(), 0x12, 0x34, 0x56, 0xff);
    SDL_RenderClear(renderer.get());

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

    SDL_RenderPresent(renderer.get());
  }

  SDL_LogDebug(SDL_LOG_CATEGORY_SYSTEM, "shutdown ImGui.");
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  return EXIT_SUCCESS;
}
