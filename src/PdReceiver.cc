#include "./PdReceiver.h"

namespace node_lib_pd {

PdReceiver::PdReceiver(LockedQueue<pd_msg_t> *msgQueue) : msgQueue_(msgQueue) {}

PdReceiver::~PdReceiver() {}

//--------------------------------------------------------------
void PdReceiver::print(const std::string &message) {
#ifdef DEBUG
  std::cout << message << std::endl;
#endif
  auto ptr = std::make_shared<pd_msg_t>("print", message);
  this->msgQueue_->push(ptr);
}

//--------------------------------------------------------------
void PdReceiver::receiveBang(const std::string &channel) {
  auto ptr = std::make_shared<pd_msg_t>(channel);
  this->msgQueue_->push(ptr);
}

void PdReceiver::receiveFloat(const std::string &channel, float num) {
  auto ptr = std::make_shared<pd_msg_t>(channel, num);
  this->msgQueue_->push(ptr);
}

void PdReceiver::receiveSymbol(const std::string &channel,
                               const std::string &symbol) {
  auto ptr = std::make_shared<pd_msg_t>(channel, symbol);
  this->msgQueue_->push(ptr);
}

void PdReceiver::receiveList(const std::string &channel, const pd::List &list) {
  auto ptr = std::make_shared<pd_msg_t>(channel, list);
  this->msgQueue_->push(ptr);
}

}; // namespace node_lib_pd
