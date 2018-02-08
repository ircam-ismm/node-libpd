#include "./BackgroundProcess.hpp"

namespace nodePd {

BackgroundProcess::BackgroundProcess(
  Nan::Callback * callback, // this is a dummy callback created in NodePD.cpp
  Nan::Callback * onProgress,
  audio_config_t * audioConfig,
  LockedQueue<pd_msg_t> * msgQueue,
  pd::PdBase * pd,
  PaStream * paStream)
  : Nan::AsyncProgressWorker(callback)
  , onProgress_(onProgress)
  , msgQueue_(msgQueue)
  , pd_(pd)
  , paStream_(paStream)
  , audioConfig_(audioConfig)
{}

BackgroundProcess::~BackgroundProcess() {}

void BackgroundProcess::Execute(const Nan::AsyncProgressWorker::ExecutionProgress & progress)
{
  while (Pa_IsStreamActive(this->paStream_) == 1) {
    this->pd_->receiveMessages();

    // add flag to progress callback if the queue is not empty
    if (!this->msgQueue_->empty()) {
      bool flag = true;
      progress.Send(reinterpret_cast<const char *>(& flag), sizeof(bool));
    }

    // @note - as stated in the doc of `Pa_Sleep`
    // "This function is provided only as a convenience for authors of portable code"
    // > maybe we should be do this in some other way, but can't find any doc or example
    //
    // sleep for a block (in ms)
    // http://portaudio.com/docs/v19-doxydocs/portaudio_8h.html#a1b3c20044c9401c42add29475636e83d
    Pa_Sleep(this->audioConfig_->bufferDuration * 1000.0f);
  }
}

void BackgroundProcess::HandleProgressCallback(const char * data, size_t size)
{
  Nan::HandleScope scope;

  const bool flag = * reinterpret_cast<bool *>(const_cast<char *>(data));
  (void) flag; // prevent unused warning

  while (!this->msgQueue_->empty()) {
    auto ptr = this->msgQueue_->pop();

    v8::Local<v8::String> channel =
      Nan::New<v8::String>(ptr->channel).ToLocalChecked();

    switch (ptr->type) {
      case PD_MSG_TYPES::BANG_MSG: {
        v8::Local<v8::Value> argv[] = { channel };
        this->onProgress_->Call(1, argv);
        break;
      }

      case PD_MSG_TYPES::FLOAT_MSG: {
        v8::Local<v8::Number> num = Nan::New<v8::Number>(ptr->num);
        v8::Local<v8::Value> argv[] = { channel, num };
        this->onProgress_->Call(2, argv);
        break;
      }

      case PD_MSG_TYPES::SYMBOL_MSG: {
        v8::Local<v8::String> symbol =
          Nan::New<v8::String>(ptr->symbol).ToLocalChecked();
        v8::Local<v8::Value> argv[] = { channel, symbol };
        this->onProgress_->Call(2, argv);
        break;
      }

      // @note - not used: print an OSC-style type string
      case PD_MSG_TYPES::LIST_MSG: {
        const int len = ptr->list.len();
        v8::Local<v8::Array> list = Nan::New<v8::Array>(len);

        for (int i = 0; i < len; i++) {
          if (ptr->list.isFloat(i)) {
            v8::Local<v8::Number> num =
              Nan::New<v8::Number>(ptr->list.getFloat(i));

            Nan::Set(list, i, num);
          } else if (ptr->list.isSymbol(i)) {
            v8::Local<v8::String> symbol =
              Nan::New<v8::String>(ptr->list.getSymbol(i)).ToLocalChecked();

            Nan::Set(list, i, symbol);
          }
        }

        v8::Local<v8::Value> argv[] = { channel, list };
        this->onProgress_->Call(2, argv);
        break;
      }
    }

  }
}

/**
 * this crashes, problem with callback
 * this should not be called as we don't care when the stream finishes...
 */
void BackgroundProcess::HandleOkCallback()
{
  v8::Local<v8::Value> argv[] = {};
  callback->Call(1, argv);
}

}; // namespace


