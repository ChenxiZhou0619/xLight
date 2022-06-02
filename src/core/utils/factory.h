#pragma once
#include <functional>
#include <rapidjson/document.h>
#include "unordered_map"

class Configurable;
using Constructor = std::function<Configurable *(const rapidjson::Value&)>;

class ObjectFactory {
    static std::unordered_map<std::string, Constructor> *constructors;
public:
    static void registerClass(const std::string &name, const Constructor &constructor);
    static Configurable *createInstance(const std::string &name, const rapidjson::Value &_value);
};




