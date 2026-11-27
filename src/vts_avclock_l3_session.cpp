/**
 *  If not stated otherwise in this file or this component's LICENSE
 *  file the following copyright and licenses apply:
 *
 *  Copyright 2025 RDK Management
 *
 *  Licensed under the Apache License, Version 2.0 (the License);
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an AS IS BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <chrono>
#include <cmath>
#include <time.h>

#include "vts_avclock_l3_session.h"

#define SAMPLE_PERIOD_MS 100          /* ms - how often the sampler reads the clock */
#define RATE_SETTLE_TIME_MS 500       /* ms - ignored after the clock is primed before measuring the rate */
#define MIN_RATE_WINDOW_MS 2000       /* ms - shortest window used to measure the rate */
#define RATE_TOLERANCE 0.15           /* allowed difference between measured and expected rate */

/*
 * Runs the four steps. cleanup() always runs, so a failed setup or test still releases the clock.
 */
void AVClockSession::execute()
{
    UT_LOG_INFO("[AVClock %d] %s (%s) starting", mClockId.value, mScenario.name.c_str(),
                mSecure ? "secure" : "non-secure");
    if (setup())
    {
        runTest();
        validate();
    }
    cleanup();
}

/*
 * Opens the clock, sets the mode and rate (READY state only), starts the clock and checks it is
 * unprimed: no source runs yet, so getCurrentClockTime() must return null.
 */
bool AVClockSession::setup()
{
    AVCLOCK_OPEN_SUCCESS(mClock, mClockControlListener, mClockController, true);
    if (mClockController == nullptr)
    {
        return false;
    }
    mClockOpened = true;

    AVCLOCK_SET_CLOCK_MODE_SUCCESS(mClock, mClockController, mScenario.clockMode, true);

    if (mScenario.clockMode == avclock::ClockMode::PCR)
    {
        /* The playback rate cannot be used in PCR mode (IAVClockController.setPlaybackRate/getPlaybackRate) */
        AVCLOCK_SET_PLAYBACK_RATE_FAILURE_STATE_OR_MODE(mClock, mClockController, 1.0);
        AVCLOCK_GET_PLAYBACK_RATE_FAILURE_STATE_OR_MODE(mClock, mClockController);
    }
    else
    {
        AVCLOCK_SET_PLAYBACK_RATE_SUCCESS(mClock, mClockController, mScenario.playbackRate, true);
    }

    mClockControlListener->resetPrimedEvent();
    AVCLOCK_START_SUCCESS(mClock, mClockController, true);
    mClockStarted = true;

    std::optional<ClockTime> clockTime;
    AVCLOCK_GET_CURRENT_CLOCK_TIME_SUCCESS(mClock, mClockController, &clockTime);
    UT_ASSERT_FALSE(clockTime.has_value()) << "Clock is primed before any source was started";
    return true;
}

/*
 * Starts one thread per selected source and the sampler thread, waits for the sources to finish
 * and then stops the sampler.
 */
void AVClockSession::runTest()
{
    mStopSampler = false;
    mSamplerThread = std::thread(&AVClockSession::runSampler, this);

    if (mScenario.useAudio)
    {
        mAudioThread = std::thread(&AVClockSession::runAudioPath, this);
    }
    if (mScenario.useVideo)
    {
        mVideoThread = std::thread(&AVClockSession::runVideoPath, this);
    }
    if (mScenario.usePcr)
    {
        mPcrThread = std::thread(&AVClockSession::runPcrFeeder, this);
    }

    for (std::thread* source : {&mAudioThread, &mVideoThread, &mPcrThread})
    {
        if (source->joinable())
        {
            source->join();
        }
    }
    mSourcesEndNs = getMonotonicRawNs();

    mStopSampler = true;
    if (mSamplerThread.joinable())
    {
        mSamplerThread.join();
    }
}

/*
 * Checks the clock readings taken during the test:
 *  - onPrimed() came exactly once
 *  - once primed, the clock never returns to null and never goes backwards
 *  - the clock runs at the expected rate (requested rate, or 1.0 in PCR mode)
 */
