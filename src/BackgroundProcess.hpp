#pragma once

#include <iostream>

#include "nan.h"
#include "portaudio.h"
#include "libpd/PdBase.hpp"
#include "./types.hpp"
#include "./LockedQueue.hpp"
#include "./PaWrapper.hpp"
#include "./PdWrapper.hpp"

namespace nodePd {

/**
 * maintain PA_Stream alive in background process
 *
 * @todo - extends Nan::AsyncProgressQueueWorker
 * @todo - add a reference to flag to be able to close the process
 */
class BackgroundProcess : public Nan::AsyncProgressWorker
{
  public:
    // BackgroundProcess();
    BackgroundProcess(
        Nan::Callback * callback,
        Nan::Callback * onProgress,
        audio_config_t * audioConfig,
        LockedQueue<pd_msg_t> * msgQueue,
        PaWrapper * paWrapper,
        PdWrapper * pdWrapper);
    ~BackgroundProcess();

    // void scheduleMsg(timed_msg_t);
    // async worker signature
    void Execute(const Nan::AsyncProgressWorker::ExecutionProgress & progress);
    void HandleProgressCallback(const char * data, size_t size);
    void HandleOkCallback();

  private:
    Nan::Callback * onProgress_;

    audio_config_t * audioConfig_;
    LockedQueue<pd_msg_t> * msgQueue_;
    PaWrapper * paWrapper_;
    PdWrapper * pdWrapper_;
};

}; // namespace
