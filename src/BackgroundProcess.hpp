#pragma once

#include <iostream>

#include "nan.h"
#include "portaudio.h"
#include "libpd/PdBase.hpp"
#include "./types.hpp"
#include "./LockedQueue.hpp"

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
        pd::PdBase * pd,
        PaStream * paStream);
    ~BackgroundProcess();

    // async worker signature
    void Execute(const Nan::AsyncProgressWorker::ExecutionProgress & progress);
    void HandleProgressCallback(const char * data, size_t size);
    void HandleOkCallback();

  private:
    Nan::Callback * onProgress_;

    audio_config_t * audioConfig_;
    LockedQueue<pd_msg_t> * msgQueue_;
    pd::PdBase * pd_;
    PaStream * paStream_;
};

}; // namespace
