#pragma once
#include <vector>

struct DiscretePDF {
    DiscretePDF() : DiscretePDF(0) {

    }

    explicit DiscretePDF(std::size_t nEntries) {
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

public:
    std::vector<float> mCdf;
    bool mNormalized;
    float mSum, mNormalization;
};