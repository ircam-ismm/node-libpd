#include "NodePd.h"

namespace node_lib_pd {

Napi::FunctionReference NodePd::constructor;

Napi::Object NodePd::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "NodePd", {
    // readonly synctax uses: template<getter, setter=nullptr>(name)
    InstanceAccessor<&NodePd::CurrentTime>("currentTime"),

    InstanceMethod("destroy", &NodePd::Destroy),
    InstanceMethod("closePatch", &NodePd::ClosePatch),
    InstanceMethod("send", &NodePd::Send),

    // monkey patched on the js side
    InstanceMethod("_initialize", &NodePd::Initialize),
    InstanceMethod("_openPatch", &NodePd::OpenPatch),
    InstanceMethod("_subscribe", &NodePd::Subscribe),
    InstanceMethod("_unsubscribe", &NodePd::Unsubscribe),
  });

  // node: DEBUG seems to defined when doing `node-gyp build --debug`
  #ifdef DEBUG
    std::cout << "[node-libpd] c++ static init" << std::endl;
  #endif

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("NodePd", func);

  return exports;
}

NodePd::NodePd(const Napi::CallbackInfo& info)
  : Napi::ObjectWrap<NodePd>(info)
  , initialized_(false)
{
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  #ifdef DEBUG
    std::cout << "[node-libpd] c++ constructor" << std::endl;
  #endif

  this->audioConfig_ = (audio_config_t *) malloc(sizeof(audio_config_t));
  // default config
  this->audioConfig_->numInputChannels = this->DEFAULT_NUM_INPUT_CHANNELS;
  this->audioConfig_->numOutputChannels = this->DEFAULT_NUM_OUTPUT_CHANNELS;
  this->audioConfig_->sampleRate = this->DEFAULT_SAMPLE_RATE;
  this->audioConfig_->ticks = this->DEFAULT_NUM_TICKS;

  // queue for sharing messages between PdReceiver and BackgroundProcess
  this->msgQueue_ = new LockedQueue<pd_msg_t>();

  this->paWrapper_ = new PaWrapper();
  this->pdWrapper_ = new PdWrapper();
  this->pdReceiver_ = new PdReceiver(msgQueue_);
}

NodePd::~NodePd() {
  #ifdef DEBUG
    std::cout << "[node-libpd] destructor called" << std::endl;
  #endif

  // this close the portaudio stream and therefore terminates the background process
  delete this->paWrapper_;
  delete this->pdWrapper_;
  delete this->pdReceiver_;

  free(this->audioConfig_);
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//
// LIFECYCLE
//
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

/**
 * init pd instance and start audio.
 * this method is hidden behind a js proxy that passes the message callback
 *
 * @param {Object} params - override default params
 * @param {int} [param.numInputChannels=0] - number of input channels (not implemented)
 * @param {int} [param.numOutputChannels=2] - number of output channels
 * @param {int} [param.sampleRate=44100] - sample rate
 *
 * @todo - check sampleRate against portaudio informations
 * @todo - add a parameter to change the number of ticks (blocks) processed by pd
 *         force it to be a multiple of 64
 */
Napi::Value NodePd::Initialize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 2 || !info[0].IsObject() || !info[1].IsFunction()) {
    Napi::Error::New(env, "Invalid Arguments").ThrowAsJavaScriptException();
    // return;
  }

  if (this->initialized_ == false) {
    int numInputChannels = this->audioConfig_->numInputChannels;
    int numOutputChannels = this->audioConfig_->numOutputChannels;
    int sampleRate = this->audioConfig_->sampleRate;
    int ticks = this->audioConfig_->ticks;

    Napi::Object obj = info[0].As<Napi::Object>();

    if (obj.Has("numInputChannels")) {
      numInputChannels = obj.Get("numInputChannels").As<Napi::Number>().Int32Value();
    }

    if (obj.Has("numOutputChannels")) {
      numOutputChannels = obj.Get("numOutputChannels").As<Napi::Number>().Int32Value();
    }

    if (obj.Has("sampleRate")) {
      sampleRate = obj.Get("sampleRate").As<Napi::Number>().Int32Value();
    }

    if (obj.Has("ticks")) {
      ticks = obj.Get("ticks").As<Napi::Number>().Int32Value();
    }

    const int blockSize = this->pdWrapper_->blockSize();

    this->audioConfig_->numInputChannels = numInputChannels;
    this->audioConfig_->numOutputChannels = numOutputChannels;
    this->audioConfig_->sampleRate = sampleRate;
    this->audioConfig_->ticks = ticks; // number of blocks processed by pd in
    this->audioConfig_->blockSize = blockSize; // size of the pd blocks (e.g. 64)
    this->audioConfig_->framesPerBuffer = blockSize * ticks;
    this->audioConfig_->bufferDuration = (double) blockSize * (double) ticks / (double) sampleRate;

      // init lib-pd
    const bool pdInitialized = this->pdWrapper_->init(this->audioConfig_);
    this->pdWrapper_->setReceiver(this->pdReceiver_);

    // // init portaudio
    const bool paInitialized = this->paWrapper_->init(
      this->audioConfig_,
      this->pdWrapper_->getLibPdInstance()
    );

    Napi::Function callback = info[1].As<Napi::Function>();
    // @note - this pretends to works but crashes when the worker exits
    this->backgroundProcess_ = new BackgroundProcess(
      callback,
      this->audioConfig_,
      this->msgQueue_,
      this->paWrapper_,
      this->pdWrapper_
    );

    this->backgroundProcess_->Queue();

    int millis = 0; // for debug
    // block process while time is not running
    // @note: move to Promise API ? (probably not really important here...)
    while ((double) this->paWrapper_->currentTime <= 0.0f) {
      millis += 1; // for debug
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    #ifdef DEBUG
      std::cout << "[node-libpd] > pd initialized: " << pdInitialized << std::endl;
      std::cout << "[node-libpd] > portaudio initialized: " << paInitialized << std::endl;
      std::cout << "[node-libpd] audio started in: " << millis << "ms" << std::endl;
    #endif

    this->initialized_ = true;
    return Napi::Boolean::New(info.Env(), this->initialized_);
  } else {
    return Napi::Boolean::New(info.Env(), this->initialized_);
  }
}

