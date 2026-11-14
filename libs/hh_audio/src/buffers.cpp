// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/audio/buffers.hpp"

namespace hh::audio {

OpusEncoder::OpusEncoder() = default;
OpusEncoder::~OpusEncoder() {
  if (codec_) {
    // opus_encoder_destroy(codec_);
    codec_ = nullptr;
  }
}

OpusEncoder::OpusEncoder(OpusEncoder&& other) noexcept
  : codec_(other.codec_)
  , sample_rate_(other.sample_rate_)
  , channels_(other.channels_)
  , frame_size_(other.frame_size_) {
  other.codec_ = nullptr;
}

OpusEncoder& OpusEncoder::operator=(OpusEncoder&& other) noexcept {
  if (this != &other) {
    if (codec_) opus_encoder_destroy(codec_);
    codec_ = other.codec_;
    sample_rate_ = other.sample_rate_;
    channels_ = other.channels_;
    frame_size_ = other.frame_size_;
    other.codec_ = nullptr;
  }
  return *this;
}

bool OpusEncoder::initialize(int sample_rate, int channels, int frame_size_ms) {
  sample_rate_ = sample_rate;
  channels_ = channels;
  frame_size_ = sample_rate * frame_size_ms / 1000;
  // TODO: opus_encoder_create()
  return true;
}

std::vector<uint8_t> OpusEncoder::encode(std::span<const int16_t> pcm) {
  std::vector<uint8_t> output;
  // TODO: opus_encode()
  return output;
}

OpusDecoder::OpusDecoder() = default;
OpusDecoder::~OpusDecoder() {
  if (codec_) {
    // opus_decoder_destroy(codec_);
    codec_ = nullptr;
  }
}

OpusDecoder::OpusDecoder(OpusDecoder&& other) noexcept
  : codec_(other.codec_)
  , sample_rate_(other.sample_rate_)
  , channels_(other.channels_) {
  other.codec_ = nullptr;
}

OpusDecoder& OpusDecoder::operator=(OpusDecoder&& other) noexcept {
  if (this != &other) {
    if (codec_) opus_decoder_destroy(codec_);
    codec_ = other.codec_;
    sample_rate_ = other.sample_rate_;
    channels_ = other.channels_;
    other.codec_ = nullptr;
  }
  return *this;
}

bool OpusDecoder::initialize(int sample_rate, int channels) {
  sample_rate_ = sample_rate;
  channels_ = channels;
  // TODO: opus_decoder_create()
  return true;
}

std::vector<int16_t> OpusDecoder::decode(std::span<const uint8_t> opus_data) {
  std::vector<int16_t> output;
  // TODO: opus_decode()
  return output;
}

} // namespace hh::audio
