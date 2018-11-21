#include "NodePd.hpp"

namespace nodePd {

/**
 * NodeJS wrapper implementation for libpd
 */
Nan::Persistent<v8::Function> NodePd::constructor;

NAN_MODULE_INIT(NodePd::Init)
{
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("NodePd").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "init", init);
  Nan::SetPrototypeMethod(tpl, "clear", clear);

  Nan::SetPrototypeMethod(tpl, "openPatch", openPatch);
  Nan::SetPrototypeMethod(tpl, "closePatch", closePatch);
  // Nan::SetPrototypeMethod(tpl, "addToSearchPath", addToSearchPath);
  // Nan::SetPrototypeMethod(tpl, "clearSearchPath", clearSearchPath);

  Nan::SetPrototypeMethod(tpl, "send", send);
  Nan::SetPrototypeMethod(tpl, "subscribe", subscribe);
  Nan::SetPrototypeMethod(tpl, "unsubscribe", unsubscribe);

  //  arrays
  Nan::SetPrototypeMethod(tpl, "arraySize", arraySize);
  Nan::SetPrototypeMethod(tpl, "writeArray", writeArray);
  // Nan::SetPrototypeMethod(tpl, "readArray", readArray);
  // Nan::SetPrototypeMethod(tpl, "clearArray", clearArray);

  // @note - pseudo-private method (is nullified in index.js)
  Nan::SetPrototypeMethod(tpl, "_setMessageCallback", _setMessageCallback);

