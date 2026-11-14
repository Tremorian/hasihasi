// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace hh::audio {

// Lock-free SPSC Ringpuffer für Audio-Frames
template<typename T, size_t Capacity>
class RingBuffer {
public:
  RingBuffer() = default;

  // RT-safe: keine Allokationen, keine Locks
  bool push(const T& item) {
    size_t write_pos = write_pos_.load(std::memory_order_relaxed);
    size_t next = (write_pos + 1) % Capacity;
    if (next == read_pos_.load(std::memory_order_acquire)) return false; // voll
    buffer_[write_pos] = item;
    write_pos_.store(next, std::memory_order_release);
    return true;
  }

  bool pop(T& item) {
    size_t read_pos = read_pos_.load(std::memory_order_relaxed);
    if (read_pos == write_pos_.load(std::memory_order_acquire)) return false; // leer
    item = buffer_[read_pos];
    read_pos_.store((read_pos + 1) % Capacity, std::memory_order_release);
    return true;
  }

  size_t size() const {
    size_t w = write_pos_.load(std::memory_order_acquire);
    size_t r = read_pos_.load(std::memory_order_acquire);
    return (w >= r) ? (w - r) : (Capacity - r + w);
  }

  bool empty() const {
    return read_pos_.load(std::memory_order_acquire) == write_pos_.load(std::memory_order_acquire);
  }

  bool full() const {
    return size() == Capacity - 1;
  }

private:
  std::array<T, Capacity> buffer_{};
  alignas(64) std::atomic<size_t> read_pos_ = {0};
  alignas(64) std::atomic<size_t> write_pos_ = {0};
};

// Opus-Encoder-Wrapper (libopus)
class OpusEncoder {
public:
  OpusEncoder();
  ~OpusEncoder();

  // Nicht kopierbar, bewegbar
  OpusEncoder(const OpusEncoder&) = delete;
  OpusEncoder& operator=(const OpusEncoder&) = delete;
  OpusEncoder(OpusEncoder&&) noexcept;
  OpusEncoder& operator=(OpusEncoder&&) noexcept;

  // Initialisiert den Encoder mit Default-Parametern (48kHz, mono, 10ms)
  bool initialize(int sample_rate = 48000, int channels = 1, int frame_size_ms = 10);

  // Kodiert PCM zu Opus-Bits
  // PCM: 16-bit signed, little-endian
  // Gibt komprimierte Opus-Pakete zurück
  std::vector<uint8_t> encode(std::span<const int16_t> pcm);

  int sample_rate() const { return sample_rate_; }
  int channels() const { return channels_; }
  int frame_size() const { return frame_size_; }

private:
  struct OpusEncoderState; // PIMPL für libopus
  OpusEncoderState* state_ = nullptr;
  int sample_rate_ = 48000;
  int channels_ = 1;
  int frame_size_ = 480; // 10ms @ 48kHz
};

// Opus-Decoder-Wrapper (libopus)
class OpusDecoder {
public:
  OpusDecoder();
  ~OpusDecoder();

  OpusDecoder(const OpusDecoder&) = delete;
  OpusDecoder& operator=(const OpusDecoder&) = delete;
  OpusDecoder(OpusDecoder&&) noexcept;
  OpusDecoder& operator=(OpusDecoder&&) noexcept;

  // Initialisiert den Decoder (muss gleiche Parameter wie Encoder haben)
  bool initialize(int sample_rate = 48000, int channels = 1);

  // Dekodiert Opus-Bits zu PCM
  std::vector<int16_t> decode(std::span<const uint8_t> opus_data);

  int sample_rate() const { return sample_rate_; }
  int channels() const { return channels_; }

private:
  struct OpusDecoderState; // PIMPL für libopus
  OpusDecoderState* state_ = nullptr;
  int sample_rate_ = 48000;
  int channels_ = 1;
};

// Audio-Frame mit Zeitstempel für Latenzmessung
struct AudioFrame {
  std::int64_t timestamp_ns = 0; // Nanosekunden
  std::vector<std::int16_t> data;
};

} // namespace hh::audio
