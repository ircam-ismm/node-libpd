#include "NodePd.h"



namespace node_lib_pd {

Napi::FunctionReference NodePd::constructor;

Napi::Object NodePd::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(
      env, "NodePd",
      {
          // readonly syntax uses: template<getter, setter=nullptr>(name)
          InstanceAccessor<&NodePd::CurrentTime>("currentTime"),

          InstanceMethod("destroy", &NodePd::Destroy),

          InstanceMethod("computeAudio", &NodePd::ComputeAudio),

          InstanceMethod("getDevicesCount", &NodePd::GetDevicesCount),
          InstanceMethod("listDevices", &NodePd::ListDevices),
          InstanceMethod("getDefaultInputDevice", &NodePd::GetDefaultInputDevice),
          InstanceMethod("getDefaultOutputDevice", &NodePd::GetDefaultOutputDevice),
          InstanceMethod("getInputDevices", &NodePd::GetInputDevices),
          InstanceMethod("getOutputDevices", &NodePd::GetOutputDevices),
          InstanceMethod("getDeviceAtIndex", &NodePd::GetDeviceAtIndex),

          InstanceMethod("closePatch", &NodePd::ClosePatch),
          InstanceMethod("addToSearchPath", &NodePd::AddToSearchPath),
          InstanceMethod("clearSearchPath", &NodePd::ClearSearchPath),
          InstanceMethod("send", &NodePd::Send),
          InstanceMethod("readArray", &NodePd::ReadArray),
          InstanceMethod("writeArray", &NodePd::WriteArray),
          InstanceMethod("clearArray", &NodePd::ClearArray),
          InstanceMethod("arraySize", &NodePd::ArraySize),

          InstanceMethod("startGUI", &NodePd::StartGUI),
          InstanceMethod("pollGUI", &NodePd::PollGUI),
          InstanceMethod("stopGUI", &NodePd::StopGUI),

          // monkey patched on the js side
          InstanceMethod("_initialize", &NodePd::Initialize),
          InstanceMethod("_openPatch", &NodePd::OpenPatch),
          InstanceMethod("_subscribe", &NodePd::Subscribe),
          InstanceMethod("_unsubscribe", &NodePd::Unsubscribe),
      });

// node: DEBUG seems to be defined when doing `node-gyp build --debug`
#ifdef DEBUG
  std::cout << "[node-libpd] c++ static init" << std::endl;
#endif

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("NodePd", func);

  return exports;
}

