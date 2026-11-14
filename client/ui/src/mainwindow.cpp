// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/ui/mainwindow.hpp"

#include "hh/logging.hpp"
#include "hh/client/i18n_manager.hpp"
#include "hh/client/ui/languagedialog.hpp"

#include <QDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QSlider>
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
#include <QTranslator>

namespace hh::client::ui {

// ── ChannelTreeModel ──────────────────────────────────────────────────

ChannelTreeModel::ChannelTreeModel(QObject* parent)
  : QAbstractItemModel(parent) {
  id_to_node_[0] = &root_;
}

ChannelNode* ChannelTreeModel::node_from_index(const QModelIndex& index) const {
  if (!index.isValid()) return &root_;
  auto* node = static_cast<ChannelNode*>(index.internalPointer());
  return node ? node : &root_;
}

QModelIndex ChannelTreeModel::index_from_node(ChannelNode* node) const {
  if (!node || node == &root_) return QModelIndex();
  auto it = node_to_index_.find(node);
  if (it != node_to_index_.end()) return it->second;
  return QModelIndex();
}

void ChannelTreeModel::add_channel(uint32_t id, uint32_t parent_id, const std::string& name,
                                    uint32_t max_users, bool temporary,
                                    const std::string& description) {
  auto* parent = id_to_node_.count(parent_id) ? id_to_node_[parent_id] : &root_;

  auto* node = new ChannelNode{
    id, parent_id, name, max_users, temporary, description, {}, {}, {}
  };
  parent->children.push_back(node);
  id_to_node_[id] = node;

  beginResetModel();
  endResetModel();
}

void ChannelTreeModel::remove_channel(uint32_t id) {
  auto it = id_to_node_.find(id);
  if (it == id_to_node_.end()) return;

  auto* node = it->second;
  auto* parent = id_to_node_.count(node->parent_id) ? id_to_node_[node->parent_id] : &root_;

  parent->children.erase(
    std::remove(parent->children.begin(), parent->children.end(), node),
    parent->children.end());

  delete node;
  id_to_node_.erase(it);

  beginResetModel();
  endResetModel();
}

void ChannelTreeModel::add_user_to_channel(uint32_t channel_id, uint32_t user_id) {
  auto* node = id_to_node_.count(channel_id) ? id_to_node_[channel_id] : nullptr;
  if (!node) return;

  node->user_ids.push_back(user_id);
  emit dataChanged(index_from_node(node), index_from_node(node), {Qt::DisplayRole});
}

void ChannelTreeModel::remove_user_from_channel(uint32_t channel_id, uint32_t user_id) {
  auto* node = id_to_node_.count(channel_id) ? id_to_node_[channel_id] : nullptr;
  if (!node) return;

  node->user_ids.erase(
    std::remove(node->user_ids.begin(), node->user_ids.end(), user_id),
    node->user_ids.end());
  emit dataChanged(index_from_node(node), index_from_node(node), {Qt::DisplayRole});
}

void ChannelTreeModel::set_speaking(uint32_t user_id, bool speaking) {
  (void)user_id;
  (void)speaking;
  // TODO: Sprechindikator aktualisieren
}

QModelIndex ChannelTreeModel::index(int row, int column, const QModelIndex& parent) const {
  auto* parent_node = node_from_index(parent);
  if (row < 0 || row >= static_cast<int>(parent_node->children.size())) {
    return {};
  }
  auto* child_node = parent_node->children[row];
  return createIndex(row, column, child_node);
}

QModelIndex ChannelTreeModel::parent(const QModelIndex& child) const {
  auto* child_node = node_from_index(child);
  if (!child_node || child_node == &root_) return {};
  auto* parent_node = id_to_node_.count(child_node->parent_id) ?
                       id_to_node_[child_node->parent_id] : &root_;
  if (parent_node == &root_) return {};

  auto* grandparent = id_to_node_.count(parent_node->parent_id) ?
                       id_to_node_[parent_node->parent_id] : &root_;
  int row = 0;
  for (size_t i = 0; i < grandparent->children.size(); ++i) {
    if (grandparent->children[i] == parent_node) {
      row = static_cast<int>(i);
      break;
    }
  }
  return createIndex(row, 0, parent_node);
}

int ChannelTreeModel::rowCount(const QModelIndex& parent) const {
  auto* node = node_from_index(parent);
  return static_cast<int>(node->children.size());
}

int ChannelTreeModel::columnCount(const QModelIndex& parent) const {
  (void)parent;
  return 1;
}

QVariant ChannelTreeModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};
  auto* node = node_from_index(index);

  if (role == Qt::DisplayRole) {
    QString text = QString::fromStdString(node->name);
    if (!node->user_ids.empty()) {
      text += QString(" (%1)") .arg(static_cast<int>(node->user_ids.size()));
    }
    return text;
  }

  if (role == Qt::ForegroundRole) {
    // Sprechende Nutzer hervorgehoben
    if (!node->user_ids.empty()) {
      return QColor("#FFCC33"); // Akzentfarbe
    }
    return QColor("#FFB000"); // Primärfarbe
  }

  return {};
}

