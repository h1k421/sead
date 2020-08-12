#pragma once

#include <algorithm>
#include <type_traits>

#include <basis/seadNew.h>
#include <basis/seadRawPrint.h>
#include <basis/seadTypes.h>
#include <math/seadMathCalcCommon.h>
#include <prim/seadPtrUtil.h>

namespace sead
{
class Heap;

template <typename T>
class RingBuffer
{
public:
    RingBuffer() = default;
    RingBuffer(s32 capacity, T* buffer) : mBuffer(buffer), mCapacity(capacity) {}
    template <s32 N>
    explicit RingBuffer(T (&array)[N]) : RingBuffer(N, array)
    {
    }

    class iterator
    {
    public:
        explicit iterator(T* buffer, s32 index = 0) : mIndex(index), mBuffer(buffer) {}
        bool operator==(const iterator& rhs) const
        {
            return mIndex == rhs.mIndex && mBuffer == rhs.mBuffer;
        }
        bool operator!=(const iterator& rhs) const { return !operator==(rhs); }
        iterator& operator++()
        {
            ++mIndex;
            return *this;
        }
        T& operator*() const { return mBuffer[mIndex]; }
        T* operator->() const { return &mBuffer[mIndex]; }
        s32 getIndex() const { return mIndex; }

    private:
        s32 mIndex;
        T* mBuffer;
    };

    class constIterator
    {
    public:
        explicit constIterator(const T* buffer, s32 index = 0) : mIndex(index), mBuffer(buffer) {}
        bool operator==(const constIterator& rhs) const
        {
            return mIndex == rhs.mIndex && mBuffer == rhs.mBuffer;
        }
        bool operator!=(const constIterator& rhs) const { return !operator==(rhs); }
        constIterator& operator++()
        {
            ++mIndex;
            return *this;
        }
        const T& operator*() const { return mBuffer[mIndex]; }
        const T* operator->() const { return &mBuffer[mIndex]; }
        s32 getIndex() const { return mIndex; }

    private:
        s32 mIndex;
        const T* mBuffer;
    };

    iterator begin() { return iterator(mBuffer); }
    constIterator begin() const { return constIterator(mBuffer); }
    iterator end() { return iterator(mBuffer, mCapacity); }
    constIterator end() const { return constIterator(mBuffer, mCapacity); }

    void allocBuffer(s32 capacity, s32 alignment) { void(tryAllocBuffer(capacity, alignment)); }

    void allocBuffer(s32 capacity, Heap* heap, s32 alignment = sizeof(void*))
    {
        static_cast<void>(tryAllocBuffer(capacity, heap, alignment));
    }

    bool tryAllocBuffer(s32 capacity, s32 alignment = sizeof(void*))
    {
        SEAD_ASSERT(mBuffer == nullptr);
        if (capacity > 0)
        {
            T* buffer = new (alignment, std::nothrow) T[capacity];
            if (buffer)
            {
                mBuffer = buffer;
                mHead = mSize = 0;
                mCapacity = capacity;
                SEAD_ASSERT_MSG(PtrUtil::isAlignedPow2(mBuffer, sead::abs(alignment)),
                                "don't set alignment for a class with destructor");
                return true;
            }
            return false;
        }
        SEAD_ASSERT_MSG(false, "numMax[%d] must be larger than zero", capacity);
        return false;
    }

    bool tryAllocBuffer(s32 capacity, Heap* heap, s32 alignment = sizeof(void*))
    {
        SEAD_ASSERT(mBuffer == nullptr);
        if (capacity > 0)
        {
            T* buffer = new (heap, alignment, std::nothrow) T[capacity];
            if (buffer)
            {
                mBuffer = buffer;
                mHead = mSize = 0;
                mCapacity = capacity;
                SEAD_ASSERT_MSG(PtrUtil::isAlignedPow2(mBuffer, sead::abs(alignment)),
                                "don't set alignment for a class with destructor");
                return true;
            }
            return false;
        }
        SEAD_ASSERT_MSG(false, "numMax[%d] must be larger than zero", capacity);
        return false;
    }

    void allocBufferAssert(s32 size, Heap* heap, s32 alignment = sizeof(void*))
    {
        if (!tryAllocBuffer(size, heap, alignment))
            AllocFailAssert(heap, sizeof(T) * size, alignment);
    }

