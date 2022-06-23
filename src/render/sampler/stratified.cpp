#include "core/render-core/sampler.h"

class Stratified : public PixelSampler {
    void StratifiedSample1D(std::vector<float>& samples, int nSamples) {
        float invNSamples = 1.f / nSamples;
        // TODO: using a constant to replace it
        for (int i = 0; i < nSamples; ++i) {
            samples[i] = std::min((i + dist(rng)) * invNSamples, 0.9999999);
        }    
    }

    void StratifiedSample2D(std::vector<Point2f> &samples, int nx, int ny) {
        float dx = 1.f / nx,
              dy = 1.f / ny;
        for (int y = 0; y < ny; ++y) {
            for (int x = 0; x < nx; ++x) {
                float deltaX = dist(rng),
                      deltaY = dist(rng);
                samples[x + y * nx].x = std::min((x + deltaX) * dx, 0.9999999f);
                samples[x + y * nx].y = std::min((y + deltaY) * dy, 0.9999999f);
            }
        }
    }

    template<typename T>
    void shuffle(std::vector<T> &samples, int count) {
        for (int i = 0; i < count; ++i) {
            int other = i + std::min(int(dist(rng) * (count - i)), count - i - 1);
            std::swap(samples[i], samples[other]);
        }
    }


public: 
    Stratified() : PixelSampler() { }

    Stratified(int _xSamples, int _ySamples, int _nDimensions)
    :PixelSampler(_xSamples, _ySamples, _nDimensions) { }

    Stratified(const rapidjson::Value &_value) 
    :PixelSampler(_value) { }

    ~Stratified() = default;

    virtual void startPixel(const Point2f &p) override {
        // generate the stratified samples and shuffle
        for (int i = 0; i < nDimensions; ++i) {
            StratifiedSample1D(samples1D[i], samplesPerPixel);
            shuffle(samples1D[i], samplesPerPixel);
            StratifiedSample2D(samples2D[i], xSamples, ySamples);
            shuffle(samples2D[i], samplesPerPixel);
        }
        PixelSampler::startPixel(p);
    }

    virtual std::shared_ptr<Sampler> clone() const override {
        return std::make_shared<Stratified>(xSamples, ySamples, nDimensions);
    }

};

REGISTER_CLASS(Stratified, "stratified")