bool ChannelTreeModel::setData(const QModelIndex& index, const QVariant& value, int role) {
  if (!index.isValid() || role != Qt::EditRole) return false;
  auto* node = node_from_index(index);
  node->name = value.toString().toStdString();
  emit dataChanged(index, index, {Qt::DisplayRole});
  return true;
}

Qt::ItemFlags ChannelTreeModel::flags(const QModelIndex& index) const {
  if (!index.isValid()) return {};
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

// ── MainWindow ────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent) {
  // I18nManager initialisieren
  i18n_manager_ = std::make_shared<hh::client::I18nManager>(this);
  i18n_manager_->set_translation_directory(":/translations");
  
  setup_ui();
  apply_amber_theme();
  load_settings();

  // Timer für Statistik-Updates
  stats_timer_ = new QTimer(this);
  connect(stats_timer_, &QTimer::timeout, this, &MainWindow::on_statistics_timer);
  stats_timer_->start(1000); // Alle Sekunde
}

void MainWindow::setup_ui() {
  setWindowTitle("HasiHasi");
  setMinimumSize(600, 450);

  setup_menu_bar();
  setup_toolbar();
  setup_central_widget();
  setup_status_bar();
  setup_tray_icon();

  // PTT-Tastenkürzel
  auto* ptt_shortcut = new QShortcut(ptt_key_, this);
  connect(ptt_shortcut, &QShortcut::activated, this, &MainWindow::on_ptt_pressed);
  auto* ptt_release_shortcut = new QShortcut(ptt_key_, this);
  connect(ptt_release_shortcut, &QShortcut::shortcutActivated,
          this, [this](int) { on_ptt_released(); });
}

