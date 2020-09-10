#include "./PdWrapper.h"

namespace node_lib_pd {

PdWrapper::PdWrapper() {
  this->pd_ = new pd::PdBase();
}

PdWrapper::~PdWrapper() {
  #ifdef DEBUG
    std::cout << "[node-libpd] clear and delete pd instance" << std::endl;
  #endif
  this->pd_->clear();
  delete this->pd_;
}

pd::PdBase * PdWrapper::getLibPdInstance() {
  return this->pd_;
}

// --------------------------------------------------------------------------
// INITIALIZATION
// --------------------------------------------------------------------------

bool PdWrapper::init(audio_config_t * config) {
  if (this->pd_->isInited())
    return false;

  const int numInputChannels = config->numInputChannels;
  const int numOutputChannels = config->numOutputChannels;
  const int sampleRate = config->sampleRate;
  const bool queued = false;

  // return 1 if setup successfully
  const int initialized = this->pd_->init(numInputChannels, numOutputChannels, sampleRate, queued);

  if (initialized == 0)
    return false;

  this->pd_->computeAudio(true);
  return true;
}

bool PdWrapper::isInited() {
  return this->pd_->isInited();
}

int PdWrapper::blockSize() {
  return this->pd_->blockSize();
}

// --------------------------------------------------------------------------
// PATCH
// --------------------------------------------------------------------------

patch_infos_t PdWrapper::openPatch(const std::string filename, const std::string path) {
  pd::Patch patch = this->pd_->openPatch(filename, path);

  if (patch.isValid()) {
    std::pair<int, pd::Patch> element = { patch.dollarZero(), patch };
    this->patches_.insert(element);
  };

  return this->createPatchInfos_(patch);
}

patch_infos_t PdWrapper::closePatch(int dollarZero) {
  patch_infos_t emptyPatch;
  // patch invalid or already closed
  if (dollarZero != 0) {
    auto search = this->patches_.find(dollarZero);

    if (search != this->patches_.end()) {
      pd::Patch patch = search->second;

      this->pd_->closePatch(patch);
      this->patches_.erase(search);

      return this->createPatchInfos_(patch);
    }

    return emptyPatch;
  }

  return emptyPatch;
}

patch_infos_t PdWrapper::createPatchInfos_(pd::Patch patch) {
  patch_infos_t patchInfos;

  // @note - ignore `dollarZeroString` as it makes no sens in js
  patchInfos.isValid = patch.isValid();
  patchInfos.filename = patch.filename();
  patchInfos.path = patch.path();
  patchInfos.dollarZero = patch.dollarZero();

  return patchInfos;
}

// --------------------------------------------------------------------------
// COMMUNICATIONS
// --------------------------------------------------------------------------

// send to pd
void PdWrapper::sendMessage(const pd_scheduled_msg_t msg) {
  switch (msg.type) {
    case PD_MSG_TYPES::BANG_MSG:
      this->pd_->sendBang(msg.channel);
      break;
    case PD_MSG_TYPES::FLOAT_MSG:
      this->pd_->sendFloat(msg.channel, msg.num);
      break;
    case PD_MSG_TYPES::SYMBOL_MSG:
      this->pd_->sendSymbol(msg.channel, msg.symbol);
      break;
    case PD_MSG_TYPES::LIST_MSG:
      this->pd_->sendList(msg.channel, msg.list);
      break;
  }
}


// receive from pd
void PdWrapper::setReceiver(PdReceiver * receiver) {
  this->pd_->setReceiver(receiver);
}

void PdWrapper::subscribe(const std::string & channel) {
  this->pd_->subscribe(channel);
}

void PdWrapper::unsubscribe(const std::string & channel) {
  this->pd_->unsubscribe(channel);
}

// arrays
int PdWrapper::arraySize(const std::string & size) {
  return this->pd_->arraySize(size);
}

bool PdWrapper::writeArray(const std::string& name, std::vector<float>& source, int writeLen, int offset) {
  return this->pd_->writeArray(name, source, writeLen, offset);
}

bool PdWrapper::readArray(const std::string& name, std::vector<float>& dest, int readLen, int offset) {
  return this->pd_->readArray(name, dest, readLen, offset);
}

} // namespace
