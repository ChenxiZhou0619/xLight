#include <core/render-core/filter.h>

class BoxFilter : public Filter {
public:
  BoxFilter() = default;
  BoxFilter(const rapidjson::Value &_value) : Filter(_value) {}
  virtual ~BoxFilter() = default;

  virtual float evaluate(Point2f offset) const override { return 1; }
};

REGISTER_CLASS(BoxFilter, "box")

class TriangleFilter : public Filter {
public:
  TriangleFilter() = default;
  TriangleFilter(const rapidjson::Value &_value) : Filter(_value) {}
  virtual ~TriangleFilter() = default;

  virtual float evaluate(Point2f offset) const override {
    return (1 - std::abs(offset.x)) * (1 - std::abs(offset.y));
  }
};
REGISTER_CLASS(TriangleFilter, "triangle")