void MainWindow::setup_menu_bar() {
  auto* menu = menuBar();

  // Datei-Menü
  auto* file_menu = menu->addMenu("&Datei");

  auto* connect_action = new QAction("Ver&binden", this);
  connect_action->setShortcut(QKeySequence::Open);
  connect(connect_action, &QAction::triggered, this, &MainWindow::show_connect_dialog);
  file_menu->addAction(connect_action);

  file_menu->addSeparator();

  auto* quit_action = new QAction("Be&enden", this);
  quit_action->setShortcut(QKeySequence::Quit);
  connect(quit_action, &QAction::triggered, this, &QWidget::close);
  file_menu->addAction(quit_action);

  // Ansicht-Menü
  auto* view_menu = menu->addMenu("&Ansicht");

  auto* settings_action = new QAction("&Einstellungen", this);
  connect(settings_action, &QAction::triggered, this, &MainWindow::on_settings_clicked);
  view_menu->addAction(settings_action);

  // Sprache-Menü
  auto* language_menu = menu->addMenu("&Sprache");

  auto* german_action = new QAction("Deutsch", this);
  connect(german_action, &QAction::triggered, [this]() {
    if (i18n_manager_) {
      i18n_manager_->set_language(hh::client::I18nManager::Language::German);
    }
  });
  language_menu->addAction(german_action);

  auto* english_action = new QAction("English", this);
  connect(english_action, &QAction::triggered, [this]() {
    if (i18n_manager_) {
      i18n_manager_->set_language(hh::client::I18nManager::Language::English);
    }
  });
  language_menu->addAction(english_action);

  auto* french_action = new QAction("Français", this);
  connect(french_action, &QAction::triggered, [this]() {
    if (i18n_manager_) {
      i18n_manager_->set_language(hh::client::I18nManager::Language::French);
    }
  });
  language_menu->addAction(french_action);

  auto* spanish_action = new QAction("Español", this);
  connect(spanish_action, &QAction::triggered, [this]() {
    if (i18n_manager_) {
      i18n_manager_->set_language(hh::client::I18nManager::Language::Spanish);
    }
  });
  language_menu->addAction(spanish_action);

  language_menu->addSeparator();

  auto* language_dialog_action = new QAction("&Spracheinstellungen...", this);
  connect(language_dialog_action, &QAction::triggered, [this]() {
    LanguageDialog* lang_dialog = new LanguageDialog(this);
    lang_dialog->exec();
    delete lang_dialog;
  });
  language_menu->addAction(language_dialog_action);

  // Hilfe-Menü
  auto* help_menu = menu->addMenu("&Hilfe");

  auto* about_action = new QAction("&Über HasiHasi", this);
  connect(about_action, &QAction::triggered, [this]() {
    QMessageBox::about(this, "Über HasiHasi",
                       "HasiHasi v0.1.0\n"
                       "Low-Latency-Voice-Chat für Linux-Gamer\n"
                       "\n"
                       "SPDX-FileCopyrightText: 2026 Nils Pankoke\n"
                       "SPDX-License-Identifier: GPL-3.0-or-later\n"
                       "Homepage: https://hasihasi.de");
  });
  help_menu->addAction(about_action);
}

void MainWindow::setup_toolbar() {
  auto* toolbar = addToolBar("Haupttoolbar");
  toolbar->setMovable(false);

  // PTT-Button
  ptt_button_ = new QPushButton("🎤 PTT", this);
  ptt_button_->setCheckable(true);
  ptt_button_->setMaximumWidth(80);
  toolbar->addWidget(ptt_button_);

  connect(ptt_button_, &QPushButton::toggled, this, [this](bool checked) {
    if (checked) {
      on_ptt_pressed();
    } else {
      on_ptt_released();
    }
  });
}

void MainWindow::setup_central_widget() {
  splitter_ = new QSplitter(Qt::Horizontal, this);
  setCentralWidget(splitter_);

  // Kanalbaum
  channel_tree_ = new QTreeWidget(this);
  channel_tree_->setHeaderHidden(true);
  channel_tree_->setIndentation(20);
  channel_tree_->setAnimated(true);
  channel_tree_->setContextMenuPolicy(Qt::CustomContextMenu);

  channel_model_ = new ChannelTreeModel(this);
  channel_tree_->setModel(channel_model_);

  connect(channel_tree_, &QTreeWidget::itemDoubleClicked, this,
          [this](QTreeWidgetItem* item, int column) {
            (void)item;
            (void)column;
            // Kanal-beitritt oder -info
          });

  splitter_->addWidget(channel_tree_);

  // Chat-Bereich
  auto* chat_widget = new QWidget(this);
  auto* chat_layout = new QVBoxLayout(chat_widget);
  chat_layout->setContentsMargins(0, 0, 0, 0);

  chat_area_ = new QPlainTextEdit(this);
  chat_area_->setReadOnly(true);
  chat_area_->setPlaceholderText("Nachrichten erscheinen hier...");
  chat_layout->addWidget(chat_area_);

  message_input_ = new QLineEdit(this);
  message_input_->setPlaceholderText("Nachricht eingeben (ENTER zum Senden)");
  connect(message_input_, &QLineEdit::returnPressed, this, &MainWindow::on_text_message_sent);
  chat_layout->addWidget(message_input_);

  splitter_->addWidget(chat_widget);

  // splitter_ Breitenverhältnis
  splitter_->setStretchFactor(0, 1);
  splitter_->setStretchFactor(1, 2);
}

