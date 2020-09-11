#pragma once

#include <iostream>
#include <chrono>
#include <thread>
// #include <vector>
// #include <iterator>

#include <napi.h>
#include "types.h"
#include "./PaWrapper.h"
#include "./PdWrapper.h"
#include "./PdReceiver.h"
#include "./LockedQueue.h"
#include "./BackgroundProcess.h"
#include "PdBase.hpp"

namespace node_lib_pd {

/**
 *
 */
class NodePd : public Napi::ObjectWrap<NodePd> {
  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    NodePd(const Napi::CallbackInfo& info);

  private:
    static Napi::FunctionReference constructor;
    ~NodePd();

    static const int DEFAULT_NUM_INPUT_CHANNELS = 1;
    static const int DEFAULT_NUM_OUTPUT_CHANNELS = 2;
    static const int DEFAULT_SAMPLE_RATE = 48000;
    static const int DEFAULT_NUM_TICKS = 1;

    bool initialized_;
    audio_config_t * audioConfig_;
    LockedQueue<pd_msg_t> * msgQueue_;
    PaWrapper * paWrapper_;
    PdWrapper * pdWrapper_;
    PdReceiver * pdReceiver_;
    BackgroundProcess * backgroundProcess_;

    Napi::Value Initialize(const Napi::CallbackInfo& info);
    Napi::Value Destroy(const Napi::CallbackInfo& info);
    Napi::Value OpenPatch(const Napi::CallbackInfo& info);
    Napi::Value ClosePatch(const Napi::CallbackInfo& info);
    Napi::Value AddToSearchPath(const Napi::CallbackInfo& info);
    Napi::Value ClearSearchPath(const Napi::CallbackInfo& info);

    Napi::Value CurrentTime(const Napi::CallbackInfo& info);
    Napi::Value Send(const Napi::CallbackInfo& info);
    Napi::Value Subscribe(const Napi::CallbackInfo & info);
    Napi::Value Unsubscribe(const Napi::CallbackInfo & info);

    Napi::Value WriteArray(const Napi::CallbackInfo& info);
    Napi::Value ArraySize(const Napi::CallbackInfo& info);
    Napi::Value ReadArray(const Napi::CallbackInfo& info);
    Napi::Value ClearArray(const Napi::CallbackInfo& info);

    // static NAN_METHOD(listDevices);
};

}; // namespace

