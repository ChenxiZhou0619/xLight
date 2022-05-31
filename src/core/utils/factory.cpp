#include "factory.h"
#include "configurable.h"
#include <iostream>

std::unordered_map<std::string, Constructor>* ObjectFactory::constructors = nullptr;

void ObjectFactory::registerClass(const std::string &name, const Constructor &constr)  {
    if (!constructors) {
        constructors = new std::unordered_map<std::string, Constructor>();
    }
    (*constructors)[name] = constr;
}

Configurable* ObjectFactory::createInstance(const std::string &name, const rapidjson::Value &_value) {
    if (!constructors || constructors->find(name) == constructors->end()) {
        std::cout << "Fatal : Unknown class! " << name << std::endl;
        exit(1);
    }
    return (*constructors)[name](_value);
}