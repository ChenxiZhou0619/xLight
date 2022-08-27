#include "core/render-core/medium.h"

class Homogeneous : public Medium {
public:
    Homogeneous() = default;
    Homogeneous(const rapidjson::Value &_value) {

    }
    virtual ~Homogeneous() = default;

    

};

REGISTER_CLASS(Homogeneous, "homegeneous")
