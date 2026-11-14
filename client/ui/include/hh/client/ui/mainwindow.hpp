// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemModel>
#include <QAction>
#include <QAbstractItemView>
#include <QKeyEvent>
#include <QMainWindow>
#include <QModelIndex>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSplitter>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QSettings>
#include <QCloseEvent>
#include <QInputDialog>
#include <QMessageBox>
#include <QKeySequence>
#include <QShortcut>
#include <QCheckBox>
#include <QSlider>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QProgressBar>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>

#include "hh/client/session.hpp"
#include "hh/client/voice_pipeline.hpp"

namespace hh::client::ui {

// ── Kanalbaum-Modell ──────────────────────────────────────────────────

struct ChannelNode {
  uint32_t id = 0;
  uint32_t parent_id = 0;
  std::string name;
  uint32_t max_users = 0;
  bool temporary = false;
  std::string description;
  std::vector<ChannelNode*> children;
  std::vector<uint32_t> user_ids;

  // Positional Audio
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  bool is_speaking = false;
  bool is_muted = false;
  bool is_deafened = false;
};

class ChannelTreeModel : public QAbstractItemModel {
  Q_OBJECT

public:
  explicit ChannelTreeModel(QObject* parent = nullptr);
  ~ChannelTreeModel() override = default;

  void add_channel(uint32_t id, uint32_t parent_id, const std::string& name,
                   uint32_t max_users, bool temporary, const std::string& description);
  void remove_channel(uint32_t id);
  void add_user_to_channel(uint32_t channel_id, uint32_t user_id);
  void remove_user_from_channel(uint32_t channel_id, uint32_t user_id);
  void set_speaking(uint32_t user_id, bool speaking);
  void set_user_position(uint32_t user_id, float x, float y, float z);

  // QAbstractItemModel-Interface
  QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
  QModelIndex parent(const QModelIndex& child) const override;
  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
  ChannelNode* node_from_index(const QModelIndex& index) const;
  QModelIndex index_from_node(ChannelNode* node) const;

  ChannelNode root_;
  std::map<uint32_t, ChannelNode*> id_to_node_;
  std::map<ChannelNode*, QModelIndex> node_to_index_;
};

// ── Hauptfenster ──────────────────────────────────────────────────────

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override = default;

  // Session-Callbacks registrieren
  void set_session(std::shared_ptr<hh::client::Session> session);

  // Verbindungs-Assistent
  void show_connect_dialog();

  // PTT (Push-to-Talk)
  void set_ptt_key(QKeySequence key);
  void set_ptt_enabled(bool enabled);
  bool is_ptt_active() const { return ptt_active_; }

  // VAD (Voice-Activation-Detection)
  void set_vad_enabled(bool enabled);
  void set_vad_threshold(int threshold);

  // Status
  void set_connection_status(const std::string& status);
  void set_username(const std::string& username);
  void update_channel_list(const std::vector<std::pair<uint32_t, std::string>>& channels);
  void update_user_list(uint32_t channel_id, const std::vector<std::pair<uint32_t, std::string>>& users);
  void set_user_speaking(uint32_t user_id, bool speaking);
  void set_user_muted(uint32_t user_id, bool muted);
  void set_user_deafened(uint32_t user_id, bool deafened);

  // Statistik
  void update_stats(double rtt_ms, double jitter_ms, double loss_percent,
                    int bitrate_kbit, int latency_ms);

  // Latenz-Monitor
  void add_latency_sample(double latency_ms);
  double avg_latency() const { return avg_latency_; }
  double min_latency() const { return min_latency_; }
  double max_latency() const { return max_latency_; }

protected:
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private slots:
  void on_connect();
  void on_disconnect();
  void on_ptt_pressed();
  void on_ptt_released();
  void on_channel_double_clicked(const QModelIndex& index);
  void on_text_message_sent();
  void on_settings_clicked();
  void on_tray_activated(QSystemTrayIcon::ActivationReason reason);
  void on_statistics_timer();
  void on_voice_status(double volume, bool speaking);

private:
  void setup_ui();
  void setup_menu_bar();
  void setup_toolbar();
  void setup_central_widget();
  void setup_status_bar();
  void setup_tray_icon();
  void apply_amber_theme();
  void load_settings();
  void save_settings();

  // UI-Komponenten
  QSplitter* splitter_ = nullptr;
  QTreeWidget* channel_tree_ = nullptr;
  ChannelTreeModel* channel_model_ = nullptr;
  QPlainTextEdit* chat_area_ = nullptr;
  QLineEdit* message_input_ = nullptr;
  QPushButton* ptt_button_ = nullptr;
  QStatusBar* status_bar_ = nullptr;
  QSystemTrayIcon* tray_icon_ = nullptr;
  QTimer* stats_timer_ = nullptr;
  QLabel* latency_label_ = nullptr;

  // Session
  std::shared_ptr<hh::client::Session> session_;

  // Voice-Pipeline
  std::shared_ptr<hh::client::VoicePipeline> voice_pipeline_;

  // Latenz-Monitor
  std::vector<double> latency_samples_;
  double avg_latency_ = 0.0;
  double min_latency_ = 1000.0;
  double max_latency_ = 0.0;

  // PTT/VAD
  bool ptt_active_ = false;
  bool ptt_enabled_ = false;
  QKeySequence ptt_key_ = QKeySequence(Qt::Key_MediaPlay);
  bool vad_enabled_ = false;
  int vad_threshold_ = 30;

  // Aktuelle Daten
  std::string username_;
  std::string connection_status_;
  double stats_rtt_ms_ = 0;
  double stats_jitter_ms_ = 0;
  double stats_loss_percent_ = 0;
  int stats_bitrate_kbit_ = 0;
  int stats_latency_ms_ = 0;
};

} // namespace hh::client::ui
