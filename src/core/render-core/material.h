#pragma once
#include <optional>

#include "bsdf.h"
#include "bssrdf.h"
#include "medium.h"
#include "texture.h"
#include <core/utils/configurable.h>

class Material {
public:
  Material() = default;
  Material(const rapidjson::Value &_value) {}
  virtual ~Material() = default;

protected:
  std::shared_ptr<BSDF> bsdf;
  std::shared_ptr<BSSRDF> bssrdf;
};