void MainWindow::setup_status_bar() {
  status_bar_ = statusBar();
  status_bar_->showMessage("Nicht verbunden");

  // Latenz-Monitor hinzufügen
  latency_label_ = new QLabel("Ø Latenz: --", this);
  status_bar_->addPermanentWidget(latency_label_);
}

void MainWindow::setup_tray_icon() {
  tray_icon_ = new QSystemTrayIcon(this);
  tray_icon_->setIcon(QIcon::fromTheme("audio-input-microphone", QIcon()));
  tray_icon_->setToolTip("HasiHasi");

  auto* tray_menu = new QMenu(this);
  auto* show_action = new QAction("Zeigen", this);
  connect(show_action, &QAction::triggered, this, [this]() {
    show();
    raise();
    activateWindow();
  });
  tray_menu->addAction(show_action);

  tray_menu->addSeparator();

  auto* quit_action = new QAction("Beenden", this);
  connect(quit_action, &QAction::triggered, this, &QWidget::close);
  tray_menu->addAction(quit_action);

  tray_icon_->setContextMenu(tray_menu);
  tray_icon_->show();

  connect(tray_icon_, &QSystemTrayIcon::activated, this, &MainWindow::on_tray_activated);
}

void MainWindow::apply_amber_theme() {
  // Amber-CRT-Theme: bernsteinfarbener Phosphor auf Schwarz
  setStyleSheet(R"(
    QMainWindow {
      background-color: #0A0A08;
    }

    QMenu {
      background-color: #141410;
      color: #FFB000;
      border: 1px solid #3A2A00;
      padding: 4px;
    }

    QMenu::item {
      padding: 6px 20px;
      border-radius: 4px;
    }

    QMenu::item:selected {
      background-color: #3A2A00;
      color: #FFCC33;
    }

    QMenuBar {
      background-color: #141410;
      color: #FFB000;
      border-bottom: 1px solid #3A2A00;
      padding: 4px;
    }

    QMenuBar::item:selected {
      background-color: #3A2A00;
    }

    QToolBar {
      background-color: #141410;
      border: none;
      border-bottom: 1px solid #3A2A00;
      padding: 4px;
      spacing: 4px;
    }

    QToolBar QToolButton {
      background-color: transparent;
      color: #FFB000;
      border: none;
      border-radius: 6px;
      padding: 6px 12px;
    }

    QToolBar QToolButton:hover {
      background-color: #3A2A00;
      color: #FFCC33;
    }

    QToolBar QToolButton:checked {
      background-color: #FFB000;
      color: #0A0A08;
    }

    QTreeWidget {
      background-color: #0A0A08;
      color: #FFB000;
      border: none;
      outline: none;
      font-size: 13px;
    }

    QTreeWidget::item {
      padding: 4px 0;
      border-radius: 4px;
    }

    QTreeWidget::item:selected {
      background-color: #3A2A00;
      color: #FFCC33;
    }

    QTreeWidget::item:hover {
      background-color: #1A1A14;
    }

    QPlainTextEdit {
      background-color: #0A0A08;
      color: #FFB000;
      border: none;
      font-family: 'IBM Plex Mono', 'JetBrains Mono', monospace;
      font-size: 13px;
    }

    QPlainTextEdit > QTextDocument {
      color: #FFB000;
    }

    QLineEdit {
      background-color: #141410;
      color: #FFB000;
      border: 1px solid #3A2A00;
      border-radius: 6px;
      padding: 6px 10px;
      font-family: 'Inter', sans-serif;
      font-size: 13px;
    }

    QLineEdit:focus {
      border: 1px solid #FFB000;
    }

    QPushButton {
      background-color: #141410;
      color: #FFB000;
      border: 1px solid #3A2A00;
      border-radius: 6px;
      padding: 6px 16px;
      font-family: 'Inter', sans-serif;
      font-size: 13px;
    }

    QPushButton:hover {
      background-color: #3A2A00;
      color: #FFCC33;
    }

    QPushButton:checked {
      background-color: #FFB000;
      color: #0A0A08;
    }

    QStatusBar {
      background-color: #141410;
      color: #B07800;
      border-top: 1px solid #3A2A00;
      padding: 4px;
      font-size: 12px;
    }

    QSplitter::handle {
      background-color: #3A2A00;
      width: 2px;
    }

    QScrollBar:vertical {
      background-color: #0A0A08;
      width: 10px;
      border: none;
    }

    QScrollBar::handle:vertical {
      background-color: #3A2A00;
      border-radius: 5px;
      min-height: 20px;
    }

    QScrollBar::handle:vertical:hover {
      background-color: #FFB000;
    }

    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
      height: 0px;
    }

    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
      background: none;
    }

    QCheckBox {
      color: #FFB000;
      spacing: 8px;
    }

    QCheckBox::indicator {
      width: 16px;
      height: 16px;
      border: 1px solid #3A2A00;
      border-radius: 4px;
      background-color: #141410;
    }

    QCheckBox::indicator:checked {
      background-color: #FFB000;
      border: 1px solid #FFB000;
    }

    QSlider::groove:horizontal {
      background: #3A2A00;
      height: 4px;
      border-radius: 2px;
    }

    QSlider::handle:horizontal {
      background: #FFB000;
      width: 14px;
      height: 14px;
      margin: -5px 0;
      border-radius: 7px;
    }

    QGroupBox {
      color: #FFB000;
      border: 1px solid #3A2A00;
      border-radius: 6px;
      margin-top: 8px;
      padding-top: 12px;
      font-size: 13px;
    }

    QGroupBox::title {
      subcontrol-origin: margin;
      left: 10px;
      padding: 0 6px;
      color: #FFCC33;
    }

    QComboBox {
      background-color: #141410;
      color: #FFB000;
      border: 1px solid #3A2A00;
      border-radius: 6px;
      padding: 4px 8px;
      font-size: 13px;
    }

    QComboBox::drop-down {
      border: none;
    }

    QComboBox::down-arrow {
      image: none;
      border-left: 4px solid transparent;
      border-right: 4px solid transparent;
      border-top: 6px solid #FFB000;
      margin-right: 6px;
    }

    QComboBox QAbstractItemView {
      background-color: #141410;
      color: #FFB000;
      selection-background-color: #3A2A00;
      border: 1px solid #3A2A00;
    }

    QProgressBar {
      background-color: #141410;
      border: 1px solid #3A2A00;
      border-radius: 6px;
      text-align: center;
      color: #FFB000;
      font-size: 11px;
    }

    QProgressBar::chunk {
      background-color: #FFB000;
      border-radius: 5px;
    }
  )");
}

