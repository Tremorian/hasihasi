// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "voice_assistant.hpp"

#include <QOperatingSystemVersion>
#include <QSysInfo>
#include <QSettings>
#include <QRegularExpression>

namespace hh::client {

VoiceAssistant::VoiceAssistant(QObject* parent)
  : QObject(parent) {
  
  // Standard-Commands registrieren
  register_command({
    "stumm",
    "stumm schalten|mute|stumm",
    []() {
      // Stumm umschalten
      return true;
    },
    "Mikrofon stumm schalten"
  });
  
  register_command({
    "kanal wechseln",
    "kanal wechseln|switch channel|change channel",
    []() {
      // Kanal wechseln
      return true;
    },
    "Zum nächsten Kanal wechseln"
  });
  
  register_command({
    "lautstärke",
    "lautstärke (erhöhen|verringern|hoch|runter|up|down)",
    []() {
      // Lautstärke ändern
      return true;
    },
    "Lautstärke ändern"
  });
  
  register_command({
    "verbindung trennen",
    "verbindung trennen|disconnect|disconnect me",
    []() {
      // Verbindung trennen
      return true;
    },
    "Verbindung zum Server trennen"
  });
}

VoiceAssistant::VoiceAssistant(VoiceAssistant&& other) noexcept
  : QObject(other.parent())
  , stt_engine_(other.stt_engine_)
  , tts_engine_(other.tts_engine_)
  , language_(other.language_)
  , wake_word_state_(other.wake_word_state_)
  , wake_word_(std::move(other.wake_word_))
  , wake_word_enabled_(other.wake_word_enabled_)
  , commands_(std::move(other.commands_))
  , last_recognition_(other.last_recognition_)
  , audio_level_(other.audio_level_)
  , audio_input_device_(std::move(other.audio_input_device_))
  , stats_(other.stats_)
  , recognition_callback_(std::move(other.recognition_callback_))
  , command_callback_(std::move(other.command_callback_))
  , wake_word_callback_(std::move(other.wake_word_callback_)) {
  
  other.stt_engine_ = STTEngine::NONE;
  other.tts_engine_ = TTSEngine::NONE;
  other.language_ = Language::GERMAN;
  other.wake_word_state_ = WakeWordState::DISABLED;
  other.wake_word_enabled_ = false;
  other.audio_level_ = {0.0f, false, 0.0f};
  other.stats_ = Stats{};
}

VoiceAssistant& VoiceAssistant::operator=(VoiceAssistant&& other) noexcept {
  if (this != &other) {
    QObject::operator=(std::move(other));
    stt_engine_ = other.stt_engine_;
    tts_engine_ = other.tts_engine_;
    language_ = other.language_;
    wake_word_state_ = other.wake_word_state_;
    wake_word_ = std::move(other.wake_word_);
    wake_word_enabled_ = other.wake_word_enabled_;
    commands_ = std::move(other.commands_);
    last_recognition_ = other.last_recognition_;
    audio_level_ = other.audio_level_;
    audio_input_device_ = std::move(other.audio_input_device_);
    stats_ = other.stats_;
    recognition_callback_ = std::move(other.recognition_callback_);
    command_callback_ = std::move(other.command_callback_);
    wake_word_callback_ = std::move(other.wake_word_callback_);
    other.stt_engine_ = STTEngine::NONE;
    other.tts_engine_ = TTSEngine::NONE;
    other.language_ = Language::GERMAN;
    other.wake_word_state_ = WakeWordState::DISABLED;
    other.wake_word_enabled_ = false;
    other.audio_level_ = {0.0f, false, 0.0f};
    other.stats_ = Stats{};
  }
  return *this;
}

// ── STT-Engine ───────────────────────────────────────────────────────

VoiceAssistant::STTEngine VoiceAssistant::stt_engine() const {
  return stt_engine_;
}

bool VoiceAssistant::set_stt_engine(STTEngine engine) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (stt_engine_ != engine) {
    stt_engine_ = engine;
    emit stt_engine_changed(engine);
    return true;
  }
  return false;
}

// ── TTS-Engine ───────────────────────────────────────────────────────

VoiceAssistant::TTSEngine VoiceAssistant::tts_engine() const {
  return tts_engine_;
}

bool VoiceAssistant::set_tts_engine(TTSEngine engine) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (tts_engine_ != engine) {
    tts_engine_ = engine;
    emit tts_engine_changed(engine);
    return true;
  }
  return false;
}

// ── Sprache ──────────────────────────────────────────────────────────

VoiceAssistant::Language VoiceAssistant::language() const {
  return language_;
}

