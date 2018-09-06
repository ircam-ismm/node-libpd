#include "./PaWrapper.hpp"

namespace nodePd {

PaWrapper::PaWrapper()
  : paInitErr_(Pa_Initialize())
  , currentTime(0)
{}

PaWrapper::~PaWrapper()
{
  if (this->paInitErr_ == paNoError)
    Pa_Terminate();
}

PaStream * PaWrapper::getStream()
{
  return this->paStream_;
}

bool PaWrapper::init(audio_config_t * audioConfig, pd::PdBase * pd)
{
  this->audioConfig_ = audioConfig;
  this->pd_ = pd;

  const int numInputChannels = audioConfig->numInputChannels;
  const int numOutputChannels = audioConfig->numOutputChannels;
  const int sampleRate = audioConfig->sampleRate;
  const int framesPerBuffer = audioConfig->framesPerBuffer;

  if (this->paInitErr_ != paNoError) {
    std::cout << "An error occured while using the portaudio stream" << std::endl;
    std::cout << "Error number: " << this->paInitErr_ << std::endl;
    std::cout << "Error message: " << Pa_GetErrorText(this->paInitErr_) << std::endl;
    return false;
  }

  std::cout << "------------------------------------------------" << std::endl;
  std::cout << ">>> Config:" << std::endl;
  std::cout << "numInputChannels: " << audioConfig->numInputChannels << std::endl;
  std::cout << "numOutputChannels: " << audioConfig->numOutputChannels << std::endl;
  std::cout << "sampleRate: " << audioConfig->sampleRate << std::endl;
  std::cout << "blockSize: " << audioConfig->blockSize << std::endl;
  std::cout << "ticks: " << audioConfig->ticks << std::endl;
  std::cout << "framesPerBuffer: " << audioConfig->framesPerBuffer << std::endl;
  std::cout.precision(9);
  std::cout << "bufferDuration: " << std::fixed << audioConfig->bufferDuration << std::endl;

  PaStreamParameters inputParameters;
  PaStreamParameters outputParameters;

  // -------------------------------------------------------------
  // INPUT PARAMETERS
  // -------------------------------------------------------------

  if (numInputChannels > 0) {
    PaDeviceIndex index = Pa_GetDefaultInputDevice();

    inputParameters.device = index;

    if (inputParameters.device == paNoDevice) {
      std::cout << "[Error] No device found" << std::endl;
      return false;
    }

    const PaDeviceInfo * pInputInfo = Pa_GetDeviceInfo(index);

    if (pInputInfo != 0) {
      std::cout << std::endl;
      std::cout << ">>> Input:" << std::endl;
      std::cout << "Input device name: " << pInputInfo->name << std::endl;
      std::cout << "DefaultLowInputLatency: " << pInputInfo->defaultLowOutputLatency << std::endl;
      std::cout << "DefaultHighInputLatency: " << pInputInfo->defaultHighOutputLatency << std::endl;
    }

    inputParameters.channelCount = numInputChannels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = pInputInfo->defaultLowOutputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
  }

  // -------------------------------------------------------------
  // OUTPUT PARAMETERS
  // -------------------------------------------------------------

  if (numOutputChannels > 0) {
    PaDeviceIndex index = Pa_GetDefaultOutputDevice();

    outputParameters.device = index;

    if (outputParameters.device == paNoDevice) {
      std::cout << "[Error] No device found" << std::endl;
      return false;
    }

    const PaDeviceInfo * pOutputInfo = Pa_GetDeviceInfo(index);

    if (pOutputInfo != 0) {
      std::cout << std::endl;
      std::cout << ">>> Ouput:" << std::endl;
      std::cout << "Output device name: " << pOutputInfo->name << std::endl;
      std::cout << "DefaultLowOutputLatency: " << pOutputInfo->defaultLowOutputLatency << std::endl;
      std::cout << "DefaultHighOutputLatency: " << pOutputInfo->defaultHighOutputLatency << std::endl;
    }

    outputParameters.channelCount = numOutputChannels; // numOutputChannels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = pOutputInfo->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
  }

  std::cout << "------------------------------------------------" << std::endl;

  PaError err;

  err = Pa_OpenStream(
    &this->paStream_,
    audioConfig->numInputChannels > 0 ? &inputParameters : NULL,
    audioConfig->numOutputChannels > 0 ? &outputParameters : NULL,
    sampleRate,
    framesPerBuffer,
    paClipOff, // we won't output out of range samples so don't bother clipping them
    &PaWrapper::paCallback,
    this // Using 'this' for userData so we can cast to LibPdWorker* in paCallback method
  );

  if (err != paNoError) {
    std::cout << "[Error] Failed to open stream" << std::endl;
    std::cout << "Error number: " << err << std::endl;
    std::cout << "Error message: " << Pa_GetErrorText(err) << std::endl;
    return false;
  }

  err = Pa_StartStream(this->paStream_);

  if (err != paNoError) {
    std::cout << "[Error] Failed to start stream" << std::endl;
    std::cout << "Error number: " << err << std::endl;
    std::cout << "Error message: " << Pa_GetErrorText(err) << std::endl;
  }

  return true;
}

int PaWrapper::paCallbackMethod(
  const void * inputBuffer,
  void * outputBuffer,
  unsigned long framesPerBuffer,
  const PaStreamCallbackTimeInfo * timeInfo,
  PaStreamCallbackFlags statusFlags)
{
  float * in = (float *) inputBuffer;
  float * out = (float *) outputBuffer;

  this->pd_->processFloat(this->audioConfig_->ticks, in, out);

  this->currentTime += (double) framesPerBuffer / (double) this->audioConfig_->sampleRate;

  return paContinue;
}

}; // namespace

