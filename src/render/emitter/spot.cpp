#include "core/render-core/emitter.h"

class SpotEmitter : public Emitter {
public:
    SpotEmitter() = default;

    SpotEmitter(const rapidjson::Value &_value) {

    }

    virtual ~SpotEmitter() = default;

    virtual SpectrumRGB evaluate(const EmitterQueryRecord &eRec) const override
    {
        //* No implement
        std::cout << "No implement!\n";
        std::exit(1);
    }

    virtual SpectrumRGB evaluate(const Ray3f &ray) const override
    {
        //* No implement
        std::cout << "No implement!\n";
        std::exit(1);
    }

};
REGISTER_CLASS(SpotEmitter, "spot")