bool VoiceAssistant::set_language(Language lang) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (language_ != lang) {
    language_ = lang;
    emit language_changed(lang);
    return true;
  }
  return false;
}

std::vector<VoiceAssistant::Language> VoiceAssistant::supported_languages() const {
  return {
    Language::GERMAN,
    Language::ENGLISH,
    Language::FRENCH,
    Language::SPANISH
  };
}

// ── Wake Word ────────────────────────────────────────────────────────

VoiceAssistant::WakeWordState VoiceAssistant::wake_word_state() const {
  return wake_word_state_;
}

void VoiceAssistant::set_wake_word(const QString& word) {
  std::lock_guard<std::mutex> lock(mutex_);
  wake_word_ = word;
}

bool VoiceAssistant::is_wake_word_enabled() const {
  return wake_word_enabled_;
}

// ── Voice Commands ───────────────────────────────────────────────────

bool VoiceAssistant::register_command(const Command& command) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  commands_[command.name] = command;
  return true;
}

bool VoiceAssistant::unregister_command(const QString& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = commands_.find(name);
  if (it != commands_.end()) {
    commands_.erase(it);
    return true;
  }
  return false;
}

std::vector<QString> VoiceAssistant::available_commands() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<QString> result;
  for (const auto& pair : commands_) {
    result.push_back(pair.first);
  }
  return result;
}

VoiceAssistant::CommandResult VoiceAssistant::execute_command(const QString& text) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  // Command matching
  for (const auto& pair : commands_) {
    QRegularExpression regex(pair.second.pattern);
    if (regex.match(text).hasMatch()) {
      // Command ausführen
      bool success = pair.second.action();
      
      stats_.commands_executed++;
      emit command_executed(pair.first, success);
      
      if (command_callback_) {
        command_callback_(pair.first, success);
      }
      
      return success ? CommandResult::SUCCESS : CommandResult::FAILED;
    }
  }
  
  stats_.commands_failed++;
  return CommandResult::NOT_FOUND;
}

// ── Audio-Eingabe ────────────────────────────────────────────────────

VoiceAssistant::AudioLevel VoiceAssistant::get_audio_level() const {
  return audio_level_;
}

void VoiceAssistant::set_audio_input(const QString& device_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  audio_input_device_ = device_id;
}

// ── Erkennungsergebnis ───────────────────────────────────────────────

VoiceAssistant::RecognitionResult VoiceAssistant::last_recognition() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_recognition_;
}

// ── Statistik ────────────────────────────────────────────────────────

VoiceAssistant::Stats VoiceAssistant::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return stats_;
}

void VoiceAssistant::reset_stats() {
  std::lock_guard<std::mutex> lock(mutex_);
  stats_ = Stats{};
}

// ── Callbacks ────────────────────────────────────────────────────────

void VoiceAssistant::set_recognition_callback(RecognitionCallback callback) {
  recognition_callback_ = std::move(callback);
}

void VoiceAssistant::set_command_callback(CommandCallback callback) {
  command_callback_ = std::move(callback);
}

void VoiceAssistant::set_wake_word_callback(WakeWordCallback callback) {
  wake_word_callback_ = std::move(callback);
}

// ── Slots ────────────────────────────────────────────────────────────

void VoiceAssistant::on_audio_input(const QByteArray& audio_data) {
  // Audio-Eingabe verarbeiten
  // In einer echten Implementierung würde hier die STT-Engine angesprochen
  
  // Simuliere Erkennung
  audio_level_.volume = 0.5f;
  audio_level_.is_speech = true;
  audio_level_.confidence = 0.8f;
}

void VoiceAssistant::on_wake_word_detected() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  wake_word_state_ = WakeWordState::DETECTED;
  stats_.wake_word_detections++;
  
  emit wake_word_detected(wake_word_);
  
  if (wake_word_callback_) {
    wake_word_callback_(wake_word_);
  }
}

void VoiceAssistant::on_recognition_complete(const QString& text, float confidence) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  last_recognition_.text = text;
  last_recognition_.confidence = confidence;
  last_recognition_.detected_language = language_;
  last_recognition_.is_command = true;
  
  // Command matching
  for (const auto& pair : commands_) {
    QRegularExpression regex(pair.second.pattern);
    if (regex.match(text).hasMatch()) {
      last_recognition_.matched_command = pair.first;
      break;
    }
  }
  
  stats_.recognitions++;
  
  emit recognition_completed(last_recognition_);
  
  if (recognition_callback_) {
    recognition_callback_(last_recognition_);
  }
}

} // namespace hh::client
