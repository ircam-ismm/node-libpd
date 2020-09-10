#pragma once

#include <iostream>

#include "libpd/PdBase.hpp"
#include "./PdReceiver.h"
#include "./types.h"

namespace node_lib_pd {

/**
 * Wrapper around pd::PdBase for more simple use in NodePd
 *
 * @todo - clean `closePatches`
 * @todo - should use `std::unordered_map` for `patches_` but refuses to
 *         compile... (looks like c++11 issue)
 */
class PdWrapper {
  public:

    PdWrapper();
    ~PdWrapper();

    pd::PdBase * getLibPdInstance();

    bool init(audio_config_t * config);
    bool isInited();
    // void clear();
    int blockSize();


    patch_infos_t openPatch(const std::string patch, const std::string path);
    patch_infos_t closePatch(int dollarZero);

    void sendMessage(const pd_scheduled_msg_t);
    // void sendBang(const std::string & channel);
    // void sendFloat(const std::string & channel, float value);
    // void sendSymbol(const std::string & channel, const std::string & symbol);

    // void startMessage();
    // void addFloat(const float num);
    // void addSymbol(const std::string & symbol);
    // void finishList(const std::string & dest);

    void setReceiver(PdReceiver * receiver);
    void subscribe(const std::string & channel);
    void unsubscribe(const std::string & channel);

    int arraySize(const std::string & name);
    bool writeArray(const std::string& name, std::vector<float>& source, int writeLen=-1, int offset=0);
    bool readArray(const std::string& name, std::vector<float>& dest, int readLen=-1, int offset=0);
    // void unsubscribe(const std::string & channel);
    // void unsubscribe(const std::string & channel);

  private:
    pd::PdBase * pd_;
    std::map<int, pd::Patch> patches_;

    patch_infos_t createPatchInfos_(pd::Patch);
};

}; // namespace
