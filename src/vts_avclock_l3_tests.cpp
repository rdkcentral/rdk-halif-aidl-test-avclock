/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup HPK Hardware Porting Kit
 * @{
 *
 */

/**
 * @addtogroup AVClock AVClock Module
 * @{
 *
 */

/**
 * @defgroup AVCLOCK_HALTEST AVClock HAL Tests
 * @{
 *
 */

/**
 * @defgroup AVCLOCK_HALTEST_L3 AVClock HAL Tests L3 File
 * @{
 * @parblock
 *
 * ### L3 Test Cases for AVClock HAL :
 *
 * This module includes Level 3 functional tests (success and failure
 * scenarios). This is to ensure that the API meets the operational requirements
 * of the module across all vendors.
 *
 * **Pre-Conditions:** @n
 * **Dependencies:** None@n
 *
 * @endparblock
 *
 */

/**
 * @file vts_avclock_l3_tests.cpp
 *
 */

#include <algorithm>
#include <atomic>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/Status.h>
#include <chrono>
#include <condition_variable>

/* Wait slice used by the prime phase barrier (waits until all stream threads arrive) */
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <unordered_set>
#include <ut.h>
#include <ut_control_plane.h>
#include <ut_kvp_profile.h>
#include <ut_log.h>
#include <utils/String16.h>
#include <vector>

#include <map>
#include <memory>

#include "com/rdk/hal/avbuffer/BnAVBufferSpaceListener.h"
#include "com/rdk/hal/avbuffer/IAVBuffer.h"
#include "com/rdk/hal/audiosink/IAudioSink.h"

#include "vts_audiodecoder.h"
#include "vts_audiodecoder_parse_testconfig.h"
#include "vts_audiodecoder_test.h"
#include "vts_audiosink_parse_hfp.h"
#include "vts_videodecoder.h"
#include "vts_audiosink.h"
#include "vts_audiosink_test.h"

#include "vts_audiosink_helper_functions.h"
#include "vts_avbuffer.h"

#include "vts_videosink_helper_functions.h"
#include "vts_videosink_parse_hfp.h"
#include "vts_videosink.h"
#include "vts_videosink_test.h"


#include "vts_avclock_parse_hfp.h"
#include "vts_avclock_parse_testconfig.h"

#include "vts_avclock.h"
#include "vts_avclock_l3_session.h"

extern const char *gAudioModuleName;

static const char *gTestGroup = "L3";

// static IAVBufferHelper* gAVbufferHelper;
static sp<IAVBuffer> vts_avbuffer_getService() {
  return android::interface_cast<IAVBuffer>(
      android::defaultServiceManager()->getService(
          android::String16(IAVBuffer::serviceName().c_str())));
}

class VTS_L3_AVClock : public UTCore {
public:
  static std::vector<
      std::tuple<IAVClock::Id, sp<IAVClock>, sp<AVClockControllerListener>,
                 sp<AVClockEventListener>>>
      mAVClockList;
  static std::vector<std::tuple<
      IAudioDecoder::Id, sp<IAudioDecoder>, sp<AudioDecoderControllerListener>,
      sp<AudioDecoderEventListener>, audiodecoder::Capabilities *>>
      mAudioDecoderList;
  static std::vector<std::tuple<
      IVideoDecoder::Id, sp<IVideoDecoder>, sp<VideoDecoderControllerListener>,
      sp<VideoDecoderEventListener>, videodecoder::Capabilities *>>
      mVideoDecoderList;
  static std::vector<
      std::tuple<IAudioSink::Id, sp<IAudioSink>,
                 sp<AudioSinkControllerListener>, sp<AudioSinkEventListener>>>
      mAudioSinkList;
  static std::vector<
      std::tuple<IVideoSink::Id, sp<IVideoSink>,
                 sp<VideoSinkControllerListener>, sp<VideoSinkEventListener>>>
      mVideoSinkList;
  static sp<IAVBuffer> mAVBufferManager;
  static IAVBufferHelper *mAVbufferHelper;

  VTS_L3_AVClock() : UTCore(){};

  ~VTS_L3_AVClock() override = default;

  void SetUp() override {
    std::vector<IAVClock::Id> avClockIds;
    std::vector<IAudioSink::Id> audioSinkIds;
    std::vector<IVideoSink::Id> videoSinkIds;
    std::vector<IAudioDecoder::Id> audioDecoderIds;
    std::vector<IVideoDecoder::Id> videoDecoderIds;

    if (gAVClockManagerService == NULL) {
      UT_FAIL_FATAL("AVClock Service not available");
    }

    if (gAudioDecoderManager == NULL) {
      UT_FAIL_FATAL("AudioDecoder Service not available");
    }

    if (gAudioSinkManager == NULL) {
      UT_FAIL_FATAL("AudioSink Service not available");
    }

    if (gVideoDecoderManager == NULL) {
      UT_FAIL_FATAL("VideoDecoder Service not available");
    }

    if (gVideoSinkManager == NULL) {
      UT_FAIL_FATAL("VideoSink Service not available");
    }

    mAVBufferManager = vts_avbuffer_getService();
    if (mAVBufferManager == NULL) {
      UT_FAIL_FATAL("AVBufferManager Service not available");
    }

    mAVbufferHelper = getAVBufferHelperInstance();
    if (mAVbufferHelper == nullptr) {
      UT_FAIL_FATAL("AVBuffer Helper not available");
    }

    /* Get the AVClock IDs */
    AVCLOCK_MANAGER_GETAVCLOCKIDS(avClockIds);

    /* Get the Audio Sink IDs */
    AUDIOSINK_MANAGER_GETAUDIOSINKIDS(audioSinkIds);

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(audioDecoderIds);

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(videoDecoderIds);

    /* Get the Video Sink IDs */
    VIDEOSINK_MANAGER_GETVIDEOSINKIDS(videoSinkIds);

    android::ProcessState::self()->startThreadPool();

    for (const auto &_avClockId : avClockIds) {
      sp<IAVClock> avClock;

      sp<AVClockEventListener> eventListener =
          new AVClockEventListener(_avClockId.value);

      sp<AVClockControllerListener> controlListener =
          new AVClockControllerListener(_avClockId.value);

      /* Get AVClock handle */
      AVCLOCK_MANAGER_GETAVCLOCK_SUCCESS(_avClockId, avClock);

      /* Register events */
      AVCLOCK_REGISTER_EVENT_LISTENER_SUCCESS(avClock, eventListener);

      /* Get Capabilities of AVClock */
      // TODO: Uncomment when capabilities are implemented in the HAL
      // AVCLOCK_GETCAPABILITIES(avClock, capabilities);

      mAVClockList.push_back(
          {_avClockId, std::move(avClock), std::move(controlListener), std::move(eventListener)});
    }

    for (const auto &_audioSinkId : audioSinkIds) {
      sp<IAudioSink> audioSink;

      sp<AudioSinkEventListener> eventListener =
          new AudioSinkEventListener(_audioSinkId.value);

      sp<AudioSinkControllerListener> controlListener =
          new AudioSinkControllerListener(_audioSinkId.value);

      /* Get Audio decoder handle */
      AUDIOSINK_MANAGER_GETAUDIOSINK(_audioSinkId, audioSink);

      /* Register events */
      AUDIOSINK_REGISTER_EVENT_SUCCESS(audioSink, eventListener);

      /* Get Capabilities of Audio Sink */
      // TODO: Uncomment when capabilities are implemented in the HAL
      // AUDIOSINK_GETCAPABILITIES(audioSink, capabilities);

      mAudioSinkList.push_back(
          {_audioSinkId, std::move(audioSink), std::move(controlListener), std::move(eventListener)});
    }

    for (const auto &_audioDecoderId : audioDecoderIds) {
      sp<IAudioDecoder> audioDecoder;

      std::unique_ptr<audiodecoder::Capabilities> capabilitiesOwner(new audiodecoder::Capabilities());
      audiodecoder::Capabilities *capabilities = capabilitiesOwner.get();

      sp<AudioDecoderEventListener> eventListener =
          new AudioDecoderEventListener();

      sp<AudioDecoderControllerListener> controlListener =
          new AudioDecoderControllerListener();

      controlListener->setDecoderID(_audioDecoderId.value);
      eventListener->setDecoderID(_audioDecoderId.value);

      /* Get Audio decoder handle */
      AUDIODECODER_MANAGER_GETAUDIODECODER(_audioDecoderId, audioDecoder);

      /* Register events */
      AUDIODECODER_REGISTER_EVENT_SUCCESS(audioDecoder, eventListener);

      /* Get Capabilities of Audio Decoder */
      AUDIODECODER_GETCAPABILITIES(audioDecoder, capabilities);
      UT_LOG_DEBUG(
          "Result getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[0x%X]), "
          "IN:audioDeccontrollerListener[%p], audioDecEventListener[%p] ",
          _audioDecoderId.value, audioDecoder.get(), controlListener.get(),
          eventListener.get());

      mAudioDecoderList.push_back({_audioDecoderId, std::move(audioDecoder),
                                   std::move(controlListener), std::move(eventListener),
                                   capabilitiesOwner.release()});
    }

    for (const auto &_videoDecoderId : videoDecoderIds) {
      sp<IVideoDecoder> videoDecoder;
      std::unique_ptr<videodecoder::Capabilities> capabilitiesOwner(new videodecoder::Capabilities());
      videodecoder::Capabilities *capabilities = capabilitiesOwner.get();
      sp<VideoDecoderEventListener> eventListener =
          new VideoDecoderEventListener();
      sp<VideoDecoderControllerListener> controlListener =
          new VideoDecoderControllerListener();

      VIDEODECODER_MANAGER_GETVIDEODECODER(_videoDecoderId, videoDecoder);

      VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, eventListener);

      VIDEODECODER_GETCAPABILITIES(videoDecoder, capabilities);

      mVideoDecoderList.push_back({_videoDecoderId, std::move(videoDecoder),
                                   std::move(controlListener), std::move(eventListener),
                                   capabilitiesOwner.release()});
      UT_LOG_INFO("Added decoder ID %d to test list", _videoDecoderId.value);
    }

    for (const auto &_videoSinkId : videoSinkIds) {
      sp<IVideoSink> videoSink;

      sp<VideoSinkEventListener> videoSinkEventListener =
          new VideoSinkEventListener();

      sp<VideoSinkControllerListener> videoSinkControlListener =
          new VideoSinkControllerListener();

      videoSinkControlListener->setSinkID(_videoSinkId.value);
      videoSinkEventListener->setSinkID(_videoSinkId.value);

      /* Get Video Sink handle */
      VIDEOSINK_MANAGER_GETVIDEOSINK(_videoSinkId, videoSink);

      /* Register events */
      VIDEOSINK_REGISTER_EVENT_SUCCESS(videoSink, videoSinkEventListener);

      // TODO: VideoSink Get capabilities are yet to implement in the video sink

      mVideoSinkList.push_back({_videoSinkId, std::move(videoSink),
                                std::move(videoSinkControlListener),
                                std::move(videoSinkEventListener)});
    }
  }

  void TearDown() override {
    mAVClockList.clear();
    mAudioDecoderList.clear();
    mVideoDecoderList.clear();
    mAudioSinkList.clear();
    mVideoSinkList.clear();

    /* rdk-halif-aidl 0.23.0: release the AV Buffer helper instance taken in SetUp() (null if SetUp() failed early) */
    if (mAVbufferHelper != nullptr) {
      releaseAVBufferHelperInstance(mAVbufferHelper);
      mAVbufferHelper = nullptr;
    }
    // TODO: check right place to call joinThreadPool
    // android::IPCThreadState::self()->joinThreadPool();
  }
};