    void freeBuffer()
    {
        if (mBuffer)
        {
            delete[] mBuffer;
            mBuffer = nullptr;
            mCapacity = 0;
            mHead = 0;
            mSize = 0;
        }
    }

    void setBuffer(s32 capacity, T* bufferptr)
    {
        if (capacity < 1)
        {
            SEAD_ASSERT_MSG(false, "numMax[%d] must be larger than zero", capacity);
            return;
        }
        if (!bufferptr)
        {
            SEAD_ASSERT_MSG(false, "bufferptr is null");
            return;
        }
        mBuffer = bufferptr;
        mHead = mSize = 0;
        mCapacity = capacity;
    }

    bool isBufferReady() const { return mBuffer != nullptr; }

    T& operator[](s32 idx)
    {
        if (u32(mSize) <= u32(idx))
        {
            SEAD_ASSERT_MSG(false, "index exceeded [%d/%d/%d]", idx, mSize, mCapacity);
            return *unsafeGet(0);
        }
        return *unsafeGet(idx);
    }

    const T& operator[](s32 idx) const
    {
        if (u32(mSize) <= u32(idx))
        {
            SEAD_ASSERT_MSG(false, "index exceeded [%d/%d/%d]", idx, mSize, mCapacity);
            return *unsafeGet(0);
        }
        return *unsafeGet(idx);
    }

    T* get(s32 idx)
    {
        if (u32(mSize) <= u32(idx))
        {
            SEAD_ASSERT_MSG(false, "index exceeded [%d/%d/%d]", idx, mSize, mCapacity);
            return nullptr;
        }
        return unsafeGet(idx);
    }

    const T* get(s32 idx) const
    {
        if (u32(mSize) <= u32(idx))
        {
            SEAD_ASSERT_MSG(false, "index exceeded [%d/%d/%d]", idx, mSize, mCapacity);
            return nullptr;
        }

        return unsafeGet(idx);
    }

    T& operator()(s32 idx) { return *unsafeGet(idx); }
    const T& operator()(s32 idx) const { return *unsafeGet(idx); }

    T* unsafeGet(s32 idx)
    {
        s32 real_idx = mHead + idx;
        if (real_idx >= mCapacity)
            real_idx -= mCapacity;
        return &mBuffer[real_idx];
    }

    const T* unsafeGet(s32 idx) const
    {
        s32 real_idx = mHead + idx;
        if (real_idx >= mCapacity)
            real_idx -= mCapacity;
        return &mBuffer[real_idx];
    }

    T& front() { return *unsafeGet(0); }
    const T& front() const { return *unsafeGet(0); }

    T& back()
    {
        if (mSize < 1)
        {
            SEAD_ASSERT_MSG(false, "no element");
            return mBuffer[0];
        }
        return *unsafeGet(mSize - 1);
    }

    const T& back() const
    {
        if (mSize < 1)
        {
            SEAD_ASSERT_MSG(false, "no element");
            return mBuffer[0];
        }
        return *unsafeGet(mSize - 1);
    }

    s32 capacity() const { return mCapacity; }
    T* data() { return mBuffer; }
    const T* data() const { return mBuffer; }

    void pushBack(const T& item)
    {
        if (mSize == mCapacity)
        {
            mBuffer[mHead] = item;
            ++mHead;
        }
        else
        {
            *unsafeGet(mSize) = item;
            ++mSize;
        }

        if (mHead >= mCapacity)
            mHead -= mCapacity;
    }

    void pop() { --mSize; }

protected:
    T* mBuffer = nullptr;
    s32 mCapacity = 0;
    s32 mHead = 0;
    s32 mSize = 0;
};

template <typename T, s32 N>
class FixedRingBuffer : public RingBuffer<T>
{
public:
    FixedRingBuffer() : RingBuffer<T>(mData) {}

    void allocBuffer(s32 capacity, s32 alignment) = delete;
    void allocBuffer(s32 capacity, Heap* heap, s32 alignment) = delete;
    bool tryAllocBuffer(s32 capacity, s32 alignment) = delete;
    bool tryAllocBuffer(s32 capacity, Heap* heap, s32 alignment) = delete;
    void allocBufferAssert(s32 size, Heap* heap, s32 alignment) = delete;
    void freeBuffer() = delete;
    void setBuffer(s32 capacity, T* bufferptr) = delete;

private:
    T mData[N];
};
}  // namespace sead
