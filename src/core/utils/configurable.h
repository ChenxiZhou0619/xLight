#pragma once
#include "rapidjson/document.h"
#include "factory.h"
#include <unordered_map>
#include <functional>
#include "core/geometry/geometry.h"
#include "core/render-core/spectrum.h"

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

inline Point3f getPoint3f(const std::string &name, const rapidjson::Value &_value) {
    return Point3f {
        _value[name.c_str()].GetArray()[0].GetFloat(), 
        _value[name.c_str()].GetArray()[1].GetFloat(), 
        _value[name.c_str()].GetArray()[2].GetFloat()
    };
}

inline Vector3f getVector3f(const std::string &name, const rapidjson::Value &_value) {
    return Vector3f {
        _value[name.c_str()].GetArray()[0].GetFloat(), 
        _value[name.c_str()].GetArray()[1].GetFloat(), 
        _value[name.c_str()].GetArray()[2].GetFloat()
    };
}

inline SpectrumRGB getSpectrumRGB(const std::string &name, const rapidjson::Value &_value) {
    return SpectrumRGB {
        getVector3f(name, _value)
    };
}

inline float getFloat(const std::string &name, const rapidjson::Value &_value) {
    return _value[name.c_str()].GetFloat();
}

inline int getInt(const std::string &name, const rapidjson::Value &_value) {
    return _value[name.c_str()].GetInt();
}