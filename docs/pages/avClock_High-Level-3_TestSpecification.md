# AV Clock Level 3 High Level Test Specification Document

## Table of Contents

- [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
- [Introduction](#introduction)
- [Module Description](#module-description)
- [Test Frame Specifications](#test-frame-specifications)
- [L3 Test Specification](#l3-test-specification)

## Acronyms, Terms and Abbreviations

- `HAL`    - Hardware Abstraction Layer
- `API`    - Application Programming Interface
- `L3`     - Level 3 Testing
- `NA`     - Not Applicable
- `Y`      - Yes
- `CB`     - Call-back function (suffix)
- `RDK`    - Reference Design Kit
- `EOS`    - End Of Stream
- `PCR`    - Program Clock Reference

## Introduction

This document provides an overview of the testing requirements for the AV Clock module. It outlines the scope of testing, objectives, testing levels and approaches, specific test requirements, and expected deliverables.

- AV Clock Interface Definitions - [AV Clock HAL](https://github.com/rdkcentral/rdk-halif-aidl/tree/0.7.1/avclock/current/com/rdk/hal/avclock)

- AV Clock Overview - [AV Clock Overview](https://wiki.rdkcentral.com/display/RDKHAL/AV+Clock)

## Module Description

The AV Clock HAL service links audio sinks and video sinks together to provide lip sync playback against a shared clock.

The audio and video sinks that are linked by an AV Clock form a sync group which share a common clock for presentation.

The AV Clock HAL allows the application to specify which video sink and audio sink(s) are linked in the sync group.

## Test Streams Specification

-Audio Test Config File - [Audio Test Config File](../../avclock_test_configuration/vts_l3_primeaudio_test_config.yaml)

-Video Test Config File - [Video Test Config File](../../avclock_test_configuration/vts_l3_primevideo_test_config.yaml)

-PCR Test Config File - [PCR Test Config File](../../avclock_test_configuration/vts_l3_AVClock_PCR.yaml)

## L3 Test Specification

### Prerequisites

#### AV Buffer

Audio frames , Video frames are supplied to the Audio Decoder , Video Decoder  during testing on AV Clock, which necessitates a pre-validated AVBuffer module. Therefore, the AVBuffer must successfully pass VTS L1, L2, and L3 testing before being used in the Audio Decoder tests.

### Audio Decoder 

Audio frames are supplied to the Audio Decoder during testing on AV Clock, which necessitates a pre-validated Audio Decoder module. Therefore, the AudioDecoder must successfully pass VTS L1, L2, and L3 testing before being used in the AV Clock tests.

### Video Decoder

Video frames are supplied to the Video Decoder during testing on AV Clock, which necessitates a pre-validated Video Decoder module. Therefore, the VideoDecoder must successfully pass VTS L1, L2, and L3 testing before being used in the AV Clock tests.

### Audio Sink

Decoded Audio frames are supplied to the Audio Sink during testing, which necessitates a pre-validated Audio Sink module. Therefore, the AudioSink must successfully pass VTS L1, L2, and L3 testing before being used in the AV Clock tests.

### Video Sink

Decoded Video frames are supplied to the Video Sink during testing, which necessitates a pre-validated Video Sink module. Therefore, the VideoSink must successfully pass VTS L1, L2, and L3 testing before being used in the AV Clock tests.

### Feature List

VTS L3 validates all the AV Clock features in both sequential and concurrent  modes (if the platform supports multiple AV Clock instances) as represented below.

### Sequential Clock Priming

The Sequential AV Clock test evaluates each AV Clock individually, one after another, to ensure functional correctness across all supported features. This approach helps isolate issues and verify that every AV Clock instance performs as expected in a standalone environment.

#### Audio Stream

```mermaid
block-beta
    block:1
        AudioStream("Audio Stream") space AVBuffer("AVBuffer") space AudioDecoder("AudioDecoder") space AudioSink("AudioSink") space AVClock("AV Clock") space Output("Output")
        AudioStream --> AVBuffer
        AVBuffer --> AudioDecoder
        AudioDecoder --> AudioSink
        AudioSink --> AVClock
        AVClock --> Output
    end
```
#### Video Stream

```mermaid
block-beta
    block:1
        VideoStream("Video Stream") space AVBuffer("AVBuffer") space VideoDecoder("VideoDecoder") space VideoSink("VideoSink") space AVClock("AV Clock") space Output("Output")
        VideoStream --> AVBuffer
        AVBuffer --> VideoDecoder
        VideoDecoder --> VideoSink
        VideoSink --> AVClock
        AVClock --> Output
    end
```

### Concurrent Clock Priming

The Concurrent Clocking test assesses the system's ability to handle multiple AV Clock operating simultaneously, each configured with different supported features. This test is designed to validate the robustness, resource management, and feature compatibility of the platform under concurrent load, and is applicable only if the platform supports multiple AV Clock instances.

#### Audio Stream
```mermaid
block-beta
columns 1
    block:1
        AudioStream_1("Audio Stream") space AVBuffer_1("AVBuffer") space AudioDecoder_1("AudioDecoder_1") space AudioSink_1("AudioSink") space AVClock_1("AVClock_1") space Output_1("Output")
        AudioStream_1 --> AVBuffer_1
        AVBuffer_1 --> AudioDecoder_1
        AudioDecoder_1 --> AudioSink_1
        AudioSink_1 --> AVClock_1
        AVClock_1 --> Output_1
    end
    space
    block:2
        AudioStream_2("Audio Stream") space AVBuffer_2("AVBuffer") space AudioDecoder_2("AudioDecoder_2") space AudioSink_2("AudioSink_2") space AVClock_2("AVClock_2") space Output_1("Output")
        AudioStream_2 --> AVBuffer_2
        AVBuffer_2 --> AudioDecoder_2
        AudioDecoder_2 --> AudioSink_2
        AudioSink_2 --> AVClock_2
        AVClock_2 --> Output_2
    end
    space
    block:N
        AudioStream_N("Audio Stream") space AVBuffer_N("AVBuffer") space AudioDecoder_N("AudioDecoder_N") space AudioSink_N("AudioSink_N") space AVClock_N("AVClock_N") space Output_N("Output")
        AudioStream_N --> AVBuffer_N
        AVBuffer_N --> AudioDecoder_N
        AudioDecoder_N--> AudioSink_N
        AudioSink_N --> AVClock_N
        AVClock_N --> Output_N
    end
```

#### Video Stream
```mermaid
block-beta
columns 1
    block:1
        VideoStream_1("Video Stream") space AVBuffer_1("AVBuffer") space VideoDecoder_1("VideoDecoder_1") space VideoSink_1("VideoSink") space AVClock_1("AVClock_1") space Output_1("Output")
        VideoStream_1 --> AVBuffer_1
        AVBuffer_1 --> VideoDecoder_1
        VideoDecoder_1 --> VideoSink_1
        VideoSink_1 --> AVClock_1
        AVClock_1 --> Output_1
    end
    space
    block:2
        VideoStream_2("Video Stream") space AVBuffer_2("AVBuffer") space VideoDecoder_2("VideoDecoder_2") space VideoSink_2("VideoSink_2") space AVClock_2("AVClock_2") space Output_1("Output")
        VideoStream_2 --> AVBuffer_2
        AVBuffer_2 --> VideoDecoder_2
        VideoDecoder_2 --> VideoSink_2
        VideoSink_2 --> AVClock_2
        AVClock_2 --> Output_2
    end
    space
    block:N
        VideoStream_N("Video Stream") space AVBuffer_N("AVBuffer") space VideoDecoder_N("VideoDecoder_N") space VideoSink_N("VideoSink_N") space AVClock_N("AVClock_N") space Output_N("Output")
        VideoStream_N --> AVBuffer_N
        AVBuffer_N --> VideoDecoder_N
        VideoDecoder_N--> VideoSink_N
        VideoSink_N --> AVClock_N
        AVClock_N --> Output_N
    end
```

### Test Cases

The table below summarizes the L3 test cases executed using various audio stream formats and configurations. These tests are also executed in a multi-process environment:

|#|Test|Secure|Auto|PCR|AudioMaster|VideoMaster|AudioOnly|VideoOnly|AudioBeforeVideo|VideoBeforeAudio|PlayBackRate|Concurrency|
|-|----|------|----|---|-----------|-----------|---------|---------|----------------|----------------|------------|-----------|
|1|PrimeClock_Sequential_NonSecure_AutoAudioOnly|✘|✔|✘|✘|✘|✔|✘|✘|✘|✘|✘|
|2|PrimeClock_Sequential_NonSecure_AutoVideoOnly|✘|✔|✘|✘|✘|✘|✔|✘|✘|✘|✘|
|3|PrimeClock_Sequential_NonSecure_AudioMasterAudioOnly|✘|✘|✘|✔|✘|✔|✘|✘|✘|✘|✘|
|4|PrimeClock_Sequential_NonSecure_VideoMasterVideoOnly|✘|✘|✘|✘|✔|✘|✔|✘|✘|✘|✘|
|5|PrimeClock_Sequential_NonSecure_AudioMasterVideoOnly|✘|✘|✘|✔|✘|✘|✔|✘|✘|✘|✘|
|6|PrimeClock_Sequential_NonSecure_VideoMasterAudioOnly|✘|✘|✘|✘|✔|✔|✘|✘|✘|✘|✘|
|7|PrimeClock_Sequential_NonSecure_PCRMode|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|
|8|PrimeClock_Sequential_NonSecure_AutoAudioBeforeVideo|✘|✔|✘|✘|✘|✘|✘|✔|✘|✘|✘|
|9|PrimeClock_Sequential_NonSecure_AutoVideoBeforeAudio|✘|✔|✘|✘|✘|✘|✘|✘|✔|✘|✘|
|10|PrimeClock_Sequential_NonSecure_AutoSetAudioPlayRateNormalSpeed|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|11|PrimeClock_Sequential_NonSecure_AutoSetAudioPlayRatePause|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|12|PrimeClock_Sequential_NonSecure_AutoSetAudioPlayRateFastForward|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|13|PrimeClock_Sequential_NonSecure_AutoSetAudioPlayRateNormalSpeed|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|14|PrimeClock_Sequential_NonSecure_AutoSetAudioPlayRatePause|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|15|PrimeClock_Sequential_NonSecure_AutoSetAudioPlayRateFastForward|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|16|PrimeClock_Sequential_Secure_AutoAudioOnly|✔|✔|✘|✘|✘|✔|✘|✘|✘|✘|✘|
|17|PrimeClock_Sequential_Secure_AutoVideoOnly|✔|✔|✘|✘|✘|✘|✔|✘|✘|✘|✘|
|18|PrimeClock_Sequential_Secure_AudioMasterAudioOnly|✔|✘|✘|✔|✘|✔|✘|✘|✘|✘|✘|
|19|PrimeClock_Sequential_Secure_VideoMasterVideoOnly|✔|✘|✘|✘|✔|✘|✔|✘|✘|✘|✘|
|20|PrimeClock_Sequential_Secure_AudioMasterVideoOnly|✔|✘|✘|✔|✘|✘|✔|✘|✘|✘|✘|
|21|PrimeClock_Sequential_Secure_VideoMasterAudioOnly|✔|✘|✘|✘|✔|✔|✘|✘|✘|✘|✘|
|22|PrimeClock_Sequential_Secure_PCRMode|✔|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|
|23|PrimeClock_Sequential_Secure_AutoAudioBeforeVideo|✔|✔|✘|✘|✘|✘|✘|✔|✘|✘|✘|
|24|PrimeClock_Sequential_Secure_AutoVideoBeforeAudio|✔|✔|✘|✘|✘|✘|✘|✘|✔|✘|✘|
|25|PrimeClock_Sequential_Secure_AutoSetAudioPlayRateNormalSpeed|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|26|PrimeClock_Sequential_Secure_AutoSetAudioPlayRatePause|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|27|PrimeClock_Sequential_Secure_AutoSetAudioPlayRateFastForward|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|28|PrimeClock_Sequential_Secure_AutoSetVideoPlayRateNormalSpeed|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|29|PrimeClock_Sequential_Secure_AutoSetVideoPlayRatePause|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|30|PrimeClock_Sequential_Secure_AutoSetVideoPlayRateFastForward|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|31|PrimeClock_Concurrent_NonSecure_AutoAudioOnly|✘|✔|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|32|PrimeClock_Concurrent_NonSecure_AutoVideoOnly|✘|✔|✘|✘|✘|✘|✔|✘|✘|✘|✔|
|33|PrimeClock_Concurrent_NonSecure_AudioMasterAudioOnly|✘|✘|✘|✔|✘|✔|✘|✘|✘|✘|✔|
|34|PrimeClock_Concurrent_NonSecure_VideoMasterVideoOnly|✘|✘|✘|✘|✔|✘|✔|✘|✘|✘|✔|
|35|PrimeClock_Concurrent_NonSecure_AudioMasterVideoOnly|✘|✘|✘|✔|✘|✘|✔|✘|✘|✘|✔|
|36|PrimeClock_Concurrent_NonSecure_VideoMasterAudioOnly|✘|✘|✘|✘|✔|✔|✘|✘|✘|✘|✔|
|37|PrimeClock_Concurrent_NonSecure_PCRMode|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✔|
|38|PrimeClock_Concurrent_NonSecure_AutoAudioBeforeVideo|✘|✔|✘|✘|✘|✘|✘|✔|✘|✘|✔|
|39|PrimeClock_Concurrent_NonSecure_AutoVideoBeforeAudio|✘|✔|✘|✘|✘|✘|✘|✘|✔|✘|✔|
|40|PrimeClock_Concurrent_NonSecure_AutoSetAudioPlayRateNormalSpeed|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|41|PrimeClock_Concurrent_NonSecure_AutoSetAudioPlayRatePause|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|42|PrimeClock_Concurrent_NonSecure_AutoSetAudioPlayRateFastForward|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|43|PrimeClock_Concurrent_NonSecure_AutoSetVideoPlayRateNormalSpeed|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|44|PrimeClock_Concurrent_NonSecure_AutoSetVideoPlayRatePause|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|45|PrimeClock_Concurrent_NonSecure_AutoSetVideoPlayRateFastForward|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|46|PrimeClock_Concurrent_Secure_AutoAudioOnly|✔|✔|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|47|PrimeClock_Concurrent_Secure_AutoVideoOnly|✔|✔|✘|✘|✘|✘|✔|✘|✘|✘|✔|
|48|PrimeClock_Concurrent_Secure_AudioMasterAudioOnly|✔|✘|✘|✔|✘|✔|✘|✘|✘|✘|✔|
|49|PrimeClock_Concurrent_Secure_VideoMasterVideoOnly|✔|✘|✘|✘|✔|✘|✔|✘|✘|✘|✔|
|50|PrimeClock_Concurrent_Secure_AudioMasterVideoOnly|✔|✘|✘|✔|✘|✘|✔|✘|✘|✘|✔|
|51|PrimeClock_Concurrent_Secure_VideoMasterAudioOnly|✔|✘|✘|✘|✔|✔|✘|✘|✘|✘|✔|
|52|PrimeClock_Concurrent_Secure_PCRMode|✔|✘|✔|✘|✘|✘|✘|✘|✘|✘|✔|
|53|PrimeClock_Concurrent_Secure_AutoAudioBeforeVideo|✔|✔|✘|✘|✘|✘|✘|✔|✘|✘|✔|
|54|PrimeClock_Concurrent_Secure_AutoVideoBeforeAudio|✔|✔|✘|✘|✘|✘|✘|✘|✔|✘|✔|
|55|PrimeClock_Concurrent_Secure_AutoSetAudioPlayRateNormalSpeed|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|56|PrimeClock_Concurrent_Secure_AutoSetAudioPlayRatePause|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|57|PrimeClock_Concurrent_Secure_AutoSetAudioPlayRateFastForward|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|58|PrimeClock_Concurrent_Secure_AutoSetVideoPlayRateNormalSpeed|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|59|PrimeClock_Concurrent_Secure_AutoSetVideoPlayRatePause|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|60|PrimeClock_Concurrent_Secure_AutoSetVideoPlayRateFastForward|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|