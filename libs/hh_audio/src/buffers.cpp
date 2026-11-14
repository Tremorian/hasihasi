// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/audio/buffers.hpp"

#include <opus/opus.h>

namespace hh::audio {

// ── OpusEncoder ───────────────────────────────────────────────────────

struct OpusEncoder::OpusEncoderState {
  int dummy = 0;
};

OpusEncoder::OpusEncoder() = default;

OpusEncoder::~OpusEncoder() {
  if (state_) {
    opus_encoder_destroy(static_cast<OpusEncoderState*>(state_));
    state_ = nullptr;
  }
}

OpusEncoder::OpusEncoder(OpusEncoder&& other) noexcept
  : state_(other.state_)
  , sample_rate_(other.sample_rate_)
  , channels_(other.channels_)
  , frame_size_(other.frame_size_) {
  other.state_ = nullptr;
}

OpusEncoder& OpusEncoder::operator=(OpusEncoder&& other) noexcept {
  if (this != &other) {
    if (state_) {
      opus_encoder_destroy(static_cast<OpusEncoderState*>(state_));
    }
    state_ = other.state_;
    sample_rate_ = other.sample_rate_;
    channels_ = other.channels_;
    frame_size_ = other.frame_size_;
    other.state_ = nullptr;
  }
  return *this;
}

bool OpusEncoder::initialize(int sample_rate, int channels, int frame_size_ms) {
  if (state_) {
    opus_encoder_destroy(static_cast<OpusEncoderState*>(state_));
    state_ = nullptr;
  }

  int error = OPUS_OK;
  auto* encoder = opus_encoder_create(sample_rate, channels,
                                       OPUS_APPLICATION_VOIP, &error);
  if (error != OPUS_OK || !encoder) {
    return false;
  }

  // Encoder-Konfiguration (laut Spezifikation Abschnitt 4.1)
  opus_encoder_ctl(encoder, OPUS_SET_BITRATE(40000));       // 40 kbit/s
  opus_encoder_ctl(encoder, OPUS_SET_VBR(1));               // VBR an
  opus_encoder_ctl(encoder, OPUS_SET_VBR_CONSTRAINT(1));    // constrained VBR
  opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(10));       // maximale Komplexität
  opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(1));        // In-Band-FEC an
  opus_encoder_ctl(encoder, OPUS_SET_DTX(1));               // DTX an

  sample_rate_ = sample_rate;
  channels_ = channels;
  frame_size_ = sample_rate * frame_size_ms / 1000;
  state_ = static_cast<OpusEncoderState*>(encoder);

  return true;
}

std::vector<uint8_t> OpusEncoder::encode(std::span<const int16_t> pcm) {
  if (!state_ || pcm.size() < static_cast<size_t>(frame_size_)) {
    return {};
  }

  // Opus-Paketpuffer (max 1275 Bytes pro Paket nach RFC 6716)
  std::vector<uint8_t> output(1275);
  int encoded = opus_encode(static_cast<OpusEncoderState*>(state_),
                             pcm.data(), frame_size_,
                             output.data(), output.size());

  if (encoded < 0) {
    return {};
  }

  output.resize(static_cast<size_t>(encoded));
  return output;
}

// ── OpusDecoder ───────────────────────────────────────────────────────

struct OpusDecoder::OpusDecoderState {
  int dummy = 0;
};

OpusDecoder::OpusDecoder() = default;

OpusDecoder::~OpusDecoder() {
  if (state_) {
    opus_decoder_destroy(static_cast<OpusDecoderState*>(state_));
    state_ = nullptr;
  }
}

OpusDecoder::OpusDecoder(OpusDecoder&& other) noexcept
  : state_(other.state_)
  , sample_rate_(other.sample_rate_)
  , channels_(other.channels_) {
  other.state_ = nullptr;
}

OpusDecoder& OpusDecoder::operator=(OpusDecoder&& other) noexcept {
  if (this != &other) {
    if (state_) {
      opus_decoder_destroy(static_cast<OpusDecoderState*>(state_));
    }
    state_ = other.state_;
    sample_rate_ = other.sample_rate_;
    channels_ = other.channels_;
    other.state_ = nullptr;
  }
  return *this;
}

bool OpusDecoder::initialize(int sample_rate, int channels) {
  if (state_) {
    opus_decoder_destroy(static_cast<OpusDecoderState*>(state_));
    state_ = nullptr;
  }

  int error = OPUS_OK;
  auto* decoder = opus_decoder_create(sample_rate, channels, &error);
  if (error != OPUS_OK || !decoder) {
    return false;
  }

  // PLC (Packet Loss Concealment) ist standardmäßig an
  sample_rate_ = sample_rate;
  channels_ = channels;
  state_ = static_cast<OpusDecoderState*>(decoder);

  return true;
}

std::vector<int16_t> OpusDecoder::decode(std::span<const uint8_t> opus_data) {
  if (!state_) {
    return {};
  }

  // Maximale Dekodierlänge: 48000 Hz * 120 ms / 1000 = 5760 Samples
  std::vector<int16_t> output(5760);
  int decoded = opus_decode(static_cast<OpusDecoderState*>(state_),
                             opus_data.data(), static_cast<int>(opus_data.size()),
                             output.data(), static_cast<int>(output.size()), 0);

  if (decoded < 0) {
    return {};
  }

  output.resize(static_cast<size_t>(decoded * channels_));
  return output;
}

} // namespace hh::audio
