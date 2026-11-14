// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QProgressBar>
#include <QTimer>
#include <QTableWidget>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QDateTime>
#include <cmath>

namespace hh::client::ui {

// Statistikfenster für Netzwerk- und Audio-Statistiken
class StatisticsWidget : public QDialog {
  Q_OBJECT

public:
  explicit StatisticsWidget(QWidget* parent = nullptr);
  ~StatisticsWidget() override = default;

  // Netzwerkstatistik aktualisieren
  void update_network(double rtt_ms, double jitter_ms, double loss_percent,
                      int bitrate_kbit, int latency_ms);

  // Audio-Statistik aktualisieren
  void update_audio(int sample_rate, int channels, int frame_size_ms,
                    double volume_percent, bool muted);

  // Jitter-Buffer-Statistik
  void update_jitter_buffer(int delay_ms, int packet_loss_count,
                            int total_packets, double jitter_ms);

  // VAD-Statistik
  void update_vad(bool is_speaking, double energy, double zcr,
                  double speech_probability);

  // RNNoise-Statistik
  void update_noise_suppression(int frames_processed, float suppression_strength);

  // PLC-Statistik
  void update_plc(int total_losses, int total_concealed_frames);

  // Protokoll-Log hinzufügen
  void add_log_entry(const std::string& message);

  // Fenster aktualisieren (von Timer aufgerufen)
  void refresh();

protected:
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

private slots:
  void on_timer_update();

private:
  void setup_ui();
  void update_network_display();
  void update_audio_display();
  void update_jitter_buffer_display();
  void update_vad_display();
  void update_noise_suppression_display();
  void update_plc_display();

  // Timer
  QTimer* timer_ = nullptr;

  // Netzwerk-Widgets
  QLabel* rtt_label_ = nullptr;
  QLabel* jitter_label_ = nullptr;
  QLabel* loss_label_ = nullptr;
  QLabel* bitrate_label_ = nullptr;
  QLabel* latency_label_ = nullptr;
  QProgressBar* rtt_bar_ = nullptr;
  QProgressBar* jitter_bar_ = nullptr;
  QProgressBar* loss_bar_ = nullptr;

  // Audio-Widgets
  QLabel* sample_rate_label_ = nullptr;
  QLabel* channels_label_ = nullptr;
  QLabel* frame_size_label_ = nullptr;
  QLabel* volume_label_ = nullptr;
  QLabel* muted_label_ = nullptr;
  QProgressBar* volume_bar_ = nullptr;

  // Jitter-Buffer-Widgets
  QLabel* jb_delay_label_ = nullptr;
  QLabel* jb_loss_label_ = nullptr;
  QLabel* jb_total_label_ = nullptr;
  QLabel* jb_jitter_label_ = nullptr;
  QProgressBar* jb_delay_bar_ = nullptr;

  // VAD-Widgets
  QLabel* vad_state_label_ = nullptr;
  QLabel* vad_energy_label_ = nullptr;
  QLabel* vad_zcr_label_ = nullptr;
  QLabel* vad_prob_label_ = nullptr;
  QProgressBar* vad_energy_bar_ = nullptr;
  QProgressBar* vad_zcr_bar_ = nullptr;
  QProgressBar* vad_prob_bar_ = nullptr;

  // RNNoise-Widgets
  QLabel* ns_frames_label_ = nullptr;
  QLabel* ns_strength_label_ = nullptr;
  QProgressBar* ns_strength_bar_ = nullptr;

  // PLC-Widgets
  QLabel* plc_losses_label_ = nullptr;
  QLabel* plc_concealed_label_ = nullptr;

  // Protokoll
  QPlainTextEdit* log_area_ = nullptr;
};

} // namespace hh::client::ui