NodePd::NodePd(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<NodePd>(info), initialized_(false) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

#ifdef DEBUG
  std::cout << "[node-libpd] c++ constructor" << std::endl;
#endif

  this->audioConfig_ = (audio_config_t *)malloc(sizeof(audio_config_t));
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

  // this close the portaudio stream and therefore terminates the background
  // process
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
 * @param {int} [param.numInputChannels=0] - number of input channels (not
 * implemented)
 * @param {int} [param.numOutputChannels=2] - number of output channels
 * @param {int} [param.sampleRate=44100] - sample rate
 * @param {int} [param.ticks=1] - ticks
 */
Napi::Value NodePd::Initialize(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() != 3 || !info[0].IsObject() || !info[1].IsBoolean() || !info[2].IsFunction()) {
    Napi::Error::New(env, "Invalid Arguments").ThrowAsJavaScriptException();
  }

  if (this->initialized_ == false) {
    int numInputChannels = this->audioConfig_->numInputChannels;
    int numOutputChannels = this->audioConfig_->numOutputChannels;
    int sampleRate = this->audioConfig_->sampleRate;
    int ticks = this->audioConfig_->ticks;

    Napi::Object obj = info[0].As<Napi::Object>();

    if (obj.Has("numInputChannels")) {
      numInputChannels =
          obj.Get("numInputChannels").As<Napi::Number>().Int32Value();
    }

    if (obj.Has("numOutputChannels")) {
      numOutputChannels =
          obj.Get("numOutputChannels").As<Napi::Number>().Int32Value();
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
    this->audioConfig_->blockSize =
        blockSize; // size of the pd blocks (e.g. 64)
    this->audioConfig_->framesPerBuffer = blockSize * ticks;
    this->audioConfig_->bufferDuration =
        (double)blockSize * (double)ticks / (double)sampleRate;

    const bool compute_audio = info[1].As<Napi::Boolean>().Value();

    // init lib-pd
    const bool pdInitialized = this->pdWrapper_->init(this->audioConfig_, compute_audio);
    this->pdWrapper_->setReceiver(this->pdReceiver_);

    // // init portaudio
    const bool paInitialized = this->paWrapper_->init(
        this->audioConfig_, this->pdWrapper_->getLibPdInstance());

    Napi::Function callback = info[2].As<Napi::Function>();

    this->backgroundProcess_ =
        new BackgroundProcess(callback, this->audioConfig_, this->msgQueue_,
                              this->paWrapper_, this->pdWrapper_);

    this->backgroundProcess_->Queue();

    int millis = 0; // for debug
    // block process while time is not running
    // @note: maybe move to Promise API, but probably not really important
    // here...
    while ((double)this->paWrapper_->currentTime <= 0.0f) {
      millis += 1; // for debug
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

#ifdef DEBUG
    std::cout << "[node-libpd] > pd initialized: " << pdInitialized
              << std::endl;
    std::cout << "[node-libpd] > portaudio initialized: " << paInitialized
              << std::endl;
    std::cout << "[node-libpd] audio started in: " << millis << "ms"
              << std::endl;
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
Napi::Value NodePd::Destroy(const Napi::CallbackInfo &info) {
#ifdef DEBUG
  std::cout << "[node-libpd] destroy node-libpd instance" << std::endl;
#endif

  // not sure c++ guys would like that...
  delete this;

  return info.Env().Undefined();
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//
// DYNAMIC CONFIGURATION
//
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

/**
 * Set PD to compute audio or not.
 */
Napi::Value NodePd::ComputeAudio(const Napi::CallbackInfo &info) {
  bool compute_audio = true;

  if (info.Length() >= 1 && info[0].IsBoolean()) {
    compute_audio = info[0].As<Napi::Boolean>().Value();
  }

  this->pdWrapper_->computeAudio(compute_audio);

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

/**
 * Get audio devices count.
 */
Napi::Value NodePd::GetDevicesCount(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't getDevicesCount before init")
        .ThrowAsJavaScriptException();
  }

  int numDevices = this->paWrapper_->getDeviceCount();
  return Napi::Number::New(env, numDevices);
}
/**
 * List audio devices.
 */
Napi::Value NodePd::ListDevices(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't listDevices before init")
        .ThrowAsJavaScriptException();
  }

  int numDevices = this->paWrapper_->getDeviceCount();

  const PaDeviceInfo *deviceInfo;
  Napi::Array devices = Napi::Array::New(env, numDevices);

  for (int i = 0; i < numDevices; i++) {
    deviceInfo = this->paWrapper_->getDeviceAtIndex(i);

    Napi::Object device = this->PaDeviceToObject_(env, deviceInfo, i);
    devices[i] = device;
  }

  return devices;
}

Napi::Value NodePd::GetDefaultInputDevice(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't getDefaultInputDevice before init")
        .ThrowAsJavaScriptException();
  }

  PaDeviceIndex index = this->paWrapper_->getDefaultInputDevice();

  if (index == paNoDevice) {
    return env.Undefined();
  }

  const PaDeviceInfo *deviceInfo = this->paWrapper_->getDeviceAtIndex(index);
  return this->PaDeviceToObject_(env, deviceInfo, index);
}

Napi::Value NodePd::GetDefaultOutputDevice(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't getDefaultOutputDevice before init")
        .ThrowAsJavaScriptException();
  }

  PaDeviceIndex index = this->paWrapper_->getDefaultOutputDevice();
  if (index == paNoDevice) {
    return env.Undefined();
  }

  const PaDeviceInfo *deviceInfo = this->paWrapper_->getDeviceAtIndex(index);
  return this->PaDeviceToObject_(env, deviceInfo, index);
}

Napi::Value NodePd::GetInputDevices(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't getInputDevices before init")
        .ThrowAsJavaScriptException();
  }

  int numDevices = this->paWrapper_->getDeviceCount();

  const PaDeviceInfo *deviceInfo;
  Napi::Array devices = Napi::Array::New(env);

  int next = 0;
  for (int i = 0; i < numDevices; i++) {
    deviceInfo = this->paWrapper_->getDeviceAtIndex(i);

    if (deviceInfo->maxInputChannels > 0) {
      Napi::Object device = this->PaDeviceToObject_(env, deviceInfo, i);
      devices[next] = device;
      next++;
    }
  }

  return devices;
}

Napi::Value NodePd::GetOutputDevices(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't getOutputDevices before init")
        .ThrowAsJavaScriptException();
  }

  int numDevices = this->paWrapper_->getDeviceCount();

  const PaDeviceInfo *deviceInfo;
  Napi::Array devices = Napi::Array::New(env);

  int next = 0;
  for (int i = 0; i < numDevices; i++) {
    deviceInfo = this->paWrapper_->getDeviceAtIndex(i);

    if (deviceInfo->maxOutputChannels > 0) {
      Napi::Object device = this->PaDeviceToObject_(env, deviceInfo, i);
      devices[next] = device;
      next++;
    }
  }

  return devices;
}

