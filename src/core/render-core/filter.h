#pragma once
#include <core/utils/configurable.h>

class Filter : public Configurable {
public:
  int radius;

  Filter() = default;
  Filter(const rapidjson::Value &_value) { radius = getInt("radius", _value); }
  virtual ~Filter() = default;

  virtual float evaluate(Point2f offset) const = 0;
};
