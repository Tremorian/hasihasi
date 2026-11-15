// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/proto/framing.hpp"

#include <cstring>
#include <arpa/inet.h>

namespace hh::proto {

void FrameBuilder::set_type(uint16_t type) {
  type_ = type;
}

void FrameBuilder::set_payload(std::span<const uint8_t> payload) {
  payload_.assign(payload.begin(), payload.end());
}

std::vector<uint8_t> FrameBuilder::build() const {
  return build(type_, payload_);
}

std::vector<uint8_t> FrameBuilder::build(uint16_t type, std::span<const uint8_t> payload) {
  std::vector<uint8_t> frame;
  frame.reserve(6 + payload.size());

  // [u16 type][u32 length][payload]
  uint16_t net_type = htons(type);
  uint32_t net_length = htonl(static_cast<uint32_t>(payload.size()));

  frame.insert(frame.end(),
    reinterpret_cast<const uint8_t*>(&net_type),
    reinterpret_cast<const uint8_t*>(&net_type) + sizeof(net_type));
  frame.insert(frame.end(),
    reinterpret_cast<const uint8_t*>(&net_length),
    reinterpret_cast<const uint8_t*>(&net_length) + sizeof(net_length));
  frame.insert(frame.end(), payload.begin(), payload.end());

  return frame;
}

bool FrameParser::feed(std::span<const uint8_t> data) {
  if (done_) return false;

  for (uint8_t byte : data) {
    switch (state_) {
      case State::READ_TYPE:
        type_ = (type_ << 8) | byte;
        if (offset_ == 1) {
          state_ = State::READ_LENGTH;
          offset_ = 0;
        }
        break;

      case State::READ_LENGTH:
        length_ = (length_ << 8) | byte;
        offset_++;
        if (offset_ == 3) {
          payload_.resize(length_);
          offset_ = 0;
          if (length_ == 0) {
            done_ = true;
            return true;
          }
          state_ = State::READ_PAYLOAD;
        }
        break;

      case State::READ_PAYLOAD:
        payload_[offset_++] = byte;
        if (offset_ == length_) {
          done_ = true;
          return true;
        }
        break;
    }
  }

  return false;
}

} // namespace hh::proto
