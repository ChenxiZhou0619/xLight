#include "core/render-core/texture.h"

class CheckBoard : public Texture {
public:
  CheckBoard() : m_light(SpectrumRGB(0.9f)), m_dark(SpectrumRGB(0.4f)) {}

  CheckBoard(const rapidjson::Value &_value) {
    m_light = SpectrumRGB(0.9f);

    m_dark = SpectrumRGB(0.4f);
  }

  virtual SpectrumRGB evaluate(const Point2f &uv, float du = 0,
                               float dv = 0) const override {
    int u = static_cast<int>(uv.x / m_grid) % 2,
        v = static_cast<int>(uv.y / m_grid) % 2,
        u_ = static_cast<int>((uv.x + du) / m_grid) % 2,
        v_ = static_cast<int>((uv.y + dv) / m_grid) % 2;
    SpectrumRGB value{.0f};
    if ((u + v) % 2 == 1)
      value += m_light;
    else
      value += m_dark;
    if ((u_ + v) % 2 == 1)
      value += m_light;
    else
      value += m_dark;
    if ((u + v_) % 2 == 1)
      value += m_light;
    else
      value += m_dark;
    return value / 3;
  }

  virtual SpectrumRGB average() const override {
    // TODO, no implement
    return SpectrumRGB(.65f);
  }

  virtual Vector2i getResolution() const override {
    std::cout << "Error, Checkboard Texture shouldn't have resolution!\n";
    std::exit(1);
  }

  virtual SpectrumRGB dfdu(Point2f uv, float du, float dv) const override {
    std::cout << "Checkboard::dfdu not implemet!\n";
    std::exit(1);
  }

  virtual SpectrumRGB dfdv(Point2f uv, float du, float dv) const override {
    std::cout << "Checkboard::dfdv not implement!\n";
    std::exit(1);
  }

private:
  SpectrumRGB m_light;

  SpectrumRGB m_dark;

  float m_grid = 0.1; // default
};

REGISTER_CLASS(CheckBoard, "checkboard")