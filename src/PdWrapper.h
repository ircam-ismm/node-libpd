#pragma once

#include <iostream>

#include "libpd/PdBase.hpp"
#include "./PdReceiver.h"
#include "./types.h"

namespace node_lib_pd {

class PdWrapper {
  public:

    PdWrapper();
    ~PdWrapper();

    pd::PdBase * getLibPdInstance();

    bool init(audio_config_t * config);
    bool isInited();
    int blockSize();

    patch_infos_t openPatch(const std::string patch, const std::string path);
    patch_infos_t closePatch(int dollarZero);
    void addToSearchPath(const std::string path);
    void clearSearchPath();

    void setReceiver(PdReceiver * receiver);
    void subscribe(const std::string& channel);
    void unsubscribe(const std::string& channel);
    void sendMessage(const pd_scheduled_msg_t);
    // void sendBang(const std::string & channel);
    // void sendFloat(const std::string & channel, float value);
    // void sendSymbol(const std::string & channel, const std::string & symbol);
    // void startMessage();
    // void addFloat(const float num);
    // void addSymbol(const std::string & symbol);
    // void finishList(const std::string & dest);

    int arraySize(const std::string& name);
    bool writeArray(const std::string& name, std::vector<float>& source, int writeLen=-1, int offset=0);
    bool readArray(const std::string& name, std::vector<float>& dest, int readLen=-1, int offset=0);
    void clearArray(const std::string& name, int value=0);

  private:
    pd::PdBase * pd_;
    std::map<int, pd::Patch> patches_;

    patch_infos_t createPatchInfos_(pd::Patch);
};

}; // namespace