/**
 * clear Pd instance.
 * need to stop the background process cleanly
 */
Napi::Value NodePd::Destroy(const Napi::CallbackInfo& info) {
  #ifdef DEBUG
    std::cout << "[node-libpd] destroy node-libpd instance" << std::endl;
  #endif

  delete this;

  return info.Env().Undefined();
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//
// DEVICES
//
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

/**
 * clear Pd instance.
 */
// NAN_METHOD(NodePd::listDevices)
// {
//   // std::cout << "clear" << std::endl;
//   NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());
//   nodePd->paWrapper_->listDevices(); // clear pd
//   // nodePd->paWrapper_->clear(); // clear portaudio
// }


// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//
// PATCH
//
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------


/**
 * open a pd patch
 *
 * @param {String} filename - filename of the patch (i.e. test.pd)
 * @param {String} path - full path to the directory containing the patch
 *
 * @return {Object} - object containing informations about the patch
 */
Napi::Value NodePd::OpenPatch(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!info[0].IsString() || !info[1].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.openPatch(filename, path)").ThrowAsJavaScriptException();
  } else {
    std::string filename = info[0].As<Napi::String>().Utf8Value();
    std::string path = info[1].As<Napi::String>().Utf8Value();

    patch_infos_t patchInfos = this->pdWrapper_->openPatch(filename, path);

    // // create a js object representing the patch
    Napi::Object patch = Napi::Object::New(env);

    patch.Set("isValid", patchInfos.isValid);
    patch.Set("filename", patchInfos.filename);
    patch.Set("path", patchInfos.path);
    patch.Set("$0", patchInfos.dollarZero);

    return patch;
  }

  return env.Undefined();
}

/**
 * Close the given `NodePdPath` instance
 * @param {Object} patch - A patch object as returned by open patch
 */
Napi::Value NodePd::ClosePatch(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!info[0].IsObject()) {
    Napi::Error::New(env, "Invalid Arguments: pd.closePatch(patch)").ThrowAsJavaScriptException();
  } else {
    Napi::Object patch = info[0].As<Napi::Object>();

    if (!patch.Has("$0") || !patch.Has("filename") || !patch.Has("path") || !patch.Has("isValid")) {
      Napi::Error::New(env, "Invalid Arguments: pd.closePatch(patch)").ThrowAsJavaScriptException();
    }

    int dollarZero = patch.Get("$0").As<Napi::Number>().Int32Value();
    patch_infos_t patchInfos = this->pdWrapper_->closePatch(dollarZero);

    patch.Set("$0", patchInfos.dollarZero);
    patch.Set("isValid", patchInfos.isValid);

    return patch;
  }

  return env.Undefined();
}

