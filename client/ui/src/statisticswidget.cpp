// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/ui/statisticswidget.hpp"

#include <QDateTime>

namespace hh::client::ui {

StatisticsWidget::StatisticsWidget(QWidget* parent)
  : QDialog(parent) {
  setWindowTitle("Statistik");
  setMinimumSize(500, 600);

  setup_ui();

  // Timer für regelmäßige Updates
  timer_ = new QTimer(this);
  connect(timer_, &QTimer::timeout, this, &StatisticsWidget::on_timer_update);
  timer_->start(1000); // Alle Sekunde
}

StatisticsWidget::~StatisticsWidget() = default;

void StatisticsWidget::setup_ui() {
  auto* main_layout = new QVBoxLayout(this);

  // ── Netzwerkstatistik ─────────────────────────────────────────────
  auto* network_group = new QGroupBox("Netzwerk", this);
  auto* network_layout = new QFormLayout(network_group);

  rtt_label_ = new QLabel("0.0 ms", this);
  jitter_label_ = new QLabel("0.0 ms", this);
  loss_label_ = new QLabel("0.0%", this);
  bitrate_label_ = new QLabel("0 kbit/s", this);
  latency_label_ = new QLabel("0 ms", this);

  rtt_bar_ = new QProgressBar(this);
  rtt_bar_->setRange(0, 100);
  jitter_bar_ = new QProgressBar(this);
  jitter_bar_->setRange(0, 100);
  loss_bar_ = new QProgressBar(this);
  loss_bar_->setRange(0, 100);

  network_layout->addRow("RTT:", rtt_label_);
  network_layout->addRow("Jitter:", jitter_label_);
  network_layout->addRow("Verlust:", loss_label_);
  network_layout->addRow("Bitrate:", bitrate_label_);
  network_layout->addRow("Latenz:", latency_label_);
  network_layout->addRow("RTT:", rtt_bar_);
  network_layout->addRow("Jitter:", jitter_bar_);
  network_layout->addRow("Verlust:", loss_bar_);

  main_layout->addWidget(network_group);

  // ── Audio-Statistik ───────────────────────────────────────────────
  auto* audio_group = new QGroupBox("Audio", this);
  auto* audio_layout = new QFormLayout(audio_group);

  sample_rate_label_ = new QLabel("48000 Hz", this);
  channels_label_ = new QLabel("1 (Mono)", this);
  frame_size_label_ = new QLabel("10 ms", this);
  volume_label_ = new QLabel("0%", this);
  muted_label_ = new QLabel("Nein", this);

  volume_bar_ = new QProgressBar(this);
  volume_bar_->setRange(0, 100);

  audio_layout->addRow("Abtastrate:", sample_rate_label_);
  audio_layout->addRow("Kanäle:", channels_label_);
  audio_layout->addRow("Frame-Größe:", frame_size_label_);
  audio_layout->addRow("Lautstärke:", volume_label_);
  audio_layout->addRow("Stumm:", muted_label_);
  audio_layout->addRow("Lautstärke:", volume_bar_);

  main_layout->addWidget(audio_group);

  // ── Jitter-Buffer-Statistik ───────────────────────────────────────
  auto* jb_group = new QGroupBox("Jitter-Buffer", this);
  auto* jb_layout = new QFormLayout(jb_group);

  jb_delay_label_ = new QLabel("40 ms", this);
  jb_loss_label_ = new QLabel("0", this);
  jb_total_label_ = new QLabel("0", this);
  jb_jitter_label_ = new QLabel("0.0 ms", this);

  jb_delay_bar_ = new QProgressBar(this);
  jb_delay_bar_->setRange(0, 200);

  jb_layout->addRow("Verzögerung:", jb_delay_label_);
  jb_layout->addRow("Paketverluste:", jb_loss_label_);
  jb_layout->addRow("Gesamtpakete:", jb_total_label_);
  jb_layout->addRow("Jitter:", jb_jitter_label_);
  jb_layout->addRow("Verzögerung:", jb_delay_bar_);

  main_layout->addWidget(jb_group);

  // ── VAD-Statistik ─────────────────────────────────────────────────
  auto* vad_group = new QGroupBox("Voice-Activation-Detection", this);
  auto* vad_layout = new QFormLayout(vad_group);

  vad_state_label_ = new QLabel("Stille", this);
  vad_energy_label_ = new QLabel("0.0", this);
  vad_zcr_label_ = new QLabel("0.0", this);
  vad_prob_label_ = new QLabel("0.00", this);

  vad_energy_bar_ = new QProgressBar(this);
  vad_energy_bar_->setRange(0, 1000);
  vad_zcr_bar_ = new QProgressBar(this);
  vad_zcr_bar_->setRange(0, 100);
  vad_prob_bar_ = new QProgressBar(this);
  vad_prob_bar_->setRange(0, 100);

  vad_layout->addRow("Status:", vad_state_label_);
  vad_layout->addRow("Energie:", vad_energy_label_);
  vad_layout->addRow("ZCR:", vad_zcr_label_);
  vad_layout->addRow("Wahrscheinlichkeit:", vad_prob_label_);
  vad_layout->addRow("Energie:", vad_energy_bar_);
  vad_layout->addRow("ZCR:", vad_zcr_bar_);
  vad_layout->addRow("Wahrscheinlichkeit:", vad_prob_bar_);

  main_layout->addWidget(vad_group);

  // ── Rauschunterdrückung ───────────────────────────────────────────
  auto* ns_group = new QGroupBox("Rauschunterdrückung (RNNoise)", this);
  auto* ns_layout = new QFormLayout(ns_group);

  ns_frames_label_ = new QLabel("0", this);
  ns_strength_label_ = new QLabel("0.80", this);

  ns_strength_bar_ = new QProgressBar(this);
  ns_strength_bar_->setRange(0, 100);

  ns_layout->addRow("Verarbeitete Frames:", ns_frames_label_);
  ns_layout->addRow("Stärke:", ns_strength_label_);
  ns_layout->addRow("Stärke:", ns_strength_bar_);

  main_layout->addWidget(ns_group);

  // ── PLC-Statistik ─────────────────────────────────────────────────
  auto* plc_group = new QGroupBox("Packet Loss Concealment", this);
  auto* plc_layout = new QFormLayout(plc_group);

  plc_losses_label_ = new QLabel("0", this);
  plc_concealed_label_ = new QLabel("0", this);

  plc_layout->addRow("Gesamte Verluste:", plc_losses_label_);
  plc_layout->addRow("Verborgene Frames:", plc_concealed_label_);

  main_layout->addWidget(plc_group);

  // ── Protokoll ─────────────────────────────────────────────────────
  log_area_ = new QPlainTextEdit(this);
  log_area_->setReadOnly(true);
  log_area_->setMaximumHeight(150);
  log_area_->setPlaceholderText("Ereignis-Protokoll...");

  main_layout->addWidget(log_area_);
}

void StatisticsWidget::update_network(double rtt_ms, double jitter_ms, double loss_percent,
                                       int bitrate_kbit, int latency_ms) {
  (void)rtt_ms; (void)jitter_ms; (void)loss_percent;
  (void)bitrate_kbit; (void)latency_ms;
  // Wird von on_timer_update() aktualisiert
}

void StatisticsWidget::update_audio(int sample_rate, int channels, int frame_size_ms,
                                     double volume_percent, bool muted) {
  (void)sample_rate; (void)channels; (void)frame_size_ms;
  (void)volume_percent; (void)muted;
  // Wird von on_timer_update() aktualisiert
}

void StatisticsWidget::update_jitter_buffer(int delay_ms, int packet_loss_count,
                                             int total_packets, double jitter_ms) {
  (void)delay_ms; (void)packet_loss_count;
  (void)total_packets; (void)jitter_ms;
  // Wird von on_timer_update() aktualisiert
}

void StatisticsWidget::update_vad(bool is_speaking, double energy, double zcr,
                                   double speech_probability) {
  (void)is_speaking; (void)energy; (void)zcr;
  (void)speech_probability;
  // Wird von on_timer_update() aktualisiert
}

void StatisticsWidget::update_noise_suppression(int frames_processed, float suppression_strength) {
  (void)frames_processed; (void)suppression_strength;
  // Wird von on_timer_update() aktualisiert
}

void StatisticsWidget::update_plc(int total_losses, int total_concealed_frames) {
  (void)total_losses; (void)total_concealed_frames;
  // Wird von on_timer_update() aktualisiert
}

void StatisticsWidget::add_log_entry(const std::string& message) {
  QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  log_area_->appendPlainText(QString("[%1] %2").arg(timestamp, QString::fromStdString(message)));

  // Begrenze Log-Einträge
  auto* document = log_area_->document();
  if (document->blockCount() > 1000) {
    auto* first_block = document->firstBlock();
    auto* cursor = log_area_->textCursor();
    cursor.movePosition(QTextCursor::Start);
    cursor.setPosition(first_block.position() + first_block.length());
    cursor.removeSelectedText();
    cursor.deleteChar();
  }
}

void StatisticsWidget::refresh() {
  update_network_display();
  update_audio_display();
  update_jitter_buffer_display();
  update_vad_display();
  update_noise_suppression_display();
  update_plc_display();
}

void StatisticsWidget::showEvent(QShowEvent* event) {
  QDialog::showEvent(event);
  timer_->start(1000);
  add_log_entry("Statistikfenster geöffnet");
}

void StatisticsWidget::hideEvent(QHideEvent* event) {
  QDialog::hideEvent(event);
  timer_->stop();
  add_log_entry("Statistikfenster geschlossen");
}

void StatisticsWidget::on_timer_update() {
  refresh();
}

// ── Display-Updates ───────────────────────────────────────────────────

void StatisticsWidget::update_network_display() {
  // Platzhalter – wird von der echten Implementierung gefüllt
  rtt_label_->setText("0.0 ms");
  jitter_label_->setText("0.0 ms");
  loss_label_->setText("0.0%");
  bitrate_label_->setText("0 kbit/s");
  latency_label_->setText("0 ms");

  rtt_bar_->setValue(0);
  jitter_bar_->setValue(0);
  loss_bar_->setValue(0);
}

void StatisticsWidget::update_audio_display() {
  sample_rate_label_->setText("48000 Hz");
  channels_label_->setText("1 (Mono)");
  frame_size_label_->setText("10 ms");
  volume_label_->setText("0%");
  muted_label_->setText("Nein");

  volume_bar_->setValue(0);
}

void StatisticsWidget::update_jitter_buffer_display() {
  jb_delay_label_->setText("40 ms");
  jb_loss_label_->setText("0");
  jb_total_label_->setText("0");
  jb_jitter_label_->setText("0.0 ms");

  jb_delay_bar_->setValue(40);
}

void StatisticsWidget::update_vad_display() {
  vad_state_label_->setText("Stille");
  vad_energy_label_->setText("0.0");
  vad_zcr_label_->setText("0.0");
  vad_prob_label_->setText("0.00");

  vad_energy_bar_->setValue(0);
  vad_zcr_bar_->setValue(0);
  vad_prob_bar_->setValue(0);
}

void StatisticsWidget::update_noise_suppression_display() {
  ns_frames_label_->setText("0");
  ns_strength_label_->setText("0.80");

  ns_strength_bar_->setValue(80);
}

void StatisticsWidget::update_plc_display() {
  plc_losses_label_->setText("0");
  plc_concealed_label_->setText("0");
}

} // namespace hh::client::ui
