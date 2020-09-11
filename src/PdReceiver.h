#pragma once

#include <iostream>
#include <memory>

#include "PdBase.hpp"
#include "./types.h"
#include "./LockedQueue.h"

namespace node_lib_pd {

// custom receiver class
class PdReceiver : public pd::PdReceiver {

  public:
    PdReceiver(LockedQueue<pd_msg_t> * msgQueue);
    // should be virtual in base class
    virtual ~PdReceiver();

    // pd message receiver callbacks
    void print(const std::string& message);

    void receiveBang(const std::string & dest);
    void receiveFloat(const std::string & dest, float num);
    void receiveSymbol(const std::string & dest, const std::string & symbol);
    void receiveList(const std::string & dest, const pd::List & list);
    // void receiveMessage(const std::string& dest, const std::string& msg, const pd::List& list);

    // pd midi receiver callbacks
    // void receiveNoteOn(const int channel, const int pitch, const int velocity);
    // void receiveControlChange(const int channel, const int controller, const int value);
    // void receiveProgramChange(const int channel, const int value);
    // void receivePitchBend(const int channel, const int value);
    // void receiveAftertouch(const int channel, const int value);
    // void receivePolyAftertouch(const int channel, const int pitch, const int value);
    // void receiveMidiByte(const int port, const int byte);

  private:
    LockedQueue<pd_msg_t> * msgQueue_;
};

}; // namespace