void MainWindow::load_settings() {
  QSettings settings("hasihasi", "hasihasi");
  if (settings.contains("geometry")) {
    restoreGeometry(settings.value("geometry").toByteArray());
  }
  if (settings.contains("ptt_key")) {
    ptt_key_ = QKeySequence(settings.value("ptt_key").toString());
  }
  if (settings.contains("ptt_enabled")) {
    ptt_enabled_ = settings.value("ptt_enabled", true).toBool();
  }
}

void MainWindow::save_settings() {
  QSettings settings("hasihasi", "hasihasi");
  settings.setValue("geometry", saveGeometry());
  settings.setValue("ptt_key", ptt_key_.toString());
  settings.setValue("ptt_enabled", ptt_enabled_);
}

void MainWindow::set_session(std::shared_ptr<hh::client::Session> session) {
  session_ = session;
}

void MainWindow::show_connect_dialog() {
  QDialog dialog(this);
  dialog.setWindowTitle("Mit HasiHasi-Server verbinden");
  dialog.setMinimumWidth(400);

  auto* layout = new QFormLayout(&dialog);

  QLineEdit host_input("localhost");
  QLineEdit port_input("64738");
  QLineEdit username_input;

  layout->addRow("Server:", &host_input);
  layout->addRow("Port:", &port_input);
  layout->addRow("Benutzername:", &username_input);

  auto* connect_button = new QPushButton("Verbinden", &dialog);
  auto* cancel_button = new QPushButton("Abbrechen", &dialog);

  auto* button_layout = new QHBoxLayout();
  button_layout->addWidget(connect_button);
  button_layout->addWidget(cancel_button);
  layout->addRow(button_layout);

  connect(connect_button, &QPushButton::clicked, &dialog, [&]() {
    if (username_input.text().isEmpty()) {
      QMessageBox::warning(&dialog, "Fehler", "Benutzername darf nicht leer sein.");
      return;
    }
    set_username(username_input.text().toStdString());
    set_connection_status("Verbinde...");
    dialog.accept();
    on_connect();
  });

  connect(cancel_button, &QPushButton::clicked, &dialog, &QDialog::reject);

  dialog.exec();
}

