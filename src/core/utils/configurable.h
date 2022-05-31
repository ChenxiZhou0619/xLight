#pragma once
#include "rapidjson/document.h"
#include "factory.h"
#include <unordered_map>
#include <functional>


class Configurable {
public:
    Configurable() = default;

    Configurable(const rapidjson::Value &_value) {}
    
    virtual ~Configurable() {}
};

#define REGISTER_CLASS(cls, name) \
    cls *cls ##_create(const rapidjson::Value &_value) { \
        return new cls(_value); \
    } \
    static struct cls ##_{ \
        cls ##_() { \
            ObjectFactory::registerClass(name, cls ##_create); \
        } \
    } cls ##__XLIGHT_;