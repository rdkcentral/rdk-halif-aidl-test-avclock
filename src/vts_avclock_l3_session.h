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

#ifndef __VTS_AVCLOCK_L3_SESSION_H__
#define __VTS_AVCLOCK_L3_SESSION_H__

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "vts_audiodecoder.h"
#include "vts_audiodecoder_test.h"
#include "vts_videodecoder.h"
#include "vts_audiosink.h"
#include "vts_audiosink_test.h"
#include "vts_videosink.h"
#include "vts_videosink_test.h"
#include "vts_avclock.h"

/*
 * What one L3 AV clock test runs: the clock mode, which sources take part and the playback rate.
 * Every L3 test only fills this structure; the session does the rest.
 */
struct AVClockScenario
{
    std::string name;
    avclock::ClockMode clockMode = avclock::ClockMode::AUTO;
    bool useAudio = false;
    bool useVideo = false;
    bool usePcr = false;
    double playbackRate = 1.0;   /* not used in PCR mode (the clock follows the PCR samples) */
};

/* Resources and stream used by the audio path (decoder -> sink, sink attached to the clock) */
struct AVClockAudioPath
{
    IAudioDecoder::Id decoderId;
    sp<IAudioDecoder> decoder;
    sp<AudioDecoderControllerListener> decoderControlListener;
    sp<AudioDecoderEventListener> decoderEventListener;
    IAudioSink::Id sinkId;
    sp<IAudioSink> sink;
    sp<AudioSinkControllerListener> sinkControlListener;
    sp<AudioSinkEventListener> sinkEventListener;
    testConfiguration_t streamConfig;
};

/* Resources and stream used by the video path (decoder -> sink, sink attached to the clock) */
struct AVClockVideoPath
{
    IVideoDecoder::Id decoderId;
    sp<IVideoDecoder> decoder;
    sp<VideoDecoderControllerListener> decoderControlListener;
    sp<VideoDecoderEventListener> decoderEventListener;
    IVideoSink::Id sinkId;
    sp<IVideoSink> sink;
    sp<VideoSinkControllerListener> sinkControlListener;
    sp<VideoSinkEventListener> sinkEventListener;
    videoDecoderTestConfiguration_t streamConfig;
};

/* One reading of the clock taken by the sampler thread */
struct AVClockSample
{
    int64_t monotonicRawNs;  /* CLOCK_MONOTONIC_RAW when the sample was taken */
    bool primed;             /* false while getCurrentClockTime() returns null */
    int64_t clockTimeNs;     /* valid only when primed */
};

/*
 * One AV clock session: setup() -> runTest() -> validate() -> cleanup().
 * The session owns the clock (open, mode, rate, start/stop, close); the reused decoder and sink tests
 * only attach their sink to it (isClockInUse = true). Each selected source runs in its own thread and
 * one sampler thread reads the clock while they run.
 */
class AVClockSession
{
public:
    AVClockSession(const AVClockScenario& scenario, IAVClock::Id clockId, sp<IAVClock> clock,
                   sp<AVClockControllerListener> clockControlListener, sp<IAVBuffer> AVBufferManager,
                   IAVBufferHelper* avbufferHelper, bool secure)
        : mScenario(scenario),
          mClockId(clockId),
          mClock(clock),
          mClockControlListener(clockControlListener),
          mAVBufferManager(AVBufferManager),
          mAVbufferHelper(avbufferHelper),
          mSecure(secure)
    {
    }

    void setAudioPath(const AVClockAudioPath& audioPath) { mAudioPath = audioPath; }
    void setVideoPath(const AVClockVideoPath& videoPath) { mVideoPath = videoPath; }
    void setPcrValues(const std::vector<int64_t>& pcrValues) { mPcrValues = pcrValues; }
    void execute();

private:
    bool setup();
    void runTest();
    void validate();
    void cleanup();

    void runAudioPath();
    void runVideoPath();
    void runPcrFeeder();
    void runSampler();
    double expectedRate() const;
    static int64_t getMonotonicRawNs();

    AVClockScenario mScenario;
    IAVClock::Id mClockId;
    sp<IAVClock> mClock;
    sp<AVClockControllerListener> mClockControlListener;
    sp<IAVClockController> mClockController;
    sp<IAVBuffer> mAVBufferManager;
    IAVBufferHelper* mAVbufferHelper;
    bool mSecure;

    AVClockAudioPath mAudioPath;
    AVClockVideoPath mVideoPath;
    std::vector<int64_t> mPcrValues;

    std::thread mAudioThread;
    std::thread mVideoThread;
    std::thread mPcrThread;
    std::thread mSamplerThread;
    std::atomic<bool> mStopSampler{false};

    std::mutex mSampleMutex;
    std::vector<AVClockSample> mSamples;
    int64_t mSourcesEndNs = 0;  /* when the sources finished; the rate is checked only before it */

    bool mClockOpened = false;
    bool mClockStarted = false;
};

#endif // __VTS_AVCLOCK_L3_SESSION_H__
