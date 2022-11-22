#include "configurable.h"

//* Initialize
std::map<std::string, std::shared_ptr<Configurable>> Params::ref_cache = {};

bool Params::has_name() const { return value.HasMember("name"); }

std::string Params::get_name() const {
  assert(has_name());
  return value["name"].GetString();
}

void Params::cache_ref(const std::string &ref_name,
                       std::shared_ptr<Configurable> ref) {
  if (ref_cache.count(ref_name) != 0) {
    std::cerr << "Reference : " << ref_name << " already exists!\n";
  }
  ref_cache[ref_name] = ref;
}

template <>
Point3f Params::fetch(const std::string &key, Point3f default_value) const {
  if (!value.HasMember(key.c_str())) {
    return default_value;
  }
  return {value[key.c_str()].GetArray()[0].GetFloat(),
          value[key.c_str()].GetArray()[1].GetFloat(),
          value[key.c_str()].GetArray()[2].GetFloat()};
}

template <>
Vector3f Params::fetch(const std::string &key, Vector3f default_value) const {
  if (!value.HasMember(key.c_str())) {
    return default_value;
  }
  return {value[key.c_str()].GetArray()[0].GetFloat(),
          value[key.c_str()].GetArray()[1].GetFloat(),
          value[key.c_str()].GetArray()[2].GetFloat()};
}

template <>
SpectrumRGB Params::fetch(const std::string &key,
                          SpectrumRGB default_value) const {
  if (!value.HasMember(key.c_str())) {
    return default_value;
  }
  return {value[key.c_str()].GetArray()[0].GetFloat(),
          value[key.c_str()].GetArray()[1].GetFloat(),
          value[key.c_str()].GetArray()[2].GetFloat()};
}

template <>
float Params::fetch(const std::string &key, float default_value) const {
  if (!value.HasMember(key.c_str())) return default_value;
  return value[key.c_str()].GetFloat();
}

template <>
int Params::fetch(const std::string &key, int default_value) const {
  if (!value.HasMember(key.c_str())) return default_value;
  return value[key.c_str()].GetInt();
}