void AVClockSession::validate()
{
    const size_t primedEventCount = mClockControlListener->getPrimedEventCount();
    UT_LOG_INFO("[AVClock %d] onPrimed received %zu time(s), %zu clock samples",
                mClockId.value, primedEventCount, mSamples.size());
    UT_ASSERT_EQUAL(primedEventCount, 1u) << "onPrimed must fire exactly once per started session";

    /* First primed sample */
    size_t firstPrimed = 0;
    while (firstPrimed < mSamples.size() && !mSamples[firstPrimed].primed)
    {
        firstPrimed++;
    }
    UT_ASSERT_TRUE(firstPrimed < mSamples.size()) << "Clock was never primed during the test";
    if (firstPrimed >= mSamples.size())
    {
        return;
    }

    /* Once primed: always primed and never going backwards */
    for (size_t i = firstPrimed + 1; i < mSamples.size(); i++)
    {
        UT_ASSERT_TRUE(mSamples[i].primed) << "Clock returned to unprimed after it was primed";
        if (mSamples[i].primed && mSamples[i - 1].primed)
        {
            UT_ASSERT_TRUE(mSamples[i].clockTimeNs >= mSamples[i - 1].clockTimeNs)
                << "Clock went backwards: " << mSamples[i - 1].clockTimeNs << " -> " << mSamples[i].clockTimeNs;
        }
    }

    /*
     * Playback rate: clock change / real time change, measured after the clock settled and only while
     * the sources were still running (a clock driven by a source may stop when the source ends).
     */
    const int64_t windowStartNs = mSamples[firstPrimed].monotonicRawNs + RATE_SETTLE_TIME_MS * 1000000LL;
    size_t first = firstPrimed;
    while (first < mSamples.size() && mSamples[first].monotonicRawNs < windowStartNs)
    {
        first++;
    }
    size_t last = first;
    for (size_t i = first; i < mSamples.size() && mSamples[i].monotonicRawNs <= mSourcesEndNs; i++)
    {
        last = i;
    }
    if (first >= mSamples.size() ||
        (mSamples[last].monotonicRawNs - mSamples[first].monotonicRawNs) < MIN_RATE_WINDOW_MS * 1000000LL)
    {
        UT_LOG_WARNING("[AVClock %d] Sources ran less than %d ms after priming; rate not checked",
                       mClockId.value, MIN_RATE_WINDOW_MS);
        return;
    }
    const double realTimeNs = static_cast<double>(mSamples[last].monotonicRawNs - mSamples[first].monotonicRawNs);
    const double clockTimeNs = static_cast<double>(mSamples[last].clockTimeNs - mSamples[first].clockTimeNs);
    const double measuredRate = clockTimeNs / realTimeNs;
    UT_LOG_INFO("[AVClock %d] Measured rate %.3f over %.1f s (expected %.2f)", mClockId.value,
                measuredRate, realTimeNs / 1e9, expectedRate());
    UT_ASSERT_TRUE(std::fabs(measuredRate - expectedRate()) < RATE_TOLERANCE)
        << "Clock rate " << measuredRate << " does not match the expected rate " << expectedRate();
}

/*
 * Releases everything setup() created. Safe after a partial setup: only what was opened/started
 * is stopped/closed.
 */
void AVClockSession::cleanup()
{
    mStopSampler = true;
    for (std::thread* worker : {&mAudioThread, &mVideoThread, &mPcrThread, &mSamplerThread})
    {
        if (worker->joinable())
        {
            worker->join();
        }
    }
    if (mClockStarted)
    {
        AVCLOCK_STOP_SUCCESS(mClock, mClockController, true);
        mClockStarted = false;
    }
    if (mClockOpened)
    {
        AVCLOCK_CLOSE_SUCCESS(mClock, mClockController, true);
        mClockOpened = false;
    }
    mClockController.clear();
}

/* In PCR mode the clock follows the PCR samples, which are sent at real time (rate 1.0) */
double AVClockSession::expectedRate() const
{
    return (mScenario.clockMode == avclock::ClockMode::PCR) ? 1.0 : mScenario.playbackRate;
}

/*
 * Audio path (same flow as the audio mixer test): the decoder test runs in its own thread and feeds
 * the sink test, which attaches the sink to this clock and plays the stream until it ends.
 */
void AVClockSession::runAudioPath()
{
    /* Failures are reported as test failures instead of escaping the worker thread */
    try
    {
        bool isSinkAttached = true;
        bool isClockInUse = true;       /* the session owns the clock; the sink only attaches to it */
        bool isMixerConnected = false;  /* no AudioMixer in the AV clock suite: no first-frame callbacks (0.23.0) */

        AudioDecoderTest decodeTest(mAudioPath.decoderId, mAudioPath.decoder, mAudioPath.decoderControlListener,
                                    mAudioPath.decoderEventListener, mAVBufferManager, mAVbufferHelper,
                                    mSecure, mAudioPath.streamConfig, isSinkAttached);
        std::thread decodeThread(&AudioDecoderTest::run, &decodeTest, false);

        AudioSinkTest sinkTest(mAudioPath.sinkId, mAudioPath.sink, mAudioPath.decoder,
                               mAudioPath.sinkControlListener, mAudioPath.sinkEventListener,
                               mAudioPath.decoderControlListener, mAVBufferManager, mAudioPath.decoderId,
                               mClockId, mSecure, mAudioPath.streamConfig, isClockInUse, isMixerConnected);
        sinkTest.run();

        /* Wait for the decoder to finish before the test objects go out of scope */
        if (decodeThread.joinable())
        {
            decodeThread.join();
        }
    }
    catch (const std::exception& ex)
    {
        UT_LOG_ERROR("Exception in runAudioPath: %s", ex.what());
        UT_ASSERT_TRUE(false);
    }
    catch (...)
    {
        UT_LOG_ERROR("Unknown exception in runAudioPath");
        UT_ASSERT_TRUE(false);
    }
}