Napi::Value NodePd::GetDeviceAtIndex(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't getDeviceAtIndex before init")
        .ThrowAsJavaScriptException();
  }

  if (info.Length() == 0 || !info[0].IsNumber()) {
    Napi::Error::New(env, "Invalid Arguments: pd.getDeviceAtIndex(index)")
        .ThrowAsJavaScriptException();
  }

  int index = info[0].As<Napi::Number>().Int32Value();
  const PaDeviceInfo *deviceInfo = this->paWrapper_->getDeviceAtIndex(index);

  if (deviceInfo != NULL) {
    return this->PaDeviceToObject_(env, deviceInfo, index);
  }
  
  return env.Undefined();
}

/**
 * Convert PaDeviceInfo to object.
 */
Napi::Object NodePd::PaDeviceToObject_(Napi::Env env, PaDeviceInfo const * deviceInfo, int index) {
  Napi::Object device = Napi::Object::New(env);

  device.Set("structVersion", Napi::Number::New(env, deviceInfo->structVersion));
  device.Set("index", Napi::Number::New(env, index));
  device.Set("name", Napi::String::New(env, deviceInfo->name));
  device.Set("maxInputChannels",
              Napi::Number::New(env, deviceInfo->maxInputChannels));
  device.Set("maxOutputChannels",
              Napi::Number::New(env, deviceInfo->maxOutputChannels));
  device.Set(
      "defaultLowInputLatency",
      Napi::Number::New(env, (double)deviceInfo->defaultLowInputLatency));
  device.Set(
      "defaultLowOutputLatency",
      Napi::Number::New(env, (double)deviceInfo->defaultLowOutputLatency));
  device.Set(
      "defaultHighInputLatency",
      Napi::Number::New(env, (double)deviceInfo->defaultHighInputLatency));
  device.Set(
      "defaultHighOutputLatency",
      Napi::Number::New(env, (double)deviceInfo->defaultHighOutputLatency));
  device.Set("defaultSampleRate",
              Napi::Number::New(env, (double)deviceInfo->defaultSampleRate));

  return device;
}

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
Napi::Value NodePd::OpenPatch(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't openPatch before init")
        .ThrowAsJavaScriptException();
  }

  if (!info[0].IsString() || !info[1].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.openPatch(filename, path)")
        .ThrowAsJavaScriptException();
  } else {
    std::string filename = info[0].As<Napi::String>().Utf8Value();
    std::string path = info[1].As<Napi::String>().Utf8Value();

    patch_infos_t patchInfos = this->pdWrapper_->openPatch(filename, path);

    // create a Plain Old Javascript Object to represent the patch
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
Napi::Value NodePd::ClosePatch(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't closePatch before init")
        .ThrowAsJavaScriptException();
  }

  if (!info[0].IsObject()) {
    Napi::Error::New(env, "Invalid Arguments: pd.closePatch(patch)")
        .ThrowAsJavaScriptException();
  } else {
    Napi::Object patch = info[0].As<Napi::Object>();

    if (!patch.Has("$0") || !patch.Has("filename") || !patch.Has("path") ||
        !patch.Has("isValid")) {
      Napi::Error::New(env, "Invalid Arguments: pd.closePatch(patch)")
          .ThrowAsJavaScriptException();
    }

    int dollarZero = patch.Get("$0").As<Napi::Number>().Int32Value();
    patch_infos_t patchInfos = this->pdWrapper_->closePatch(dollarZero);

    patch.Set("$0", patchInfos.dollarZero);
    patch.Set("isValid", patchInfos.isValid);

    return patch;
  }

  return env.Undefined();
}

