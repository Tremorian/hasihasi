// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace hh::proto {

// Protobuf-Framing: [u16 type][u32 length][payload]
class FrameBuilder {
public:
  FrameBuilder() = default;

  void set_type(uint16_t type);
  void set_payload(std::span<const uint8_t> payload);

  std::vector<uint8_t> build() const;
  static std::vector<uint8_t> build(uint16_t type, std::span<const uint8_t> payload);

private:
  uint16_t type_ = 0;
  std::vector<uint8_t> payload_;
};

// Frame-Parser für eingehende Nachrichten
class FrameParser {
public:
  bool feed(std::span<const uint8_t> data);

  uint16_t type() const { return type_; }
  std::span<const uint8_t> payload() const { return payload_; }
  bool done() const { return done_; }

private:
  uint16_t type_ = 0;
  uint32_t length_ = 0;
  std::vector<uint8_t> payload_;
  uint32_t offset_ = 0;
  bool done_ = false;
  enum class State { READ_TYPE, READ_LENGTH, READ_PAYLOAD } state_ = State::READ_TYPE;
};

} // namespace hh::proto
