// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>

namespace hh::audio {

// PipeWire-Capture-Gerät
class PipeWireCapture {
public:
  PipeWireCapture();
  ~PipeWireCapture();

  PipeWireCapture(const PipeWireCapture&) = delete;
  PipeWireCapture& operator=(const PipeWireCapture&) = delete;

  // Initialisiert Capture mit angegebenem Gerät
  bool open(const std::string& device_name = "");

  // RT-safe: liest in Ringpuffer
  void read();

  using SampleReadyCallback = std::function<void(std::span<const int16_t>)>;
  void set_callback(SampleReadyCallback cb);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// PipeWire-Playback-Gerät
class PipeWirePlayback {
public:
  PipeWirePlayback();
  ~PipeWirePlayback();

  PipeWirePlayback(const PipeWirePlayback&) = delete;
  PipeWirePlayback& operator=(const PipeWirePlayback&) = delete;

  bool open(const std::string& device_name = "");

  // RT-safe: schreibt aus Ringpuffer
  void write();

  using BufferReadyCallback = std::function<void(std::span<int16_t>)>;
  void set_callback(BufferReadyCallback cb);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace hh::audio