std::vector<std::tuple<IAVClock::Id, sp<IAVClock>,
                       sp<AVClockControllerListener>, sp<AVClockEventListener>>>
    VTS_L3_AVClock::mAVClockList;
std::vector<std::tuple<
    IAudioDecoder::Id, sp<IAudioDecoder>, sp<AudioDecoderControllerListener>,
    sp<AudioDecoderEventListener>, audiodecoder::Capabilities *>>
    VTS_L3_AVClock::mAudioDecoderList;
std::vector<std::tuple<
    IVideoDecoder::Id, sp<IVideoDecoder>, sp<VideoDecoderControllerListener>,
    sp<VideoDecoderEventListener>, videodecoder::Capabilities *>>
    VTS_L3_AVClock::mVideoDecoderList;
std::vector<
    std::tuple<IAudioSink::Id, sp<IAudioSink>, sp<AudioSinkControllerListener>,
               sp<AudioSinkEventListener>>>
    VTS_L3_AVClock::mAudioSinkList;
std::vector<
    std::tuple<IVideoSink::Id, sp<IVideoSink>, sp<VideoSinkControllerListener>,
               sp<VideoSinkEventListener>>>
    VTS_L3_AVClock::mVideoSinkList;
sp<IAVBuffer> VTS_L3_AVClock::mAVBufferManager;
IAVBufferHelper *VTS_L3_AVClock::mAVbufferHelper;

