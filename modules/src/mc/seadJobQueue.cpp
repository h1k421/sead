#include <atomic>

#include "basis/seadRawPrint.h"
#include "framework/seadProcessMeter.h"
#include "mc/seadJobQueue.h"

namespace sead
{
SEAD_ENUM_IMPL(SyncType)

// NON_MATCHING
JobQueue::JobQueue()
{
    mCoreEnabled.fill(0);
    mNumDoneJobs = 0;
    mGranularity.fill(8);
}

bool JobQueue::run(u32 size, u32* finished_jobs, Worker* worker)
{
    *finished_jobs = 0;
    return true;
}

void JobQueue::runAll(u32* finished_jobs)
{
    const u32 size = getNumJobs();
    *finished_jobs = 0;
    while (true)
    {
        u32 finished_jobs_batch = 0;
        const bool ok = run(size, &finished_jobs_batch, nullptr);
        *finished_jobs += finished_jobs_batch;
        if (ok)
            break;
    }
    SEAD_ASSERT(*finished_jobs == size);
}

bool JobQueue::isAllParticipantThrough() const
{
    for (auto value : mCoreEnabled.mBuffer)
        if (value)
            return false;
    return true;
}

void JobQueue::setGranularity(CoreId core, u32 x)
{
    mGranularity[core] = x ? x : 1;
}

void JobQueue::setGranularity(u32 x)
{
    for (s32 i = 0; i < mGranularity.size(); ++i)
        setGranularity(i, x);
}

// NON_MATCHING: CMP (AND x y), #0 gets optimized into a TST
void JobQueue::setCoreMaskAndWaitType(CoreIdMask mask, SyncType type)
{
    mStatus = Status::_6;
    mMask = mask;
    for (u32 i = 0; i < CoreInfo::getNumCores(); ++i)
    {
        mCoreEnabled[i] = mask.isOn(i);
        mNumDoneJobs = 0;
    }
    mSyncType = type;
}

void JobQueue::FINISH(CoreId core)
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    mCoreEnabled[core] = 0;
    wait_AT_WORKER();
}

void JobQueue::wait_AT_WORKER()
{
    std::atomic_thread_fence(std::memory_order_seq_cst);

    switch (mSyncType)
    {
    case SyncType::cCore:
        if (!isDone_())
            mEvent.wait();
        break;
    case SyncType::cThread:
        SEAD_ASSERT_MSG(false, "*NOT YET\n");
        if (!isDone_())
            mEvent.wait();
        break;
    default:
        break;
    }
}

void JobQueue::wait()
{
    if (u32(mSyncType) >= 2)
    {
        if (mSyncType != SyncType::cThread)
            return;
        SEAD_ASSERT_MSG(false, "NOT IMPLEMENTED.\n");
    }
    if (!isDone_())
        mEvent.wait();
}

bool JobQueue::isDone_()
{
    return mNumDoneJobs == getNumJobs();
}

// NON_MATCHING: stack
void PerfJobQueue::initialize(const char* name, Heap* heap)
{
    mBars.allocBufferAssert(CoreInfo::getNumCores(), heap);
    mInts.allocBufferAssert(CoreInfo::getNumCores(), heap);

    for (s32 i = 0; i < mInts.size(); ++i)
        mInts[CoreId(i)] = 0;

    for (s32 i = 0; i < mBars.size(); ++i)
        mBars[i].setName(CoreId(i).text());

    mProcessMeterBar.setColor({1, 1, 0, 1});
    mProcessMeterBar.setName(name);
}

void PerfJobQueue::finalize()
{
    mInts.freeBuffer();
    mBars.freeBuffer();
}

void PerfJobQueue::reset()
{
    for (s32 i = 0; i < mInts.size(); ++i)
        mInts[CoreId(i)] = 0;
}

// NON_MATCHING: stack
void PerfJobQueue::measureBeginDeque()
{
    auto& bar = mBars[CoreInfo::getCurrentCoreId()];
    static_cast<void>(mInts[CoreInfo::getCurrentCoreId()]);
    bar.measureBegin(Color4f::cWhite);
}

void PerfJobQueue::measureEndDeque()
{
    mBars[CoreInfo::getCurrentCoreId()].measureEnd();
}

void PerfJobQueue::measureBeginRun()
{
    auto& bar = mBars[CoreInfo::getCurrentCoreId()];
    auto& idx = mInts[CoreInfo::getCurrentCoreId()];
    bar.measureBegin(getBarColor(idx));
    idx = (idx + 1) % 9;
}

void PerfJobQueue::measureEndRun()
{
    mBars[CoreInfo::getCurrentCoreId()].measureEnd();
}

// NON_MATCHING: loading sColors...
const Color4f& PerfJobQueue::getBarColor(u32 idx) const
{
    static const SafeArray<Color4f, 9> sColors = {{
        {0.2078431397676468, 0.8313725590705872, 0.6274510025978088, 1.0},
        {0.0, 0.6666666865348816, 0.4470588266849518, 1.0},
        {0.125490203499794, 0.49803921580314636, 0.3764705955982208, 1.0},
        {0.7490196228027344, 0.5254902243614197, 0.1882352977991104, 1.0},
        {1.0, 0.6000000238418579, 0.0, 1.0},
        {1.0, 0.6980392336845398, 0.250980406999588, 1.0},
        {0.6901960968971252, 0.1725490242242813, 0.29411765933036804, 1.0},
        {0.0, 0.9176470637321472, 0.21568627655506134, 1.0},
        {0.9607843160629272, 0.239215686917305, 0.40784314274787903, 1.0},
    }};
    return sColors.mBuffer[idx];
}

void PerfJobQueue::attachProcessMeter()
{
    if (!ProcessMeter::instance())
        return;

    for (s32 i = 0; i < mBars.size(); ++i)
        ProcessMeter::instance()->attachProcessMeterBar(&mBars[i]);

    ProcessMeter::instance()->attachProcessMeterBar(&mProcessMeterBar);
}

void PerfJobQueue::detachProcessMeter()
{
    if (!ProcessMeter::instance())
        return;

    for (s32 i = 0; i < mBars.size(); ++i)
        ProcessMeter::instance()->detachProcessMeterBar(&mBars[i]);

    ProcessMeter::instance()->detachProcessMeterBar(&mProcessMeterBar);
}
}  // namespace sead