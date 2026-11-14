// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/audio/pipewire.hpp"

namespace hh::audio {

PipeWireCapture::PipeWireCapture() = default;
PipeWireCapture::~PipeWireCapture() = default;

bool PipeWireCapture::open(const std::string& device_name) {
  // TODO: pw_stream erstellen, node connect
  (void)device_name;
  return true;
}

void PipeWireCapture::read() {
  // RT-safe: liest aus PipeWire-Stream
}

void PipeWireCapture::set_callback(SampleReadyCallback cb) {
  // TODO
  (void)cb;
}

PipeWirePlayback::PipeWirePlayback() = default;
PipeWirePlayback::~PipeWirePlayback() = default;

bool PipeWirePlayback::open(const std::string& device_name) {
  // TODO: pw_stream erstellen, node connect
  (void)device_name;
  return true;
}

void PipeWirePlayback::write() {
  // RT-safe: schreibt in PipeWire-Stream
}

void PipeWirePlayback::set_callback(BufferReadyCallback cb) {
  // TODO
  (void)cb;
}

} // namespace hh::audio