Napi::Value NodePd::AddToSearchPath(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!info[0].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.addToSearchPath(pathname)")
        .ThrowAsJavaScriptException();
  }

  std::string pathname = info[0].As<Napi::String>().Utf8Value();
  this->pdWrapper_->addToSearchPath(pathname);

  return env.Undefined();
}

Napi::Value NodePd::ClearSearchPath(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  this->pdWrapper_->clearSearchPath();

  return env.Undefined();
}

Napi::Value NodePd::CurrentTime(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't get currentTime before init")
        .ThrowAsJavaScriptException();
  }

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
 * Send a message to pd. As pd only seems to only know 3 types (Float, Symbol,
 * and bang), we follow this convertion: Number -> Float String -> Symbol
 *  * -> bang
 *  Array -> list
 */
Napi::Value NodePd::Send(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't send before init")
        .ThrowAsJavaScriptException();
  }

  if (!info[0].IsString()) {
    Napi::Error::New(
        env, "Invalid Arguments: pd.send(channel, value[, scheduledTime])")
        .ThrowAsJavaScriptException();
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
    // number
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
Napi::Value NodePd::Subscribe(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't subscribe before init")
        .ThrowAsJavaScriptException();
  }

  if (!info[0].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.subscribe(channel)")
        .ThrowAsJavaScriptException();
  } else {
    std::string channel = info[0].As<Napi::String>().Utf8Value();
    this->pdWrapper_->subscribe(channel);
  }

  return env.Undefined();
}

Napi::Value NodePd::Unsubscribe(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't unsubscribe before init")
        .ThrowAsJavaScriptException();
  }

  if (!info[0].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.subscribe(channel)")
        .ThrowAsJavaScriptException();
  } else {
    std::string channel = info[0].As<Napi::String>().Utf8Value();
    this->pdWrapper_->unsubscribe(channel);
  }

  return env.Undefined();
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//
// ARRAYS
//
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

/**
 * @param {String} name
 * @param {Float32Array} data
 * @param {Number} [writeLen=data.length]
 * @param {Number} [offset=0]
 */
