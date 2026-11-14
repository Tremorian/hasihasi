// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/audio/pipewire.hpp"

#include <pipewire/pipewire.h>
#include <pipewire/core.h>
#include <pipewire/stream.h>
#include <pipewire/data-protocol.h>

#include <cstring>
#include <mutex>

namespace hh::audio {

// ── Impl für PipeWireCapture ──────────────────────────────────────────

struct PipeWireCapture::Impl {
  struct pw_loop* loop = nullptr;
  struct pw_context* context = nullptr;
  struct pw_stream* stream = nullptr;
  struct pw_properties* props = nullptr;

  // Ringpuffer für Audio-Daten (256 Frames à 480 Samples = 122880 Samples)
  std::vector<int16_t> buffer;
  size_t buffer_pos = 0;

  // Mutex für Callback-Synchronisation (nicht RT-safe, aber notwendig)
  std::mutex mutex;
};

PipeWireCapture::PipeWireCapture()
  : impl_(std::make_unique<Impl>()) {
  impl_->buffer.resize(122880); // 256 Frames
}

PipeWireCapture::~PipeWireCapture() {
  stop();

  if (impl_->stream) {
    pw_stream_destroy(impl_->stream);
  }
  if (impl_->context) {
    pw_context_destroy(impl_->context);
  }
  if (impl_->loop) {
    pw_loop_destroy(impl_->loop);
  }
}

bool PipeWireCapture::open(const std::string& device_name) {
  if (impl_->loop) {
    return true; // Bereits geöffnet
  }

  // PipeWire-Initialisierung
  pw_init(nullptr, nullptr);

  impl_->loop = pw_loop_new(nullptr);
  if (!impl_->loop) {
    return false;
  }

  impl_->context = pw_context_new(impl_->loop, nullptr, 0);
  if (!impl_->context) {
    pw_loop_destroy(impl_->loop);
    impl_->loop = nullptr;
    return false;
  }

  // Stream-Properties
  impl_->props = pw_properties_new(
    PW_KEY_MEDIA_TYPE, "Audio",
    PW_KEY_MEDIA_CATEGORY, "Capture",
    PW_KEY_MEDIA_ROLE, "Music",
    nullptr);

  if (!device_name.empty()) {
    pw_properties_set(impl_->props, PW_KEY_NODE_NAME, device_name.c_str());
  }

  // Stream-Callbacks
  static const struct pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = on_process,
    .state_changed = on_state_changed,
  };

  impl_->stream = pw_stream_new(impl_->context, "hasihasi-capture", impl_->props);
  if (!impl_->stream) {
    pw_context_destroy(impl_->context);
    pw_loop_destroy(impl_->loop);
    impl_->context = nullptr;
    impl_->loop = nullptr;
    return false;
  }

  pw_stream_add_listener(&impl_->stream->listener,
                          pw_stream_add_output(impl_->stream, &stream_events, this,
                                                sizeof(struct pw_properties*),
                                                &impl_->props));

  return true;
}

void PipeWireCapture::start() {
  if (!impl_->stream || running_) {
    return;
  }

  // Stream-Format setzen (PCM, 16-bit, 48kHz, mono)
  struct pw_audio_info format = {
    .format = PW_AUDIO_FORMAT_S16,
    .rate = 48000,
    .channels = 1,
    .position = {
      [0] = PW_AUDIO_POSITION_MONO
    }
  };

  pw_stream_simple_alloc(impl_->stream, &format, nullptr, 0, nullptr, nullptr);

  pw_stream_connect(impl_->stream, PW_DIRECTION_INPUT, PW_ID_ANY,
                     PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS,
                     nullptr, 0);

  pw_stream_set_active(impl_->stream, true);
  running_ = true;
}

void PipeWireCapture::stop() {
  if (!running_) {
    return;
  }

  if (impl_->stream) {
    pw_stream_set_active(impl_->stream, false);
  }
  running_ = false;
}

void PipeWireCapture::set_callback(SampleReadyCallback cb) {
  callback_ = std::move(cb);
}

void PipeWireCapture::on_process(void* user, uint32_t length) {
  auto* capture = static_cast<PipeWireCapture*>(user);
  auto* impl = capture->impl_.get();

  // PW-Stream-Puffer holen
  struct pw_buffer* buffer = pw_stream_dequeue_buffer(impl->stream);
  if (!buffer) {
    return;
  }

  struct pw_buffer_data* data = buffer->buffer->datas;
  if (data->type != PW_DATA_TYPE_MEMORY) {
    pw_stream_queue_buffer(impl->stream, buffer);
    return;
  }

  struct pw_chunk* chunk = data->chunk;
  if (!chunk || !chunk->data) {
    pw_stream_queue_buffer(impl->stream, buffer);
    return;
  }

  // Audio-Daten verarbeiten
  int16_t* audio_data = static_cast<int16_t*>(data->mem->data);
  size_t sample_count = data->chunk->size / sizeof(int16_t);

  // Callback aufrufen (nicht RT-safe, aber notwendig)
  if (capture->callback_) {
    std::span<int16_t> span(audio_data, sample_count);
    capture->callback_(span);
  }

  pw_stream_queue_buffer(impl->stream, buffer);
}

void PipeWireCapture::on_state_changed(void* user, enum pw_stream_state old,
                                        enum pw_stream_state state,
                                        const char* error) {
  auto* capture = static_cast<PipeWireCapture*>(user);
  (void)old;

  switch (state) {
    case PW_STREAM_STATE_STREAMING:
      capture->running_ = true;
      break;
    case PW_STREAM_STATE_PAUSED:
    case PW_STREAM_STATE_DISCONNECTED:
      capture->running_ = false;
      break;
    default:
      break;
  }

  (void)error;
}

