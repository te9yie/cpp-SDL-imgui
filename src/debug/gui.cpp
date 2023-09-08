#include "gui.h"

#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

namespace debug {

/*virtual*/ Gui::~Gui() {
  quit();
}

bool Gui::init(SDL_Window* w, SDL_Renderer* r) {
  if (is_init_) {
    return false;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui_ImplSDL2_InitForSDLRenderer(w, r);
  ImGui_ImplSDLRenderer2_Init(r);

  is_init_ = true;

  return true;
}

void Gui::quit() {
  if (!is_init_) {
    return;
  }

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  is_init_ = false;
}

void Gui::handle_events(const SDL_Event* e) {
  ImGui_ImplSDL2_ProcessEvent(e);
}

void Gui::new_frame() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}

void Gui::render() {
  ImGui::Render();
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}

}  // namespace debug