  // getter / setter
  v8::Local<v8::ObjectTemplate> itpl = tpl->InstanceTemplate();
  Nan::SetAccessor(itpl, Nan::New<v8::String>("currentTime").ToLocalChecked(), currentTime);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("NodePd").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NodePd::NodePd()
  : initialized_(false)
{
  audioConfig_ = (audio_config_t *) malloc(sizeof(audio_config_t));
  // default config
  audioConfig_->numInputChannels = DEFAULT_NUM_INPUT_CHANNELS;
  audioConfig_->numOutputChannels = DEFAULT_NUM_OUTPUT_CHANNELS;
  audioConfig_->sampleRate = DEFAULT_SAMPLE_RATE;
  audioConfig_->ticks = DEFAULT_NUM_TICKS;

  // queue for sharing messages between PdReceiver and BackgroundProcess
  msgQueue_ = new LockedQueue<pd_msg_t>();

  // here we should have (on the heap)
  paWrapper_ = new PaWrapper();
  pdWrapper_ = new PdWrapper();
  pdReceiver_ = new PdReceiver(msgQueue_);
}

NodePd::~NodePd() {
  // clean the mess...
  delete paWrapper_;
  delete pdWrapper_;
  delete pdReceiver_;

  free(audioConfig_);
}

NAN_METHOD(NodePd::New)
{
  if (!info.IsConstructCall()) {
    return Nan::ThrowError(Nan::New("NodePd::New - called without new keyword").ToLocalChecked());
  }

  if (info.Length() > 0) {
    return Nan::ThrowError(Nan::New("NodePd::New - don't accept any arguments").ToLocalChecked());
  }

  NodePd * nodePd = new NodePd();
  nodePd->Wrap(info.This());

  info.GetReturnValue().Set(info.This());
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
NAN_METHOD(NodePd::init)
{
  NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

  if (nodePd->initialized_ == false) {
    int numInputChannels = nodePd->audioConfig_->numInputChannels;
    int numOutputChannels = nodePd->audioConfig_->numOutputChannels;
    int sampleRate = nodePd->audioConfig_->sampleRate;
    int ticks = nodePd->audioConfig_->ticks;

    // handle arguments if definde
    if (!info[0]->IsUndefined() && info[0]->IsObject()) {
      v8::Local<v8::Object> obj = info[0]->ToObject();

      v8::Local<v8::String> numInputChannelsProp =
        Nan::New<v8::String>("numInputChannels").ToLocalChecked();

      v8::Local<v8::String> numOutputChannelsProp =
        Nan::New<v8::String>("numOutputChannels").ToLocalChecked();

      v8::Local<v8::String> numSampleRateProp =
        Nan::New<v8::String>("sampleRate").ToLocalChecked();

      v8::Local<v8::String> ticksProp =
        Nan::New<v8::String>("ticks").ToLocalChecked();

      v8::Local<v8::Value> localNumInputChannels = obj->Get(numInputChannelsProp);
      v8::Local<v8::Value> localNumOutputChannels = obj->Get(numOutputChannelsProp);
      v8::Local<v8::Value> localSampleRate = obj->Get(numSampleRateProp);
      v8::Local<v8::Value> localTicks = obj->Get(ticksProp);

      if (!localNumInputChannels->IsUndefined() && localNumInputChannels->IsNumber())
        numInputChannels = Nan::To<int>(localNumInputChannels).FromJust();

      if (!localNumOutputChannels->IsUndefined() && localNumOutputChannels->IsNumber())
        numOutputChannels = Nan::To<int>(localNumOutputChannels).FromJust();

      if (!localSampleRate->IsUndefined() && localSampleRate->IsNumber())
        sampleRate = Nan::To<int>(localSampleRate).FromJust();

      if (!localTicks->IsUndefined() && localTicks->IsNumber())
        ticks = Nan::To<int>(localTicks).FromJust();
    }

    const int blockSize = nodePd->pdWrapper_->blockSize();

    nodePd->audioConfig_->numInputChannels = numInputChannels;
    nodePd->audioConfig_->numOutputChannels = numOutputChannels;
    nodePd->audioConfig_->sampleRate = sampleRate;
    nodePd->audioConfig_->blockSize = blockSize; // size of the pd blocks (e.g. 64)
    nodePd->audioConfig_->ticks = ticks; // number of blocks processed by pd in
    nodePd->audioConfig_->framesPerBuffer = blockSize * ticks;
    nodePd->audioConfig_->bufferDuration = (double) blockSize * (double) ticks / (double) sampleRate;

    // init pure-data
    const bool pdInitialized = nodePd->pdWrapper_->init(nodePd->audioConfig_);
    nodePd->pdWrapper_->setReceiver(nodePd->pdReceiver_);

    // init portaudio
    const bool paInitialized = nodePd->paWrapper_->init(
      nodePd->audioConfig_, nodePd->pdWrapper_->getLibPdInstance());

    // @note - this pretends to works but crashes when the worker exits
    v8::Local<v8::Function> dummyCallback;
    Nan::Callback * callback = new Nan::Callback(dummyCallback);

    nodePd->backgroundProcess_ = new BackgroundProcess(
      callback,
      nodePd->messageCallback_,
      nodePd->audioConfig_,
      nodePd->msgQueue_,
      nodePd->paWrapper_,
      nodePd->pdWrapper_
    );

    // launch audio loop
    AsyncQueueWorker(nodePd->backgroundProcess_);

    // block while time <= 0
    while ((double) nodePd->paWrapper_->currentTime <= 0.0f) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    nodePd->initialized_ = (pdInitialized && paInitialized);
    info.GetReturnValue().Set(nodePd->initialized_);
  } else {
    info.GetReturnValue().Set(Nan::Null());
  }
}

/**
 * clear Pd instance.
 */
NAN_METHOD(NodePd::clear)
{
  std::cout << "clear" << std::endl;
  NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());
  nodePd->pdWrapper_->clear(); // clear pd
  // nodePd->paWrapper_->clear(); // clear portaudio
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
 *
 * @todo - handle alternative signature accepting a patch as argument
 * @todo - would be nice to have only one argument with full path of the patch.
 */
NAN_METHOD(NodePd::openPatch)
{
  NodePd* nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

  if (!info[0]->IsString() || !info[1]->IsString()) {
    v8::Local<v8::String> errMsg =
      Nan::New("Invalid arguments: `filename` and `path` must be string").ToLocalChecked();

    return Nan::ThrowTypeError(errMsg);
  } else {

    v8::Local<v8::String> localFilename = Nan::To<v8::String>(info[0]).ToLocalChecked();
    Nan::Utf8String nanFilename(localFilename);
    std::string sFilename(*nanFilename);

    v8::Local<v8::String> localPath = Nan::To<v8::String>(info[1]).ToLocalChecked();
    Nan::Utf8String nanPath(localPath);
    std::string sPath(*nanPath);

    patch_infos_t patchInfos = nodePd->pdWrapper_->openPatch(sFilename, sPath);

    // create a js object representing the patch
    v8::Local<v8::Object> patch = Nan::New<v8::Object>();

    // properties
    v8::Local<v8::String> isValidProp = Nan::New<v8::String>("isValid").ToLocalChecked();
    v8::Local<v8::String> filenameProp = Nan::New<v8::String>("filename").ToLocalChecked();
    v8::Local<v8::String> pathProp = Nan::New<v8::String>("path").ToLocalChecked();
    v8::Local<v8::String> dollarZeroProp = Nan::New<v8::String>("$0").ToLocalChecked();

    v8::Local<v8::Boolean> isValid =
      Nan::New<v8::Boolean>((bool) patchInfos.isValid);

    v8::Local<v8::String> filename =
      Nan::New<v8::String>(patchInfos.filename).ToLocalChecked();

    v8::Local<v8::String> path =
      Nan::New<v8::String>(patchInfos.path).ToLocalChecked();

    v8::Local<v8::Integer> dollarZero =
      Nan::New<v8::Integer>(patchInfos.dollarZero);

    Nan::Set(patch, isValidProp, isValid);
    Nan::Set(patch, filenameProp, filename);
    Nan::Set(patch, pathProp, path);
    Nan::Set(patch, dollarZeroProp, dollarZero);

    info.GetReturnValue().Set(patch);
  }
}

/**
 * Close the given `NodePdPath` instance
 *
 * @param {Object} patch - A patch object as returned by open patch
 */
NAN_METHOD(NodePd::closePatch)
{
  NodePd* nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

  if (info[0]->IsObject()) {
    v8::Local<v8::Object> patch = info[0]->ToObject();
    v8::Local<v8::String> dollarZeroProp = Nan::New<v8::String>("$0").ToLocalChecked();
    v8::Local<v8::String> isValidProp = Nan::New<v8::String>("isValid").ToLocalChecked();
    v8::Local<v8::String> filenameProp = Nan::New<v8::String>("filename").ToLocalChecked();
    v8::Local<v8::String> pathProp = Nan::New<v8::String>("path").ToLocalChecked();

    if (
      !Nan::HasOwnProperty(patch, dollarZeroProp).FromJust() ||
      !Nan::HasOwnProperty(patch, isValidProp).FromJust() ||
      !Nan::HasOwnProperty(patch, filenameProp).FromJust() ||
      !Nan::HasOwnProperty(patch, pathProp).FromJust()
    ) {
      Nan::ThrowError(Nan::New("Invalid patch").ToLocalChecked());
    } else {
      int dollarZero = patch->Get(dollarZeroProp)->NumberValue();
      patch_infos_t patchInfos = nodePd->pdWrapper_->closePatch(dollarZero);

      // close patch only modify `isValid` and `DollarZero` props (cf. pd::Patch)
      v8::Local<v8::Boolean> localIsValid =
        Nan::New<v8::Boolean>((bool) patchInfos.isValid);

      v8::Local<v8::Integer> localDollarZero =
        Nan::New<v8::Integer>(patchInfos.dollarZero);

      Nan::Set(patch, isValidProp, localIsValid);
      Nan::Set(patch, dollarZeroProp, localDollarZero);
    }
  } else {
    Nan::ThrowError(Nan::New("patch is not an object").ToLocalChecked());
  }
}


/**
 * add the given path to the pd search path.
 * note: fails silently if path not found
 *
 * @param {String} path - pathname
 * @todo - implement
 */
NAN_METHOD(NodePd::addToSearchPath) {}

/**
 * Clear search path.
 * @todo - implement
 */
NAN_METHOD(NodePd::clearSearchPath) {}

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
NAN_METHOD(NodePd::send) {
  NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

  // get channel
  if (!info[0]->IsString())
    std::cout << "send: no channel given" << std::endl;

  v8::Local<v8::String> localChannel = Nan::To<v8::String>(info[0]).ToLocalChecked();
  Nan::Utf8String nanChannel(localChannel);
  std::string channel(*nanChannel);


  double time = 0.;

  if (info[2]->IsNumber()) {
    v8::Local<v8::Number> number = Nan::To<v8::Number>(info[2]).ToLocalChecked();
    time = number->Value();
  }

  if (info[1]->IsString()) {
    // @todo - test when receive is implemented
    v8::Local<v8::String> localSymbol = Nan::To<v8::String>(info[1]).ToLocalChecked();
    Nan::Utf8String nanSymbol(localSymbol);
    std::string symbol(*nanSymbol);

    pd_scheduled_msg_t msg(channel, time, symbol);
    nodePd->backgroundProcess_->addScheduledMessage(msg);

  } else if (info[1]->IsNumber()) {

    v8::Local<v8::Number> number = Nan::To<v8::Number>(info[1]).ToLocalChecked();
    const float num = number->Value();

    pd_scheduled_msg_t msg(channel, time, num);
    nodePd->backgroundProcess_->addScheduledMessage(msg);

  } else if (info[1]->IsArray()) {

    v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(info[1]);
    const int len = arr->Length();

    if (len > 0) {
      pd::List list;

      for (int i = 0; i < len; i++) {
        if (Nan::Has(arr, i).FromJust()) {
          // @note - ignore everything non float or string
          v8::Local<v8::Value> localValue = Nan::Get(arr, i).ToLocalChecked();

          if (localValue->IsNumber()) {
            float num = localValue->NumberValue();
            list.addFloat(num);
          } else if (localValue->IsString()) {
            Nan::Utf8String nanSymbol(localValue);
            std::string symbol(*nanSymbol);
            list.addSymbol(symbol);
          }
        }
      }

      pd_scheduled_msg_t msg(channel, time, list);
      nodePd->backgroundProcess_->addScheduledMessage(msg);
    }
  } else {
    pd_scheduled_msg_t msg(channel, time);
    nodePd->backgroundProcess_->addScheduledMessage(msg);
  }

}

NAN_METHOD(NodePd::arraySize) {
  NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

  // ignore * else
  if (info[0]->IsString()) {
    v8::Local<v8::String> localName = Nan::To<v8::String>(info[0]).ToLocalChecked();
    Nan::Utf8String nanName(localName);
    std::string name(*nanName);

    const int size = nodePd->pdWrapper_->arraySize(name);

    v8::Local<v8::Integer> localSize =
      Nan::New<v8::Integer>(size);

    info.GetReturnValue().Set(localSize);
  }
}

NAN_METHOD(NodePd::writeArray) {
  if (!info[0]->IsString() || !info[1]->IsTypedArray()) {
    v8::Local<v8::String> errMsg =
      Nan::New("Invalid arguments: `name` must be a string and `source` must be a Float32Array").ToLocalChecked();
    Nan::ThrowTypeError(errMsg);
  } else {
    NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

    v8::Local<v8::String> localName = Nan::To<v8::String>(info[0]).ToLocalChecked();
    Nan::Utf8String nanName(localName);
    std::string name(*nanName);

    v8::Local<v8::TypedArray> localArray = v8::Local<v8::TypedArray>::Cast(info[1]);
    Nan::TypedArrayContents<float> typed(localArray);

    std::vector<float> source(*typed, *typed + typed.length());//  = *typedd;

    // @todo length, offset
    // writeLen (default=-1)
    // offset (default=0)

    const bool result = nodePd->pdWrapper_->writeArray(name, source);

    v8::Local<v8::Integer> v8result =
      Nan::New<v8::Integer>(result);

    info.GetReturnValue().Set(result);
  }
}

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



// kind of private method - document only receive(channel, callback) wrapper
NAN_METHOD(NodePd::subscribe) {
  NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

  // ignore * else
  if (info[0]->IsString()) {
    v8::Local<v8::String> localChannel = Nan::To<v8::String>(info[0]).ToLocalChecked();
    Nan::Utf8String nanChannel(localChannel);
    std::string channel(*nanChannel);

    nodePd->pdWrapper_->subscribe(channel);
  }
}

NAN_METHOD(NodePd::unsubscribe) {
  NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

  // ignore * else
  if (info[0]->IsString()) {
    v8::Local<v8::String> localChannel = Nan::To<v8::String>(info[0]).ToLocalChecked();
    Nan::Utf8String nanChannel(localChannel);
    std::string channel(*nanChannel);

    nodePd->pdWrapper_->unsubscribe(channel);
  }
}

// @pseudo-private
NAN_METHOD(NodePd::_setMessageCallback) {
  NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());

  if (info[0]->IsFunction()) {
    Nan::Callback * callback = new Nan::Callback(info[0].As<v8::Function>());
    nodePd->messageCallback_ = callback;
  }
}


NAN_PROPERTY_GETTER(NodePd::currentTime) {
  NodePd * nodePd = Nan::ObjectWrap::Unwrap<NodePd>(info.This());
  double currentTime = nodePd->paWrapper_->currentTime;

  v8::Local<v8::Number> ret = Nan::New<v8::Number>(currentTime);

  info.GetReturnValue().Set(ret);
}

}; // namespace







