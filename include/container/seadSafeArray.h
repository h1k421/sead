#pragma once

#include <basis/seadRawPrint.h>
#include <basis/seadTypes.h>

namespace sead
{
/// A lightweight std::array like wrapper for a C style array.
template <typename T, s32 N>
class SafeArray
{
public:
    T mBuffer[N];

    T& operator[](s32 idx)
    {
        if (idx >= N)
        {
            SEAD_ASSERT(false, "range over [0, %d) : %d", N, idx);
            idx = 0;
        }
        return mBuffer[idx];
    }
    const T& operator[](s32 idx) const
    {
        if (idx >= N)
        {
            SEAD_ASSERT(false, "range over [0, %d) : %d", N, idx);
            idx = 0;
        }
        return mBuffer[idx];
    }

    T& operator()(s32 idx) { return mBuffer[idx]; }
    const T& operator()(s32 idx) const { return mBuffer[idx]; }

    T& front() { return mBuffer[0]; }
    const T& front() const { return mBuffer[0]; }
    T& back() { return mBuffer[N - 1]; }
    const T& back() const { return mBuffer[N - 1]; }

    int size() const { return N; }
    u32 getByteSize() const { return N * sizeof(T); }

    T* getBufferPtr() { return mBuffer; }
    const T* getBufferPtr() const { return mBuffer; }

    void fill(const T& value)
    {
        for (s32 i = 0; i < N; ++i)
            mBuffer[i] = value;
    }

    class iterator
    {
    public:
        iterator(T* buffer, s32 idx) : mBuffer(buffer), mIdx(idx) {}
        bool operator==(const iterator& rhs) const
        {
            return mBuffer == rhs.mBuffer && mIdx == rhs.mIdx;
        }
        bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
        iterator& operator++()
        {
            ++mIdx;
            return *this;
        }
        iterator& operator--()
        {
            --mIdx;
            return *this;
        }
        T* operator->() const { return &mBuffer[mIdx]; }
        T& operator*() const { return mBuffer[mIdx]; }

    private:
        T* mBuffer;
        s32 mIdx;
    };

    iterator begin() { return iterator(mBuffer, 0); }
    iterator end() { return iterator(mBuffer, N); }

    class constIterator
    {
    public:
        constIterator(const T* buffer, s32 idx) : mBuffer(buffer), mIdx(idx) {}
        bool operator==(const constIterator& rhs) const
        {
            return mBuffer == rhs.mBuffer && mIdx == rhs.mIdx;
        }
        bool operator!=(const constIterator& rhs) const { return !(*this == rhs); }
        constIterator& operator++()
        {
            ++mIdx;
            return *this;
        }
        constIterator& operator--()
        {
            --mIdx;
            return *this;
        }
        const T* operator->() const { return &mBuffer[mIdx]; }
        const T& operator*() const { return mBuffer[mIdx]; }

    private:
        const T* mBuffer;
        s32 mIdx;
    };

    constIterator constBegin() const { return constIterator(mBuffer, 0); }
    constIterator constEnd() const { return constIterator(mBuffer, N); }
};

}  // namespace sead