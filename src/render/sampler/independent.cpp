#include "core/render-core/sampler.h"
#include <random>

class Independent : public Sampler {
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dist;
public:
    Independent() : gen(rd()), dist(0.0, 1.0) { }

    Independent(const rapidjson::Value &_value) {
        // do nothing
    }

    ~Independent() { }

    virtual float next1D() {
        return dist(gen);
    }

    virtual Point2f next2D() {
        return Point2f(next1D(), next1D());    
    }
};

REGISTER_CLASS(Independent, "independent")