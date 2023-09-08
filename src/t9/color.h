#pragma once

#include <cstdint>

namespace t9 {

template <typename T>
struct RGB {
  T r;
  T g;
  T b;
};

inline RGB<float> hsv2rgb(float h, float s, float v) {
  if (s == 0) return RGB<float>{v, v, v};
  int h_i = static_cast<int>(h * 6);
  float f = h * 6 - h_i;
  float p = v * (1 - s);
  float q = v * (1 - (s * f));
  float t = v * (1 - (s * (1 - f)));
  // clang-format off
  switch (h_i) {
    case 0: return RGB<float>{v, t, p};
    case 1: return RGB<float>{q, v, p};
    case 2: return RGB<float>{p, v, t};
    case 3: return RGB<float>{p, q, v};
    case 4: return RGB<float>{t, p, v};
    case 5: return RGB<float>{v, p, q};
  }
  // clang-format on
  return RGB<float>{0, 0, 0};
}

}  // namespace t9