/* Test class registration with UT_Core */
UT_ADD_TEST_TO_GROUP(VTS_L3_AVClock, UT_TESTS_L3)

/*
 * New L3 flow (AVClockSession: setup -> runTest -> validate -> cleanup).
 */

/*
 * Audio resources for one session: the index-th decoder that supports the stream codec (and secure
 * decode when required) and the index-th sink. Concurrent sessions use different indexes, so they
 * never share a decoder or sink.
 */
static bool findAudioPath(const testConfiguration_t& stream, bool secure, size_t index, AVClockAudioPath& path)
{
    auto& audioDecoderList = VTS_L3_AVClock::mAudioDecoderList;
    auto& audioSinkList = VTS_L3_AVClock::mAudioSinkList;
    if (index >= audioSinkList.size())
        return false;

    size_t matchingDecoders = 0;
    for (const auto& [decoderId, decoder, decoderControlListener, decoderEventListener,
                      decoderCapabilities] : audioDecoderList)
    {
        if (decoderCapabilities == nullptr)
            continue;
        if (secure && !decoderCapabilities->supportsSecure)
            continue;
        if (std::find(decoderCapabilities->supportedCodecs.begin(), decoderCapabilities->supportedCodecs.end(),
                      stream.codec) == decoderCapabilities->supportedCodecs.end())
            continue;
        if (matchingDecoders++ < index)
            continue;

        const auto& [sinkId, sink, sinkControlListener, sinkEventListener] = audioSinkList[index];
        path.decoderId = decoderId;
        path.decoder = decoder;
        path.decoderControlListener = decoderControlListener;
        path.decoderEventListener = decoderEventListener;
        path.sinkId = sinkId;
        path.sink = sink;
        path.sinkControlListener = sinkControlListener;
        path.sinkEventListener = sinkEventListener;
        path.streamConfig = stream;
        return true;
    }
    return false;
}

