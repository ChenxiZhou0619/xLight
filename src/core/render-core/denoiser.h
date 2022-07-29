#pragma once
#include "core/file/image.h"
#include "core/utils/configurable.h"

class Denoiser : public Configurable{
public:
    Denoiser() = default;
    virtual ~Denoiser() = default;
    virtual void denoise(const Image *image) const = 0;
};

class ConvolutionDenoiser : public Denoiser {
public:
    ConvolutionDenoiser() = default;
    
    ConvolutionDenoiser(const rapidjson::Value &_value) {
        m_kernel_radius = getInt("kernelRadius", _value);
    }
    
    virtual ~ConvolutionDenoiser() = default;
    

    virtual Eigen::MatrixXf getKernel(Vector2i pos, const Image *image) const = 0;

    // Denoise the image using a kernel
    virtual void denoise(const Image *image) const {
       
    }
protected:
    int m_kernel_radius = 4;
};