void MainWindow::on_connect() {
  if (!session_) {
    chat_area_->appendPlainText(
      QString("[%1] Fehler: Keine Session").arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
    return;
  }

  // Voice-Pipeline erstellen
  if (!voice_pipeline_) {
    hh::client::VoicePipeline::Config config;
    config.enable_vad = vad_enabled_;
    config.vad_threshold = static_cast<float>(vad_threshold_);

    voice_pipeline_ = std::make_shared<hh::client::VoicePipeline>(config);

    // Voice-Status-Callback
    voice_pipeline_->set_audio_status_callback([this](double volume, bool speaking) {
      on_voice_status(volume, speaking);

      // Latenz-Monitor aktualisieren
      if (speaking) {
        // Simulierte Latenz basierend auf Netzwerk-Statistik
        double simulated_latency = stats_rtt_ms_ / 2.0 + 10.0; // RTT/2 + Codec-Latenz
        add_latency_sample(simulated_latency);
      }
    });
  }

  // Callbacks registrieren
  session_->set_connection_callback([this](bool success, const std::string& message) {
    set_connection_status(message);
    chat_area_->appendPlainText(
      QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"),
                             QString::fromStdString(message)));

    if (success) {
      // Kanal-Liste anfordern
      session_->request_channel_list();

      // Voice-Pipeline starten
      if (voice_pipeline_) {
        voice_pipeline_->start(session_);
        chat_area_->appendPlainText(
          QString("[%1] Voice-Pipeline gestartet").arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
      }
    }
  });

  session_->set_channel_update_callback([this](const Session::ChannelList& channels) {
    update_channel_list(channels);
    chat_area_->appendPlainText(
      QString("[%1] Kanal-Liste aktualisiert (%2 Kanäle)")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"),
             QString::number(channels.size())));
  });

  session_->set_user_update_callback([this](uint32_t channel_id, const Session::UserList& users) {
    update_user_list(channel_id, users);
  });

  session_->set_text_message_callback([this](uint32_t from_user_id, const std::string& text) {
    (void)from_user_id;
    chat_area_->appendPlainText(
      QString("[%1] Nachricht: %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"),
                                         QString::fromStdString(text)));
  });

  session_->set_voice_data_callback([this](std::span<const uint8_t> voice_data) {
    // Voice-Daten an Jitter-Buffer in Voice-Pipeline weiterleiten
    // TODO: Jitter-Buffer füllen
    (void)voice_data;
  });

  // Verbindung herstellen
  session_->connect("localhost", 64738);
  set_connection_status("Verbinde...");
  chat_area_->appendPlainText(
    QString("[%1] Verbinde mit localhost:64738").arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

void MainWindow::on_disconnect() {
  if (session_) {
    session_->disconnect();
  }
  set_connection_status("Getrennt");
  chat_area_->appendPlainText(
    QString("[%1] Von Server getrennt").arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

void MainWindow::on_ptt_pressed() {
  ptt_active_ = true;
  if (ptt_button_) {
    ptt_button_->setChecked(true);
  }

  // Voice-Pipeline unmute
  if (voice_pipeline_) {
    voice_pipeline_->set_muted(false);
  }

  hh::get_logger().info("PTT activated");
}

void MainWindow::on_ptt_released() {
  ptt_active_ = false;
  if (ptt_button_) {
    ptt_button_->setChecked(false);
  }

  // Voice-Pipeline mute
  if (voice_pipeline_) {
    voice_pipeline_->set_muted(true);
  }

  hh::get_logger().info("PTT deactivated");
}

void MainWindow::on_channel_double_clicked(const QModelIndex& index) {
  (void)index;
  // Kanal-beitritt oder -info
}

void MainWindow::on_text_message_sent() {
  if (message_input_->text().isEmpty()) return;

  QString text = message_input_->text();
  chat_area_->appendPlainText(
    QString("[%1] Du: %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), text));
  message_input_->clear();

  // TODO: Nachricht an Server senden
  (void)text;
}

void MainWindow::on_settings_clicked() {
  QDialog dialog(this);
  dialog.setWindowTitle("Einstellungen");
  dialog.setMinimumWidth(450);

  auto* layout = new QFormLayout(&dialog);

  // PTT-Einstellungen
  auto* ptt_group = new QGroupBox("Push-to-Talk", &dialog);
  auto* ptt_layout = new QFormLayout(ptt_group);

  QCheckBox ptt_enabled_check(true);
  ptt_enabled_check.setChecked(ptt_enabled_);
  ptt_layout->addRow("PTT aktiviert:", &ptt_enabled_check);

  QLineEdit ptt_key_display(ptt_key_.toString());
  ptt_key_display.setReadOnly(true);
  ptt_layout->addRow("Taste:", &ptt_key_display);

  auto* change_key_button = new QPushButton("Taste ändern", &dialog);
  ptt_layout->addRow("", change_key_button);

  connect(change_key_button, &QPushButton::clicked, [&]() {
    QMessageBox::information(&dialog, "Taste ändern",
                             "Drücke die gewünschte Taste...");
    // In echter Implementierung: Key-grabbing
  });

  layout->addRow(ptt_group);

  // VAD-Einstellungen
  auto* vad_group = new QGroupBox("Voice-Activation-Detection", &dialog);
  auto* vad_layout = new QFormLayout(vad_group);

  QCheckBox vad_enabled_check(false);
  vad_layout->addRow("VAD aktiviert:", &vad_enabled_check);

  QSlider vad_threshold_slider(Qt::Horizontal);
  vad_threshold_slider.setRange(0, 100);
  vad_threshold_slider.setValue(vad_threshold_);
  vad_layout->addRow("Schwellwert:", &vad_threshold_slider);

  QLabel vad_threshold_label(QString::number(vad_threshold_));
  vad_layout->addRow("Wert:", &vad_threshold_label);

  connect(&vad_threshold_slider, &QSlider::valueChanged, &vad_threshold_label,
          [&vad_threshold_label](int value) {
            vad_threshold_label.setText(QString::number(value));
          });

  layout->addRow(vad_group);

  // OK/Cancel
  auto* button_layout = new QHBoxLayout();
  auto* ok_button = new QPushButton("OK", &dialog);
  auto* cancel_button = new QPushButton("Abbrechen", &dialog);
  button_layout->addWidget(ok_button);
  button_layout->addWidget(cancel_button);
  layout->addRow(button_layout);

  connect(ok_button, &QPushButton::clicked, &dialog, [&]() {
    ptt_enabled_ = ptt_enabled_check.isChecked();
    vad_enabled_ = vad_enabled_check.isChecked();
    vad_threshold_ = vad_threshold_slider.value();
    save_settings();
    dialog.accept();
  });

  connect(cancel_button, &QPushButton::clicked, &dialog, &QDialog::reject);

  dialog.exec();
}

void MainWindow::on_tray_activated(QSystemTrayIcon::ActivationReason reason) {
  if (reason == QSystemTrayIcon::Trigger) {
    show();
    raise();
    activateWindow();
  }
}

void MainWindow::on_statistics_timer() {
  // Statistik-Updates (wenn verbunden)
  if (!connection_status_.empty() && connection_status_ != "Getrennt" &&
      connection_status_ != "Nicht verbunden") {
    QString stats_text = QString("RTT: %.1fms | Jitter: %.1fms | Verlust: %.1f%% | Bitrate: %dkbit/s | Latenz: %dms")
                           .arg(stats_rtt_ms_)
                           .arg(stats_jitter_ms_)
                           .arg(stats_loss_percent_)
                           .arg(stats_bitrate_kbit_)
                           .arg(stats_latency_ms_);

    // Latenz-Monitor aktualisieren
    if (latency_label_) {
      latency_label_->setText(QString("Ø Latenz: %.1fms | Min: %.1fms | Max: %.1fms")
                                .arg(avg_latency_)
                                .arg(min_latency_)
                                .arg(max_latency_));
    }
  }
}

void MainWindow::on_voice_status(double volume, bool speaking) {
  // PTT-Button visuelles Feedback
  if (ptt_button_) {
    if (speaking && volume > 5.0) {
      ptt_button_->setStyleSheet("QPushButton { background-color: #FFCC33; color: #0A0A08; }");
    } else {
      ptt_button_->setStyleSheet("");
    }
  }

  // Status-Bar aktualisieren
  if (status_bar_) {
    status_bar_->showMessage(QString("Volume: %.0f%% | Speaking: %1")
                               .arg(volume)
                               .arg(speaking ? "Ja" : "Nein"), 3000);
  }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  // PTT-Taste (wenn nicht in Input-Fokus)
  if (ptt_enabled_ && event->key() == ptt_key_.key(0) &&
      focusWidget() != message_input_) {
    on_ptt_pressed();
  }

  QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
  // PTT-Taste loslassen
  if (ptt_active_ && event->key() == ptt_key_.key(0)) {
    on_ptt_released();
  }

  QMainWindow::keyReleaseEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  save_settings();
  event->accept();
}

// ── Status-Updates ────────────────────────────────────────────────────

void MainWindow::set_connection_status(const std::string& status) {
  connection_status_ = status;
  if (status_bar_) {
    status_bar_->showMessage(QString::fromStdString(status));
  }
}

void MainWindow::set_username(const std::string& username) {
  username_ = username;
  setWindowTitle(QString("HasiHasi - %1").arg(QString::fromStdString(username)));
}

void MainWindow::update_channel_list(const std::vector<std::pair<uint32_t, std::string>>& channels) {
  channel_model_->beginResetModel();
  channel_model_->endResetModel();

  for (const auto& [id, name] : channels) {
    channel_model_->add_channel(id, 0, name, 0, false, "");
  }
}

void MainWindow::update_user_list(uint32_t channel_id,
                                   const std::vector<std::pair<uint32_t, std::string>>& users) {
  (void)channel_id;
  (void)users;
  // TODO: Nutzer im Kanalbaum anzeigen
}

void MainWindow::set_user_speaking(uint32_t user_id, bool speaking) {
  channel_model_->set_speaking(user_id, speaking);
}

void MainWindow::update_stats(double rtt_ms, double jitter_ms, double loss_percent,
                               int bitrate_kbit, int latency_ms) {
  stats_rtt_ms_ = rtt_ms;
  stats_jitter_ms_ = jitter_ms;
  stats_loss_percent_ = loss_percent;
  stats_bitrate_kbit_ = bitrate_kbit;
  stats_latency_ms_ = latency_ms;
}

void MainWindow::add_latency_sample(double latency_ms) {
  latency_samples_.push_back(latency_ms);

  // Max 100 Samples behalten
  if (latency_samples_.size() > 100) {
    latency_samples_.erase(latency_samples_.begin());
  }

  // Durchschnitt, Minimum, Maximum berechnen
  double sum = 0.0;
  min_latency_ = 1000.0;
  max_latency_ = 0.0;

  for (double sample : latency_samples_) {
    sum += sample;
    if (sample < min_latency_) min_latency_ = sample;
    if (sample > max_latency_) max_latency_ = sample;
  }

  avg_latency_ = sum / latency_samples_.size();
}

} // namespace hh::client::ui
