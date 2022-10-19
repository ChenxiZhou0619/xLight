#pragma once
#include <vector>
#include "core/geometry/geometry.h"

struct Distribution1D {
    Distribution1D() : Distribution1D(0) {

    }

    explicit Distribution1D(std::size_t nEntries) {
        mCdf.reserve(nEntries + 1);
        clear();
    }

    void clear() {
        mCdf.clear();
        mCdf.emplace_back(.0f);
        mNormalized = false;
    }

    void append(float pdfValue) {
        mCdf.emplace_back(
            mCdf.back() + pdfValue
        );
    }

    std::size_t size() const {
        return mCdf.size() - 1;
    }

    float operator[](std::size_t entry) const {
        return mCdf[entry + 1] - mCdf[entry];
    }

    bool isNormalized() const {
        return mNormalized;
    }

    float normalize() {
        mSum = mCdf.back();
        if (mSum > .0f) {
            mNormalization = 1.0f / mSum;
            for (std::size_t i = 0; i < mCdf.size(); ++i) {
                mCdf[i] *= mNormalization;
            }
            mCdf.back() = 1.f;
            mNormalized = true;
        } else {
            mNormalization = .0f;
        }
        return mSum;
    }

    std::size_t sample(float sampleValue) const {
        std::vector<float>::const_iterator entry = 
            std::lower_bound(mCdf.begin(), mCdf.end(), sampleValue);
        std::size_t index = (std::size_t) std::max((std::ptrdiff_t) 0, entry - mCdf.begin() - 1);
        return std::min(index, mCdf.size()-2);
    }

    std::size_t sample(float sampleValue, float *pdf) const {
        auto index = sample(sampleValue);
        *pdf = mCdf[index + 1] - mCdf[index];
        return index;
    }

    float pdf(int index) const {
        index = std::min((size_t)index, mCdf.size()-2);
        return mCdf[index + 1] - mCdf[index];
    }

public:
    std::vector<float> mCdf;
    bool mNormalized;
    float mSum, mNormalization;
};

struct Distribution2D {
    Distribution2D() = default;

    Distribution2D(const Distribution2D &rhs) = delete;

    Distribution2D& operator=(const Distribution2D &rhs) = delete;

    Distribution2D(int dim_x, int dim_y) : m_dim_x(dim_x), m_dim_y(dim_y), m_distribution_x(dim_x) {
        for (int i = 0; i < dim_x; ++i) {
            m_distribution_y.emplace_back(Distribution1D(dim_y));
        }
    }
    
    void appendAtX(int x, float pdf) {
        m_distribution_y[x].append(pdf);
    }

    float normalize() {
        for (int i = 0; i < m_distribution_y.size(); ++i) {
            m_distribution_x.append(m_distribution_y[i].normalize());
        }
        m_sum = m_distribution_x.normalize();
        m_normalized = true;
        return m_sum;
    }

    Vector2i sample(Point2f sample, float *pdf) const {
        float tmp_pdf;
        int x = m_distribution_x.sample(sample[0], &tmp_pdf);
        int y = m_distribution_y[x].sample(sample[1], pdf);
        *pdf *= tmp_pdf;
        return Vector2i{x, y};
    }

    float pdf(Vector2i position) const {
        int x = position[0],
            y = position[1];
        float pdf1 = m_distribution_x.pdf(x),
              pdf2 = m_distribution_y[x].pdf(y);
        return pdf1 * pdf2;
    }

public:
    bool m_normalized;
    
    float m_sum;

private:
    Distribution1D m_distribution_x;

    std::vector<Distribution1D> m_distribution_y;

    int m_dim_x, m_dim_y;
};
/**
 * @brief New 1d distribution object
 * 
 * @tparam Object 
 */
template<typename Object>
class Distrib1D {
public:
    Distrib1D() = default;

    Distrib1D(int nEntries) : distrib(nEntries) {
        objects.reserve(nEntries);
    }

    virtual ~Distrib1D() = default;

    void append(Object obj, float weight) {
        objects.emplace_back(obj);
        distrib.append(weight);
    }

    void postProcess() {
        distrib.normalize();
    }

    /**
     * @brief Receive a uniform random in [0, 1], return a sample result and its corresponding pdf
     * 
     * @param uniform 
     * @return std::pair<Object, float> 
     */
    std::pair<Object, float>
    sample(float uniform) const {
        float pdf = .0f;
        int idx = distrib.sample(uniform, &pdf);
        return {objects[idx], pdf};
    }

    /**
     * @brief Given an obj, return the pdf if it is sampled in sample routine
     * 
     * @param obj 
     * @return float 
     */
    float pdf(Object obj) const {
        if (auto itr = std::find(objects.begin(), objects.end(), obj);
            itr != objects.end())
        {
            return distrib.pdf(itr - objects.begin());
        }

        return 0.f;
    }

private:
    std::vector<Object> objects;
    Distribution1D distrib;
};