Napi::Value NodePd::WriteArray(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't writeArray before init")
        .ThrowAsJavaScriptException();
  }

  if (!info[0].IsString() || !info[1].IsTypedArray()) {
    Napi::Error::New(env, "Invalid Arguments: pd.writeArray(name, data, "
                          "len=data.length, offset=0)")
        .ThrowAsJavaScriptException();
  }

  std::string name = info[0].As<Napi::String>().Utf8Value();
  // cf.
  // https://github.com/nodejs/node-addon-examples/blob/master/array_buffer_to_native/node-addon-api/array_buffer_to_native.cc
  Napi::Float32Array buf = info[1].As<Napi::Float32Array>();
  const float *ptr = reinterpret_cast<float *>(buf.Data());
  const size_t len = buf.ByteLength() / sizeof(float);

  std::vector<float> source(ptr, ptr + len);

  int writeLen = len;
  int offset = 0;

  if (info[2].IsNumber()) {
    writeLen = info[2].As<Napi::Number>().Int32Value();
  }

  if (info[3].IsNumber()) {
    writeLen = info[3].As<Napi::Number>().Int32Value();
  }

  bool result = this->pdWrapper_->writeArray(name, source, writeLen, offset);
  return Napi::Boolean::New(env, result);
}

/**
 * @param {String} name
 * @param {Float32Arra} destBuffer
 * @param {Number} [readLen=destBuffer.length]
 * @param {Number} [offset=0]
 */
Napi::Value NodePd::ReadArray(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't readArray before init")
        .ThrowAsJavaScriptException();
  }

  if (!info[0].IsString() || !info[1].IsTypedArray()) {
    Napi::Error::New(env,
                     "Invalid Arguments: pd.readArray(name, len=1, offset=0)")
        .ThrowAsJavaScriptException();
  }

  std::string name = info[0].As<Napi::String>().Utf8Value();

  Napi::Float32Array buf = info[1].As<Napi::Float32Array>();
  const float *ptr = reinterpret_cast<float *>(buf.Data());
  const size_t len = buf.ByteLength() / sizeof(float);
  std::vector<float> dest(ptr, ptr + len);

  int readLen = len;
  int offset = 0;

  if (info[2].IsNumber()) {
    readLen = info[2].As<Napi::Number>().Int32Value();
  }

  if (info[3].IsNumber()) {
    offset = info[3].As<Napi::Number>().Int32Value();
  }

  bool result = this->pdWrapper_->readArray(name, dest, readLen, offset);

  // copy back values in the given Float32Array,
  // @note: this is weird, as it should be a reference to the same pointer
  // from what I understand (which shows I don't really understand...)
  for (int i = offset; i < readLen; i++) {
    buf[i] = dest[i];
  }

  return Napi::Boolean::New(env, result);
}

Napi::Value NodePd::ArraySize(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't arraySize before init")
        .ThrowAsJavaScriptException();
  }

  if (!info[0].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.arraySize(name)")
        .ThrowAsJavaScriptException();
  }

  std::string name = info[0].As<Napi::String>().Utf8Value();
  const int size = this->pdWrapper_->arraySize(name);

  return Napi::Number::New(env, size);
}

Napi::Value NodePd::ClearArray(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!this->initialized_) {
    Napi::Error::New(env, "Can't clearArray before init")
        .ThrowAsJavaScriptException();
  }

  if (!info[0].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.clearArray(name, value=0)")
        .ThrowAsJavaScriptException();
  }

  std::string name = info[0].As<Napi::String>().Utf8Value();
  int value = 0;

  if (info[1].IsNumber()) {
    value = info[1].As<Napi::Number>().Int32Value();
  }

  this->pdWrapper_->clearArray(name, value);

  return env.Undefined();
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//
// GUI
//
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

Napi::Value NodePd::StartGUI(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!info[0].IsString()) {
    Napi::Error::New(env, "Invalid Arguments: pd.startGUI(path)")
        .ThrowAsJavaScriptException();
  }

  std::string path = info[0].As<Napi::String>().Utf8Value();
  int result = this->pdWrapper_->startGUI(path);

  return Napi::Boolean::New(env, result == 0 ? true : false);
}

Napi::Value NodePd::PollGUI(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  this->pdWrapper_->pollGUI();

  return env.Undefined();
}

Napi::Value NodePd::StopGUI(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  this->pdWrapper_->stopGUI();

  return env.Undefined();
}

} // namespace node_lib_pd