/* Video resources for one session, chosen the same way as the audio ones */
static bool findVideoPath(const videoDecoderTestConfiguration_t& stream, bool secure, size_t index,
                          AVClockVideoPath& path)
{
    auto& videoDecoderList = VTS_L3_AVClock::mVideoDecoderList;
    auto& videoSinkList = VTS_L3_AVClock::mVideoSinkList;
    if (index >= videoSinkList.size())
        return false;

    size_t matchingDecoders = 0;
    for (const auto& [decoderId, decoder, decoderControlListener, decoderEventListener,
                      decoderCapabilities] : videoDecoderList)
    {
        if (decoderCapabilities == nullptr)
            continue;
        if (secure && !decoderCapabilities->supportsSecure)
            continue;
        const auto& codecs = decoderCapabilities->supportedCodecs;
        if (std::find_if(codecs.begin(), codecs.end(),
                         [&stream](const auto& codec) { return codec.codec == stream.codec; }) == codecs.end())
            continue;
        if (matchingDecoders++ < index)
            continue;

        const auto& [sinkId, sink, sinkControlListener, sinkEventListener] = videoSinkList[index];
        path.decoderId = decoderId;
        path.decoder = decoder;
        path.decoderControlListener = decoderControlListener;
        path.decoderEventListener = decoderEventListener;
        path.sinkId = sinkId;
        path.sink = sink;
        path.sinkControlListener = sinkControlListener;
        path.sinkEventListener = sinkEventListener;
        path.streamConfig = stream;
        return true;
    }
    return false;
}

/*
 * Runs one scenario like the audio mixer test: non-secure and secure (secure is skipped when no
 * decoder supports it). Sequential runs one clock instance at a time; concurrent runs all clock
 * instances at the same time, each with its own decoders and sinks.
 * One run per stream; with audio and video, audio stream i plays together with video stream i.
 */
