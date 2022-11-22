#pragma once
#include <rapidjson/document.h>

#include <functional>

#include "unordered_map"

class Configurable;
using Constructor = std::function<Configurable *(const rapidjson::Value &)>;

class ObjectFactory {
  static std::unordered_map<std::string, Constructor> *constructors;

 public:
  static void registerClass(const std::string &name,
                            const Constructor &constructor);
  static Configurable *createInstance(const std::string &name,
                                      const rapidjson::Value &_value);
};