// ── Impl für PipeWirePlayback ─────────────────────────────────────────

struct PipeWirePlayback::Impl {
  struct pw_loop* loop = nullptr;
  struct pw_context* context = nullptr;
  struct pw_stream* stream = nullptr;
  struct pw_properties* props = nullptr;

  // Ringpuffer für Audio-Daten (256 Frames à 480 Samples = 122880 Samples)
  std::vector<int16_t> buffer;
  size_t buffer_pos = 0;

  // Mutex für Callback-Synchronisation
  std::mutex mutex;
};

PipeWirePlayback::PipeWirePlayback()
  : impl_(std::make_unique<Impl>()) {
  impl_->buffer.resize(122880);
}

PipeWirePlayback::~PipeWirePlayback() {
  stop();

  if (impl_->stream) {
    pw_stream_destroy(impl_->stream);
  }
  if (impl_->context) {
    pw_context_destroy(impl_->context);
  }
  if (impl_->loop) {
    pw_loop_destroy(impl_->loop);
  }
}

bool PipeWirePlayback::open(const std::string& device_name) {
  if (impl_->loop) {
    return true; // Bereits geöffnet
  }

  pw_init(nullptr, nullptr);

  impl_->loop = pw_loop_new(nullptr);
  if (!impl_->loop) {
    return false;
  }

  impl_->context = pw_context_new(impl_->loop, nullptr, 0);
  if (!impl_->context) {
    pw_loop_destroy(impl_->loop);
    impl_->loop = nullptr;
    return false;
  }

  impl_->props = pw_properties_new(
    PW_KEY_MEDIA_TYPE, "Audio",
    PW_KEY_MEDIA_CATEGORY, "Playback",
    PW_KEY_MEDIA_ROLE, "Music",
    nullptr);

  if (!device_name.empty()) {
    pw_properties_set(impl_->props, PW_KEY_NODE_NAME, device_name.c_str());
  }

  static const struct pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = on_process,
    .state_changed = on_state_changed,
  };

  impl_->stream = pw_stream_new(impl_->context, "hasihasi-playback", impl_->props);
  if (!impl_->stream) {
    pw_context_destroy(impl_->context);
    pw_loop_destroy(impl_->loop);
    impl_->context = nullptr;
    impl_->loop = nullptr;
    return false;
  }

  pw_stream_add_listener(&impl_->stream->listener,
                          pw_stream_add_output(impl_->stream, &stream_events, this,
                                                sizeof(struct pw_properties*),
                                                &impl_->props));

  return true;
}

void PipeWirePlayback::start() {
  if (!impl_->stream || running_) {
    return;
  }

  struct pw_audio_info format = {
    .format = PW_AUDIO_FORMAT_S16,
    .rate = 48000,
    .channels = 1,
    .position = {
      [0] = PW_AUDIO_POSITION_MONO
    }
  };

  pw_stream_simple_alloc(impl_->stream, &format, nullptr, 0, nullptr, nullptr);

  pw_stream_connect(impl_->stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
                     PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS,
                     nullptr, 0);

  pw_stream_set_active(impl_->stream, true);
  running_ = true;
}

void PipeWirePlayback::stop() {
  if (!running_) {
    return;
  }

  if (impl_->stream) {
    pw_stream_set_active(impl_->stream, false);
  }
  running_ = false;
}

void PipeWirePlayback::write(std::span<const int16_t> data) {
  if (!impl_->stream || !running_) {
    return;
  }

  // Daten in den Puffer schreiben
  std::lock_guard<std::mutex> lock(impl_->mutex);
  for (size_t i = 0; i < data.size() && i < impl_->buffer.size(); ++i) {
    impl_->buffer[i] = data[i];
  }
  impl_->buffer_pos = data.size();
}

void PipeWirePlayback::set_buffer_callback(BufferReadyCallback cb) {
  buffer_callback_ = std::move(cb);
}

void PipeWirePlayback::on_process(void* user, uint32_t length) {
  auto* playback = static_cast<PipeWirePlayback*>(user);
  auto* impl = playback->impl_.get();

  struct pw_buffer* buffer = pw_stream_dequeue_buffer(impl->stream);
  if (!buffer) {
    return;
  }

  struct pw_buffer_data* data = buffer->buffer->datas;
  if (data->type != PW_DATA_TYPE_MEMORY) {
    pw_stream_queue_buffer(impl->stream, buffer);
    return;
  }

  struct pw_chunk* chunk = data->chunk;
  if (!chunk || !chunk->data) {
    pw_stream_queue_buffer(impl->stream, buffer);
    return;
  }

  int16_t* audio_data = static_cast<int16_t*>(data->mem->data);
  size_t sample_count = data->chunk->size / sizeof(int16_t);

  // Puffer-Callback aufrufen
  if (playback->buffer_callback_) {
    std::span<int16_t> span(audio_data, sample_count);
    playback->buffer_callback_(span);
  }

  pw_stream_queue_buffer(impl->stream, buffer);
}

void PipeWirePlayback::on_state_changed(void* user, enum pw_stream_state old,
                                         enum pw_stream_state state,
                                         const char* error) {
  auto* playback = static_cast<PipeWirePlayback*>(user);
  (void)old;

  switch (state) {
    case PW_STREAM_STATE_STREAMING:
      playback->running_ = true;
      break;
    case PW_STREAM_STATE_PAUSED:
    case PW_STREAM_STATE_DISCONNECTED:
      playback->running_ = false;
      break;
    default:
      break;
  }

  (void)error;
}

} // namespace hh::audio