static void runAVClockScenario(const AVClockScenario& scenario, bool concurrent)
{
    auto& clockList = VTS_L3_AVClock::mAVClockList;

    vector<testConfiguration_t> audioStreams;
    vector<videoDecoderTestConfiguration_t> videoStreams;
    std::vector<int64_t> pcrValues;
    if (scenario.useAudio)
    {
        vts_audiodecoder_parse_testconfig(gAudioPrimeTestConfiguration, gAudioModuleName, gTestGroup,
                                          "PrimeAudio", audioStreams);
    }
    if (scenario.useVideo)
    {
        vts_videodecoder_parse_testconfig(gVideoPrimeTestConfiguration, VIDEOSINKMGR, gTestGroup,
                                          "PrimeVideo", videoStreams);
    }
    if (scenario.usePcr)
    {
        vts_avclock_parse_testconfig(gAVClockTestConfiguration, "IAVClockManager", gTestGroup,
                                     "ParsePCR", pcrValues);
    }
    if ((scenario.useAudio && audioStreams.empty()) || (scenario.useVideo && videoStreams.empty()) ||
        (scenario.usePcr && pcrValues.empty()))
    {
        GTEST_SKIP() << "No stream / PCR configuration available for " << scenario.name;
        return;
    }

    size_t runCount = 1;
    if (scenario.useAudio && scenario.useVideo)
        runCount = std::min(audioStreams.size(), videoStreams.size());
    else if (scenario.useAudio)
        runCount = audioStreams.size();
    else if (scenario.useVideo)
        runCount = videoStreams.size();

    bool isTestExecuted = false;
    for (auto secure : {false, true})
    {
        /* Secure only applies to decoders: nothing to decode without audio or video */
        if (secure && !scenario.useAudio && !scenario.useVideo)
            continue;

        for (size_t run = 0; run < runCount; run++)
        {
            /* One session per clock instance */
            std::vector<std::unique_ptr<AVClockSession>> sessions;
            for (size_t clockIndex = 0; clockIndex < clockList.size(); clockIndex++)
            {
                const auto& [clockId, clock, clockControlListener, clockEventListener] = clockList[clockIndex];
                const size_t resourceIndex = concurrent ? clockIndex : 0;
                auto session = std::make_unique<AVClockSession>(scenario, clockId, clock, clockControlListener,
                                                                VTS_L3_AVClock::mAVBufferManager,
                                                                VTS_L3_AVClock::mAVbufferHelper, secure);
                bool hasResources = true;
                if (scenario.useAudio)
                {
                    AVClockAudioPath audioPath;
                    hasResources = findAudioPath(audioStreams[run], secure, resourceIndex, audioPath);
                    session->setAudioPath(audioPath);
                }
                if (hasResources && scenario.useVideo)
                {
                    AVClockVideoPath videoPath;
                    hasResources = findVideoPath(videoStreams[run], secure, resourceIndex, videoPath);
                    session->setVideoPath(videoPath);
                }
                if (scenario.usePcr)
                {
                    session->setPcrValues(pcrValues);
                }
                if (!hasResources)
                {
                    UT_LOG_WARNING("Skipping AV clock %d for %s (%s): no free decoder / sink supports the stream",
                                   clockId.value, scenario.name.c_str(), secure ? "secure" : "non-secure");
                    continue;
                }
                sessions.push_back(std::move(session));
            }
            if (sessions.empty())
                continue;

            if (concurrent)
            {
                std::vector<std::thread> sessionThreads;
                for (auto& session : sessions)
                    sessionThreads.emplace_back(&AVClockSession::execute, session.get());
                for (auto& sessionThread : sessionThreads)
                    sessionThread.join();
            }
            else
            {
                for (auto& session : sessions)
                    session->execute();
            }
            isTestExecuted = true;
        }
    }

    /* Report a skip when no combination could run, so it is not reported as a pass */
    if (!isTestExecuted)
    {
        GTEST_SKIP() << "No clock / decoder / sink combination supports " << scenario.name;
    }
}

static AVClockScenario makeScenario(const char* name, avclock::ClockMode clockMode, bool useAudio, bool useVideo,
                                    bool usePcr, double playbackRate)
{
    AVClockScenario scenario;
    scenario.name = name;
    scenario.clockMode = clockMode;
    scenario.useAudio = useAudio;
    scenario.useVideo = useVideo;
    scenario.usePcr = usePcr;
    scenario.playbackRate = playbackRate;
    return scenario;
}

