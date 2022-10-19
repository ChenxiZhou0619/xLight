#pragma once
#include <vector>
#include <random>
#include "core/utils/configurable.h"
#include "core/geometry/geometry.h"
#include <pcg/pcg_random.hpp>

struct CameraSample {
    Point2f sampleXY;
    float sampleT;
    Point2f sampleLens;
};


class Sampler : public Configurable {
protected:
    std::uniform_real_distribution<> dist;
    pcg_extras::seed_seq_from<std::random_device> seed_source;
    pcg32 rng;
public:
    Sampler() : dist(0.0, 1.0), rng(seed_source) {};

    Sampler(const rapidjson::Value &_value) { }

    ~Sampler() = default;

    
    virtual void startPixel(const Point2f &p) { };
    virtual float next1D() = 0;
    virtual Point2f next2D() = 0;
    virtual Point3f next3D() = 0;
    virtual void nextSample() = 0;

    CameraSample getCameraSample() {
        CameraSample sample;
        sample.sampleXY = next2D();
        sample.sampleT = next1D();
        sample.sampleLens = next2D();
        return sample;
    }

    virtual std::shared_ptr<Sampler> clone() const {
        return nullptr;
    }

};

class PixelSampler : public Sampler {
protected:
    std::vector<std::vector<float>> samples1D;
    std::vector<std::vector<Point2f>> samples2D;

    int currentDimension1D = 0;
    int currentDimension2D = 0;
    int currentSamplePixelIndex = 0;
    int xSamples, ySamples, nDimensions, samplesPerPixel;
public:
    // default 16spp and 4 dimensions(4 1D and 4 2D)
    PixelSampler() : xSamples(4), ySamples(4), nDimensions(4), samplesPerPixel(16) { };

    PixelSampler(int _xSamples, int _ySamples, int _nDimensions) 
    :xSamples(_xSamples), ySamples(_ySamples), nDimensions(_nDimensions), samplesPerPixel(_xSamples * _ySamples) {
        for (int i = 0; i < nDimensions; ++i) {
            samples1D.emplace_back(std::vector<float>(samplesPerPixel));
            samples2D.emplace_back(std::vector<Point2f>(samplesPerPixel));
        }
    }

    PixelSampler(const rapidjson::Value &_value) {
        xSamples = getInt("xSamples", _value);
        ySamples = getInt("ySamples", _value);
        nDimensions = getInt("nDimensions", _value);
        samplesPerPixel = xSamples * ySamples;

        for (int i = 0; i < nDimensions; ++i) {
            samples1D.emplace_back(std::vector<float>(samplesPerPixel));
            samples2D.emplace_back(std::vector<Point2f>(samplesPerPixel));
        }
    }

    ~PixelSampler() = default;
    virtual void startPixel(const Point2f &p) override{
        currentDimension1D = currentDimension2D = currentSamplePixelIndex = 0;
    };
    virtual void nextSample() override {
        currentDimension1D = currentDimension2D = 0;
        ++currentSamplePixelIndex;
    }
    virtual float next1D() override;
    virtual Point2f next2D() override;
    virtual Point3f next3D() override;
    virtual std::shared_ptr<Sampler> clone() const override{
        return std::make_shared<PixelSampler>(xSamples, ySamples, nDimensions);
    }

};
