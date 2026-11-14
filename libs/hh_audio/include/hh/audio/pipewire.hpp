// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace hh::audio {

// PipeWire-Capture-Gerät
// Realisiert PipeWire-Capture mit pw_stream und Ringpuffer
class PipeWireCapture {
public:
  PipeWireCapture();
  ~PipeWireCapture();

  PipeWireCapture(const PipeWireCapture&) = delete;
  PipeWireCapture& operator=(const PipeWireCapture&) = delete;

  // Initialisiert Capture mit angegebenem Gerät (leer = Standard-Mikrofon)
  bool open(const std::string& device_name = "");

  // Startet/Stoppt Capture
  void start();
  void stop();

  // Gibt aktuellen Puffer-Status zurück
  bool is_open() const { return impl_ != nullptr; }
  bool is_running() const { return running_; }

  using SampleReadyCallback = std::function<void(std::span<const int16_t>)>;
  void set_callback(SampleReadyCallback cb);

private:
  // PipeWire-Callback für Audio-Daten (wird aus PW-RT-Thread aufgerufen)
  static void on_process(void* user, uint32_t length);
  static void on_state_changed(void* user, enum pw_stream_state old,
                                enum pw_stream_state state, const char* error);

  struct Impl;
  std::unique_ptr<Impl> impl_;
  bool running_ = false;
  SampleReadyCallback callback_;
};

// PipeWire-Playback-Gerät
// Realisiert PipeWire-Playback mit pw_stream und Ringpuffer
class PipeWirePlayback {
public:
  PipeWirePlayback();
  ~PipeWirePlayback();

  PipeWirePlayback(const PipeWirePlayback&) = delete;
  PipeWirePlayback& operator=(const PipeWirePlayback&) = delete;

  // Initialisiert Playback mit angegebenem Gerät (leer = Standard-Lautsprecher)
  bool open(const std::string& device_name = "");

  // Startet/Stoppt Playback
  void start();
  void stop();

  // Gibt aktuellen Puffer-Status zurück
  bool is_open() const { return impl_ != nullptr; }
  bool is_running() const { return running_; }

  // Schreibt Audio-Daten in den Playback-Puffer
  // Wird aus dem DSP-Thread aufgerufen
  void write(std::span<const int16_t> data);

  using BufferReadyCallback = std::function<void(std::span<int16_t>)>;
  void set_buffer_callback(BufferReadyCallback cb);

private:
  // PipeWire-Callback für Audio-Daten (wird aus PW-RT-Thread aufgerufen)
  static void on_process(void* user, uint32_t length);
  static void on_state_changed(void* user, enum pw_stream_state old,
                                enum pw_stream_state state, const char* error);

  struct Impl;
  std::unique_ptr<Impl> impl_;
  bool running_ = false;
  BufferReadyCallback buffer_callback_;
};

} // namespace hh::audio
