#pragma once

#include "./types.h"
#include "libpd/PdBase.hpp"
#include "portaudio.h"

namespace node_lib_pd {

/**
 * portaudio wrapper
 * adapted from:
 * http://portaudio.com/docs/v19-doxydocs/paex__sine__c_09_09_8cpp_source.html
 */
class PaWrapper {
public:
  PaWrapper();
  ~PaWrapper();

  /**
   * init the port audio stream and store the informations needed in audio
   * callback
   */
  bool init(audio_config_t *audioConfig, pd::PdBase *pd);
  // void clear();
  int getDeviceCount();
  const PaDeviceInfo *getDeviceAtIndex(int index);

  /**
   * accessor for the PaStream
   */
  PaStream *getStream();

  /**
   * last `Pa::timeInfo->outputBufferDacTime`
   */
  double currentTime;

private:
  audio_config_t *audioConfig_;
  pd::PdBase *pd_;

  PaError paInitErr_;
  PaStream *paStream_;

  /**
   * The instance callback, where we have access to every method/variable in
   * object of class Sine
   */
  int paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags);

  /**
   * This routine will be called by the PortAudio engine when audio is needed.
   * It may called at interrupt level on some machines so don't do anything
   * that could mess up the system like calling malloc() or free().
   */
  static int paCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags, void *userData) {
    return ((PaWrapper *)userData)
        ->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer, timeInfo,
                           statusFlags);
  }
};

}; // namespace node_lib_pd
