#pragma once

#include <iostream>
#include <queue>
#include <mutex>
#include <napi.h>

#include "portaudio.h"
#include "libpd/PdBase.hpp"
#include "./types.h"
#include "./LockedQueue.h"
#include "./PaWrapper.h"
#include "./PdWrapper.h"

namespace node_lib_pd {

/**
 * maintain PA_Stream alive in background process
 *
 * @todo - add a reference to be able to kill the process
 */
class BackgroundProcess : public Napi::AsyncProgressWorker<uint32_t>
{
  public:
    BackgroundProcess(
        Napi::Function& callback,
        audio_config_t* audioConfig,
        LockedQueue<pd_msg_t>* msgQueue,
        PaWrapper* paWrapper,
        PdWrapper* pdWrapper);
    ~BackgroundProcess();

    void addScheduledMessage(pd_scheduled_msg_t);

    // This code will be executed on the worker thread
    void Execute(const BackgroundProcess::ExecutionProgress& progress);
    void OnProgress(const uint32_t* data, size_t size);
    void OnOK(); // not mandatory

  private:
    audio_config_t * audioConfig_;
    LockedQueue<pd_msg_t> * msgReceiveQueue_;
    PaWrapper * paWrapper_;
    PdWrapper * pdWrapper_;
    std::priority_queue<pd_scheduled_msg_t, std::vector<pd_scheduled_msg_t>, compare_msg_time_t> sendMsgQueue_;

    mutable std::mutex mut_;
};

}; // namespace
