#pragma once

namespace debug {

class Gui {
 private:
  bool is_init_ = false;

 public:
  virtual ~Gui();

  bool init(SDL_Window* w, SDL_Renderer* r);
  void quit();

  void handle_events(const SDL_Event* e);
  void new_frame();
  void render();
};

}  // namespace debug