Napi::Value NodePd::CurrentTime(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  double currentTime = this->paWrapper_->currentTime;
  return Napi::Number::New(env, currentTime);
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//
// MESSAGING
//
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

/**
 * Send a message to pd. As pd only knows 3 types (Float, Symbol, and bang).
 * All messages will follow this convertion:
 *  Number -> Float
 *  String -> Symbol
 *  * -> bang
 *  same for lists
 *
 * @note - don't implement typed messages for now (cf. `finishMessage` API)
 */
Napi::Value NodePd::Send(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!info[0].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.send(channel, value[, scheduledTime])").ThrowAsJavaScriptException();
  }

  std::string channel = info[0].As<Napi::String>().Utf8Value();

  double time = 0.;

  if (info[2].IsNumber()) {
    time = info[2].As<Napi::Number>().DoubleValue();
  }

  // symbol
  if (info[1].IsString()) {
    std::string symbol = info[1].As<Napi::String>().Utf8Value();
    pd_scheduled_msg_t msg(channel, time, symbol);
    this->backgroundProcess_->addScheduledMessage(msg);
  // symbol
  } else if (info[1].IsNumber()) {
    const float num = info[1].As<Napi::Number>().FloatValue();
    pd_scheduled_msg_t msg(channel, time, num);
    this->backgroundProcess_->addScheduledMessage(msg);
  // list
  } else if (info[1].IsArray()) {
    Napi::Array arr = info[1].As<Napi::Array>();
    const int len = arr.Length();

    if (len > 0) {
      pd::List list;

      for (int i = 0; i < len; i++) {
        if (arr.Has(i)) {
          Napi::Value val = arr.Get(i);

          if (val.IsNumber()) {
            const float num = val.As<Napi::Number>().FloatValue();
            list.addFloat(num);
          } else if (val.IsString()) {
            std::string str = val.As<Napi::String>().Utf8Value();
            list.addSymbol(str);
          }
        }
      }

      pd_scheduled_msg_t msg(channel, time, list);
      this->backgroundProcess_->addScheduledMessage(msg);
    } else {
      // fallback to bang
      pd_scheduled_msg_t msg(channel, time);
      this->backgroundProcess_->addScheduledMessage(msg);
    }
  } else {
    // default to bang
    pd_scheduled_msg_t msg(channel, time);
    this->backgroundProcess_->addScheduledMessage(msg);
  }

  return env.Undefined();
}

// these 2 method are hidden behind a js event emitter API
Napi::Value NodePd::Subscribe(const Napi::CallbackInfo & info) {
  Napi::Env env = info.Env();

  if (!info[0].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.subscribe(channel)").ThrowAsJavaScriptException();
  } else {
    std::string channel = info[0].As<Napi::String>().Utf8Value();
    this->pdWrapper_->subscribe(channel);
  }

  return env.Undefined();
}

Napi::Value NodePd::Unsubscribe(const Napi::CallbackInfo & info) {
  Napi::Env env = info.Env();

  if (!info[0].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.subscribe(channel)").ThrowAsJavaScriptException();
  } else {
    std::string channel = info[0].As<Napi::String>().Utf8Value();
    this->pdWrapper_->unsubscribe(channel);
  }

  return env.Undefined();
}


// NAN_METHOD(NodePd::arraySize) {
//   NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

//   // ignore * else
//   if (info[0]->IsString()) {
//     v8::Local<v8::String> localName = Nan::To<v8::String>(info[0]).ToLocalChecked();
//     Nan::Utf8String nanName(localName);
//     std::string name(*nanName);

//     const int size = nodePd->pdWrapper_->arraySize(name);

//     v8::Local<v8::Integer> localSize =
//       Nan::New<v8::Integer>(size);

//     info.GetReturnValue().Set(localSize);
//   }
// }

// NAN_METHOD(NodePd::writeArray) {
//   if (!info[0]->IsString() || !info[1]->IsTypedArray()) {
//     v8::Local<v8::String> errMsg =
//       Nan::New("Invalid arguments: `name` must be a string and `source` must be a Float32Array").ToLocalChecked();
//     Nan::ThrowTypeError(errMsg);
//   } else {
//     NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

//     v8::Local<v8::String> localName = Nan::To<v8::String>(info[0]).ToLocalChecked();
//     Nan::Utf8String nanName(localName);
//     std::string name(*nanName);

//     v8::Local<v8::TypedArray> localArray = v8::Local<v8::TypedArray>::Cast(info[1]);
//     Nan::TypedArrayContents<float> typed(localArray);

//     std::vector<float> source(*typed, *typed + typed.length());//  = *typedd;

//     // @todo length, offset
//     // writeLen (default=-1)
//     // offset (default=0)

//     const bool result = nodePd->pdWrapper_->writeArray(name, source);

//     v8::Local<v8::Integer> v8result =
//       Nan::New<v8::Integer>(result);

//     info.GetReturnValue().Set(v8result);
//   }
// }

// NAN_METHOD(NodePd::readArray) {

//   if (!info[0]->IsString()) {
//     v8::Local<v8::String> errMsg =
//       Nan::New("Invalid arguments: `name` must be a string").ToLocalChecked();
//     Nan::ThrowTypeError(errMsg);
//   } else {
//     NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

//     v8::Local<v8::String> localName = Nan::To<v8::String>(info[0]).ToLocalChecked();
//     Nan::Utf8String nanName(localName);
//     std::string name(*nanName);

//     std::vector<float> dest;

//     nodePd->pdWrapper_->readArray(name, dest);

//     float test = dest[0];
//     std::cout << test << std::endl;
//   }
// }

// NAN_METHOD(NodePd::clearArray) {

// }


/**
 * add the given path to the pd search path.
 * note: fails silently if path not found
 *
 * @param {String} path - pathname
 * @todo - implement
 */
// NAN_METHOD(NodePd::addToSearchPath) {}

/**
 * Destroy search path.
 * @todo - implement
 */
// NAN_METHOD(NodePd::clearSearchPath) {}

} // namespace

