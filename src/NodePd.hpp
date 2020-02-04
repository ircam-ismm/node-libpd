#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <iterator>

#include "nan.h"
#include "PdBase.hpp"
#include "./BackgroundProcess.hpp"
#include "./PaWrapper.hpp"
#include "./PdWrapper.hpp"
#include "./PdReceiver.hpp"
#include "./LockedQueue.hpp"

namespace nodePd {

// struct global_t {
//   std::string test;
//   pd::PdBase * pd;
// };

// global_t niap;
// niap.pd = new pd::PdBase();
// niap.test = "test";

/**
 * Based on https://nodejs.org/api/addons.html#addons_wrapping_c_objects but using NAN
 *
 * @todo - handle exit properly, cf. https://nodejs.org/api/addons.html#addons_atexit_hooks
 */
class NodePd : public Nan::ObjectWrap {
  public:
    static NAN_MODULE_INIT(Init);

  private:
    explicit NodePd();
    ~NodePd();

    bool initialized_;

    audio_config_t * audioConfig_;

    LockedQueue<pd_msg_t> * msgQueue_;

    PaWrapper * paWrapper_;
    PdWrapper * pdWrapper_;
    PdReceiver * pdReceiver_;
    BackgroundProcess * backgroundProcess_;

    Nan::Callback * messageCallback_;

    static const int DEFAULT_NUM_INPUT_CHANNELS = 1;
    static const int DEFAULT_NUM_OUTPUT_CHANNELS = 2;
    static const int DEFAULT_SAMPLE_RATE = 44100;
    static const int DEFAULT_NUM_TICKS = 1;

    static NAN_METHOD(New);
    static Nan::Persistent<v8::Function> constructor;

    static NAN_METHOD(init);
    static NAN_METHOD(clear);

    static NAN_METHOD(listDevices);

    static NAN_METHOD(openPatch);
    static NAN_METHOD(closePatch);
    // @todo - update
    static NAN_METHOD(addToSearchPath);
    static NAN_METHOD(clearSearchPath);

    // communications
    /**
     * @important
     * `subscribe` and `unsubscribe` follow here the libPd API,
     * not the API exposed in javascript, see `index.js` to see the  exposed
     * signature of these methods.
     */
    static NAN_METHOD(subscribe);
    /**
     * @note - As messages can already be in the queue when unsubscribing,
     * some messages can be triggered after the call of this method.
     * However this concurrency issue is solved on the js side
     */
    static NAN_METHOD(unsubscribe);

    /**
     * Set the callback for pd messages.
     * Is considered private as it is called in the js wrapper that implement
     * a more idiomatic js `subscribe/unsubscribe` pattern.
     * @important - must be called before `init`
     * @private
     */
    static NAN_METHOD(_setMessageCallback);

    static NAN_METHOD(send);

    static NAN_METHOD(arraySize);
    static NAN_METHOD(writeArray);
    // static NAN_METHOD(readArray);
    // static NAN_METHOD(clearArray);


    static NAN_PROPERTY_GETTER(currentTime);
};

}; // namespace
