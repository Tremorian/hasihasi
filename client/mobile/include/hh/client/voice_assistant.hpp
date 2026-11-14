// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include <map>
#include <functional>
#include <mutex>

namespace hh::client {

// Sprachassistent für Voice Commands mit lokaler STT
class VoiceAssistant : public QObject {
  Q_OBJECT

public:
  explicit VoiceAssistant(QObject* parent = nullptr);
  ~VoiceAssistant() override = default;

  // Nicht kopierbar, bewegbar
  VoiceAssistant(const VoiceAssistant&) = delete;
  VoiceAssistant& operator=(const VoiceAssistant&) = delete;
  VoiceAssistant(VoiceAssistant&&) noexcept;
  VoiceAssistant& operator=(VoiceAssistant&&) noexcept;

  // STT-Engine
  enum class STTEngine {
    VOSK,
    WHISPER_CPP,
    NONE
  };
  Q_ENUM(STTEngine)

  STTEngine stt_engine() const;
  bool set_stt_engine(STTEngine engine);

  // TTS-Engine
  enum class TTSEngine {
    NONE,
    QT_TTS,
    PICO_TTS
  };
  Q_ENUM(TTSEngine)

  TTSEngine tts_engine() const;
  bool set_tts_engine(TTSEngine engine);

  // Sprache
  enum class Language {
    GERMAN,
    ENGLISH,
    FRENCH,
    SPANISH
  };
  Q_ENUM(Language)

  Language language() const;
  bool set_language(Language lang);
  std::vector<Language> supported_languages() const;

  // Wake Word
  enum class WakeWordState {
    DISABLED,
    LISTENING,
    DETECTED,
    PROCESSING
  };
  Q_ENUM(WakeWordState)

  WakeWordState wake_word_state() const;
  void set_wake_word(const QString& word);
  bool is_wake_word_enabled() const;

  // Voice Commands
  struct Command {
    QString name;
    QString pattern; // Regex-Muster für die Erkennung
    std::function<bool()> action; // Auszuführende Aktion
    QString description;
  };

  bool register_command(const Command& command);
  bool unregister_command(const QString& name);
  std::vector<QString> available_commands() const;

  // Command-Ausführung
  enum class CommandResult {
    SUCCESS,
    FAILED,
    NOT_FOUND,
    ERROR
  };
  Q_ENUM(CommandResult)

  CommandResult execute_command(const QString& text);

  // Audio-Eingabe
  struct AudioLevel {
    float volume; // 0.0-1.0
    bool is_speech;
    float confidence;
  };

  AudioLevel get_audio_level() const;
  void set_audio_input(const QString& device_id);

  //asst
  struct RecognitionResult {
    QString text;
    float confidence;
    Language detected_language;
    bool is_command;
    QString matched_command;
  };

  RecognitionResult last_recognition() const;

  // Statistik
  struct Stats {
    int commands_executed;
    int commands_failed;
    int wake_word_detections;
    int recognitions;
    qint64 processing_time_ms;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using RecognitionCallback = std::function<void(const RecognitionResult& result)>;
  using CommandCallback = std::function<void(const QString& command, bool success)>;
  using WakeWordCallback = std::function<void(const QString& word)>;

  void set_recognition_callback(RecognitionCallback callback);
  void set_command_callback(CommandCallback callback);
  void set_wake_word_callback(WakeWordCallback callback);

signals:
  void recognition_started();
  void recognition_completed(const RecognitionResult& result);
  void command_executed(const QString& command, bool success);
  void wake_word_detected(const QString& word);
  void error_occurred(const QString& error);
  void language_changed(Language lang);
  void stt_engine_changed(STTEngine engine);
  void tts_engine_changed(TTSEngine engine);

private slots:
  void on_audio_input(const QByteArray& audio_data);
  void on_wake_word_detected();
  void on_recognition_complete(const QString& text, float confidence);

private:
  STTEngine stt_engine_ = STTEngine::NONE;
  TTSEngine tts_engine_ = TTSEngine::NONE;
  Language language_ = Language::GERMAN;
  WakeWordState wake_word_state_ = WakeWordState::DISABLED;
  QString wake_word_ = "Hey Hasi";
  bool wake_word_enabled_ = false;
  
  std::map<QString, Command> commands_;
  RecognitionResult last_recognition_;
  
  AudioLevel audio_level_ = {0.0f, false, 0.0f};
  QString audio_input_device_;
  
  Stats stats_;
  
  RecognitionCallback recognition_callback_;
  CommandCallback command_callback_;
  WakeWordCallback wake_word_callback_;
  
  mutable std::mutex mutex_;
};

} // namespace hh::client