/*
 * Video path: the decoder test runs in its own thread and feeds the sink test, which attaches the sink
 * to this clock and plays the stream until it ends. No video plane is mapped, so no first frame
 * rendered is expected (0.23.0).
 */
void AVClockSession::runVideoPath()
{
    /* Failures are reported as test failures instead of escaping the worker thread */
    try
    {
        bool isSinkAttached = true;
        bool isClockInUse = true;    /* the session owns the clock; the sink only attaches to it */
        bool isPlaneMapped = false;

        /* Clear decoder callbacks left from a previous session on the same decoder */
        mVideoPath.decoderControlListener->resetQueues();
        mVideoPath.decoderEventListener->resetQueues();

        videoDecoderTest decodeTest(mVideoPath.decoderId, mVideoPath.decoder, mVideoPath.decoderControlListener,
                                    mVideoPath.decoderEventListener, mAVBufferManager, mSecure,
                                    mVideoPath.streamConfig, mAVbufferHelper, isSinkAttached);
        std::thread decodeThread(&videoDecoderTest::run, &decodeTest);

        VideoSinkTest sinkTest(mVideoPath.sinkId, mVideoPath.sink, mVideoPath.decoder,
                               mVideoPath.sinkControlListener, mVideoPath.sinkEventListener,
                               mVideoPath.decoderControlListener, mAVBufferManager, mVideoPath.decoderId,
                               mClockId, mVideoPath.streamConfig, isClockInUse, isPlaneMapped);
        sinkTest.run();

        /* Wait for the decoder to finish, then close its session before the test objects go out of scope */
        if (decodeThread.joinable())
        {
            decodeThread.join();
        }
        decodeTest.closeDecoderSession();
    }
    catch (const std::exception& ex)
    {
        UT_LOG_ERROR("Exception in runVideoPath: %s", ex.what());
        UT_ASSERT_TRUE(false);
    }
    catch (...)
    {
        UT_LOG_ERROR("Unknown exception in runVideoPath");
        UT_ASSERT_TRUE(false);
    }
}

/*
 * PCR feeder: sends each PCR sample at its real-time offset from the first one, with the
 * CLOCK_MONOTONIC_RAW time it was "received" (IAVClockController.notifyPCRSample).
 */
void AVClockSession::runPcrFeeder()
{
    if (mPcrValues.empty())
    {
        UT_LOG_ERROR("[AVClock %d] No PCR samples configured", mClockId.value);
        UT_ASSERT_TRUE(false);
        return;
    }
    const int64_t firstPcrNs = mPcrValues.front();
    const auto startTime = std::chrono::steady_clock::now();
    for (const int64_t pcrNs : mPcrValues)
    {
        std::this_thread::sleep_until(startTime + std::chrono::nanoseconds(pcrNs - firstPcrNs));
        const int64_t sampleTimestampNs = getMonotonicRawNs();
        AVCLOCK_NOTIFY_PCR_SAMPLE_SUCCESS(mClock, mClockController, pcrNs, sampleTimestampNs, true);
    }
}

/*
 * Sampler: reads the clock every SAMPLE_PERIOD_MS until stopped. getCurrentClockTime() is called
 * directly (not through the logging macro) to keep the log small; one summary is logged in validate().
 */
void AVClockSession::runSampler()
{
    while (!mStopSampler)
    {
        std::optional<ClockTime> clockTime;
        Status status = mClockController->getCurrentClockTime(&clockTime);
        if (status.isOk())
        {
            AVClockSample sample;
            sample.monotonicRawNs = getMonotonicRawNs();
            sample.primed = clockTime.has_value();
            sample.clockTimeNs = sample.primed ? clockTime->clockTimeNs : IAVClock::UNDEFINED_TIME;
            UT_LOG_DEBUG("[AVClock %d] sample: primed=%d clockTimeNs=%lld", mClockId.value, sample.primed,
                         static_cast<long long>(sample.clockTimeNs));
            std::lock_guard<std::mutex> lock(mSampleMutex);
            mSamples.push_back(sample);
        }
        else
        {
            UT_LOG_ERROR("[AVClock %d] getCurrentClockTime failed in the sampler (exception %d)",
                         mClockId.value, status.exceptionCode());
            UT_ASSERT_TRUE(false);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(SAMPLE_PERIOD_MS));
    }
}

int64_t AVClockSession::getMonotonicRawNs()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    return static_cast<int64_t>(now.tv_sec) * 1000000000LL + now.tv_nsec;
}
