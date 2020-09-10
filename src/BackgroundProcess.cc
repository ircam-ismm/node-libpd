#include "./BackgroundProcess.h"

namespace node_lib_pd {

BackgroundProcess::BackgroundProcess(
  Napi::Function& callback,
  audio_config_t * audioConfig,
  LockedQueue<pd_msg_t> * msgQueue,
  PaWrapper * paWrapper,
  PdWrapper * pdWrapper)
  : Napi::AsyncProgressWorker<uint32_t>(callback, "pa background process")
  , audioConfig_(audioConfig)
  , msgReceiveQueue_(msgQueue)
  , paWrapper_(paWrapper)
  , pdWrapper_(pdWrapper)
  , mut_()
{}

BackgroundProcess::~BackgroundProcess() {
  #ifdef DEBUG
    std::cout << "[node-libpd] delete background process" << std::endl;
  #endif
}

void BackgroundProcess::addScheduledMessage(pd_scheduled_msg_t msg) {
  std::lock_guard<std::mutex> lock(mut_);
  this->sendMsgQueue_.push(msg);
}

// this is called in the worker thread
void BackgroundProcess::Execute(const BackgroundProcess::ExecutionProgress& progress) {
  PaStream * paStream = this->paWrapper_->getStream();

  while (Pa_IsStreamActive(paStream) == 1) {
    double currentTime = this->paWrapper_->currentTime;
    double lookAhead = this->audioConfig_->bufferDuration;
    double nextTime = currentTime + lookAhead;

    std::unique_lock<std::mutex> lock(this->mut_);
    // send scheduled messages to pd
    while (!this->sendMsgQueue_.empty() && this->sendMsgQueue_.top().time <= nextTime) {
      pd_scheduled_msg_t nextMsg = this->sendMsgQueue_.top();
      this->pdWrapper_->sendMessage(nextMsg);
      this->sendMsgQueue_.pop();
    }

    lock.unlock();

    // receive messages from pd
    this->pdWrapper_->getLibPdInstance()->receiveMessages();

    // add flag to progress callback if the queue is not empty
    if (!this->msgReceiveQueue_->empty()) {
      const uint32_t i = 1;
      progress.Send(&i, 1);
    }

    // sleep for a block (in ms)
    // http://portaudio.com/docs/v19-doxydocs/portaudio_8h.html#a1b3c20044c9401c42add29475636e83d
    // @note: Does this fix the problem of messages being delayed while num ticks increase ?
    double sleepDuration =
      (double) this->audioConfig_->blockSize / (double) this->audioConfig_->sampleRate;
    // double sleepDuration = this->audioConfig_->bufferDuration;
    Pa_Sleep(sleepDuration * 1000.0f);
  }
}

// this is called in the js event loop
void BackgroundProcess::OnProgress(const uint32_t* data, size_t size) {
  // dequeue messages
  while (!this->msgReceiveQueue_->empty()) {
    auto ptr = this->msgReceiveQueue_->pop();

    Napi::Value channel = Napi::String::New(Env(), ptr->channel);

    switch (ptr->type) {
      case PD_MSG_TYPES::BANG_MSG: {
        Callback().Call({ channel });
        break;
      }

      case PD_MSG_TYPES::FLOAT_MSG: {
        Napi::Value num = Napi::Number::New(Env(), ptr->num);
        Callback().Call({ channel, num });
        break;
      }

      case PD_MSG_TYPES::SYMBOL_MSG: {
        Napi::Value symbol = Napi::String::New(Env(), ptr->symbol);
        Callback().Call({ channel, symbol });
        break;
      }

      // // @note - not used: print an OSC-style type string
      case PD_MSG_TYPES::LIST_MSG: {
        const int len = ptr->list.len();
        Napi::Array list = Napi::Array::New(Env(), len);

        for (int i = 0; i < len; i++) {
          if (ptr->list.isFloat(i)) {
            Napi::Value num = Napi::Number::New(Env(), ptr->list.getFloat(i));
            list.Set(i, num);
          } else if (ptr->list.isSymbol(i)) {
            Napi::Value symbol = Napi::String::New(Env(), ptr->list.getSymbol(i));
            list.Set(i, symbol);
          }
        }

        Callback().Call({ channel, list });
        break;
      }
    }
  }
}

void BackgroundProcess::OnOK() {
  #ifdef DEBUG
    std::cout << "[node-libpd] background process terminated" << std::endl;
  #endif
}

}; // namespace