/*
 * L3 tests. Each runs one scenario sequentially (one clock instance at a time) or concurrently (all
 * clock instances at the same time); non-secure and secure run inside each test.
 * Checks in every session: the clock is unprimed after start, onPrimed fires exactly once, the clock
 * never returns to null or goes backwards once primed, and it runs at the expected rate.
 */

/** @brief AUTO mode with audio only */
UT_ADD_TEST(VTS_L3_AVClock, AVClock_Sequential_AutoAudioOnly)
{
    runAVClockScenario(makeScenario("AVClock_Sequential_AutoAudioOnly", avclock::ClockMode::AUTO, true, false, false, 1.0), false);
}

UT_ADD_TEST(VTS_L3_AVClock, AVClock_Concurrent_AutoAudioOnly)
{
    runAVClockScenario(makeScenario("AVClock_Concurrent_AutoAudioOnly", avclock::ClockMode::AUTO, true, false, false, 1.0), true);
}

/** @brief AUTO mode with video only (no audio sink, so the clock follows the video) */
UT_ADD_TEST(VTS_L3_AVClock, AVClock_Sequential_AutoVideoOnly)
{
    runAVClockScenario(makeScenario("AVClock_Sequential_AutoVideoOnly", avclock::ClockMode::AUTO, false, true, false, 1.0), false);
}

UT_ADD_TEST(VTS_L3_AVClock, AVClock_Concurrent_AutoVideoOnly)
{
    runAVClockScenario(makeScenario("AVClock_Concurrent_AutoVideoOnly", avclock::ClockMode::AUTO, false, true, false, 1.0), true);
}

/** @brief AUTO mode with audio and video in the same sync group */
UT_ADD_TEST(VTS_L3_AVClock, AVClock_Sequential_AutoAudioVideo)
{
    runAVClockScenario(makeScenario("AVClock_Sequential_AutoAudioVideo", avclock::ClockMode::AUTO, true, true, false, 1.0), false);
}

UT_ADD_TEST(VTS_L3_AVClock, AVClock_Concurrent_AutoAudioVideo)
{
    runAVClockScenario(makeScenario("AVClock_Concurrent_AutoAudioVideo", avclock::ClockMode::AUTO, true, true, false, 1.0), true);
}

/** @brief PCR mode: the clock follows the PCR samples; the playback rate must be rejected */
UT_ADD_TEST(VTS_L3_AVClock, AVClock_Sequential_PCRMode)
{
    runAVClockScenario(makeScenario("AVClock_Sequential_PCRMode", avclock::ClockMode::PCR, false, false, true, 1.0), false);
}

UT_ADD_TEST(VTS_L3_AVClock, AVClock_Concurrent_PCRMode)
{
    runAVClockScenario(makeScenario("AVClock_Concurrent_PCRMode", avclock::ClockMode::PCR, false, false, true, 1.0), true);
}

/** @brief AUTO mode with audio, playback rate 0.0 (paused: the clock must not advance) */
UT_ADD_TEST(VTS_L3_AVClock, AVClock_Sequential_AutoSetPlayRatePause)
{
    runAVClockScenario(makeScenario("AVClock_Sequential_AutoSetPlayRatePause", avclock::ClockMode::AUTO, true, false, false, 0.0), false);
}

UT_ADD_TEST(VTS_L3_AVClock, AVClock_Concurrent_AutoSetPlayRatePause)
{
    runAVClockScenario(makeScenario("AVClock_Concurrent_AutoSetPlayRatePause", avclock::ClockMode::AUTO, true, false, false, 0.0), true);
}

/** @brief AUTO mode with audio, playback rate 2.0 (fast forward) */
UT_ADD_TEST(VTS_L3_AVClock, AVClock_Sequential_AutoSetPlayRateFastForward)
{
    runAVClockScenario(makeScenario("AVClock_Sequential_AutoSetPlayRateFastForward", avclock::ClockMode::AUTO, true, false, false, 2.0), false);
}

UT_ADD_TEST(VTS_L3_AVClock, AVClock_Concurrent_AutoSetPlayRateFastForward)
{
    runAVClockScenario(makeScenario("AVClock_Concurrent_AutoSetPlayRateFastForward", avclock::ClockMode::AUTO, true, false, false, 2.0), true);
}

/** @} */ // End of AUDIO_SINK_HALTEST L3 File
/** @} */ // End of AUDIO_SINK_HALTEST
/** @} */ // End of AUDIO_SINK Module
/** @} */ // End of HPK
