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
    Napi::Value CurrentTime(const Napi::CallbackInfo& info);
    Napi::Value Send(const Napi::CallbackInfo& info);
    Napi::Value Subscribe(const Napi::CallbackInfo & info);
    Napi::Value Unsubscribe(const Napi::CallbackInfo & info);


    // static NAN_METHOD(listDevices);

    // static NAN_METHOD(openPatch);
    // static NAN_METHOD(closePatch);
    // // @todo - update
    // static NAN_METHOD(addToSearchPath);
    // static NAN_METHOD(clearSearchPath);

    // communications
    /**
     * @important
     * `subscribe` and `unsubscribe` follow here the libPd API,
     * not the API exposed in javascript, see `index.js` to see the  exposed
     * signature of these methods.
     */
    // static NAN_METHOD(subscribe);
    /**
     * @note - As messages can already be in the queue when unsubscribing,
     * some messages can be triggered after the call of this method.
     * However this concurrency issue is solved on the js side
     */
    // static NAN_METHOD(unsubscribe);

    /**
     * Set the callback for pd messages.
     * Is considered private as it is called in the js wrapper that implement
     * a more idiomatic js `subscribe/unsubscribe` pattern.
     * @important - must be called before `init`
     * @private
     */
    // static NAN_METHOD(_setMessageCallback);

    // static NAN_METHOD(send);

    // static NAN_METHOD(arraySize);
    // static NAN_METHOD(writeArray);
    // static NAN_METHOD(readArray);
    // static NAN_METHOD(clearArray);


    // static NAN_PROPERTY_GETTER(currentTime);
};

}; // namespace

