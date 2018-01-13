#include "./PdReceiver.hpp"

namespace nodePd {

PdReceiver::PdReceiver(LockedQueue<pd_msg_t> * msgQueue)
  : msgQueue_(msgQueue)
{}

//--------------------------------------------------------------
void PdReceiver::print(const std::string& message) {
  std::cout << message << std::endl;
}

//--------------------------------------------------------------
void PdReceiver::receiveBang(const std::string & channel) {
  auto ptr = std::make_shared<pd_msg_t>(channel);
  this->msgQueue_->push(ptr);
}

void PdReceiver::receiveFloat(const std::string & channel, float num) {
  auto ptr = std::make_shared<pd_msg_t>(channel, num);
  this->msgQueue_->push(ptr);
}

void PdReceiver::receiveSymbol(const std::string & channel, const std::string & symbol) {
  auto ptr = std::make_shared<pd_msg_t>(channel, symbol);
  this->msgQueue_->push(ptr);
}

void PdReceiver::receiveList(const std::string & channel, const pd::List & list) {
  auto ptr = std::make_shared<pd_msg_t>(channel, list);
  this->msgQueue_->push(ptr);
}

// void PdReceiver::receiveMessage(const std::string& channel, const std::string& msg, const pd::List& list) {
//   std::cout << "CPP: message " << channel << ": " << msg << " " << list.toString() << list.types() << std::endl;
// }

// //--------------------------------------------------------------
// void PdReceiver::receiveNoteOn(const int channel, const int pitch, const int velocity) {
//   std::cout << "CPP MIDI: note on: " << channel << " " << pitch << " " << velocity << std::endl;
// }

// void PdReceiver::receiveControlChange(const int channel, const int controller, const int value) {
//   std::cout << "CPP MIDI: control change: " << channel << " " << controller << " " << value << std::endl;
// }

// void PdReceiver::receiveProgramChange(const int channel, const int value) {
//   std::cout << "CPP MIDI: program change: " << channel << " " << value << std::endl;
// }

// void PdReceiver::receivePitchBend(const int channel, const int value) {
//   std::cout << "CPP MIDI: pitch bend: " << channel << " " << value << std::endl;
// }

// void PdReceiver::receiveAftertouch(const int channel, const int value) {
//   std::cout << "CPP MIDI: aftertouch: " << channel << " " << value << std::endl;
// }

// void PdReceiver::receivePolyAftertouch(const int channel, const int pitch, const int value) {
//   std::cout << "CPP MIDI: poly aftertouch: " << channel << " " << pitch << " " << value << std::endl;
// }

// void PdReceiver::receiveMidiByte(const int port, const int byte) {
//   std::cout << "CPP MIDI: midi byte: " << port << " " << byte << std::endl;
// }

}; // namespace

