#pragma once

#include "libpd/PdBase.hpp"
#include "portaudio.h"

namespace nodePd {

typedef struct audio_config_s {
  int numInputChannels;
  int numOutputChannels;
  int sampleRate;
  int blockSize;
  int ticks;
  int framesPerBuffer; // blockSize * ticks
  double bufferDuration;
} audio_config_t;

typedef struct patch_infos_s {
  bool isValid = 0;
  std::string filename = "";
  std::string path = "";
  int dollarZero = 0;
} patch_infos_t;

/**
 * Pack all messages in the same struct. This is kind of brut force
 * but using derived struct leads to weird (and not understood) behavior
 * (i.e when `dynamic_pointer_cast` the attribute values are sometime empty...,
 * but not always...)
 *
 * @todo - refactor to be more memory efficient
 */
enum class PD_MSG_TYPES {
  BANG_MSG,
  FLOAT_MSG,
  SYMBOL_MSG,
  LIST_MSG,
};

struct pd_msg_t {
  pd_msg_t(std::string c)
    : type(PD_MSG_TYPES::BANG_MSG)
    , channel(c) {};

  pd_msg_t(std::string c, float n)
    : type(PD_MSG_TYPES::FLOAT_MSG)
    , channel(c)
    , num(n) {};

  pd_msg_t(std::string c, std::string s)
    : type(PD_MSG_TYPES::SYMBOL_MSG)
    , channel(c)
    , symbol(s) {};

  pd_msg_t(std::string c, pd::List l)
    : type(PD_MSG_TYPES::LIST_MSG)
    , channel(c)
    , list(l) {};

  PD_MSG_TYPES type;
  std::string channel;
  float num;
  std::string symbol = ""; // should be a ref, but crash at compile time
  pd::List list; // should be a ref, but crash at compile time
};

// for time
struct pd_scheduled_msg_t : pd_msg_t {
  pd_scheduled_msg_t(std::string c, double t)
    : pd_msg_t(c)
    , time(t) {};

  pd_scheduled_msg_t(std::string c, double t, float n)
    : pd_msg_t(c, n)
    , time(t) {};

  pd_scheduled_msg_t(std::string c, double t, std::string s)
    : pd_msg_t(c, s)
    , time(t) {};

  pd_scheduled_msg_t(std::string c, double t, pd::List l)
    : pd_msg_t(c, l)
    , time(t) {};

  double time;
};

struct compare_msg_time_t {
  bool operator()(pd_scheduled_msg_t const & msg1, pd_scheduled_msg_t const & msg2) {
    return msg1.time > msg2.time;
  }
};

}; // namespace
