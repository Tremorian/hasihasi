// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/ui/aboutdialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QDesktopServices>
#include <QUrl>
#include <QIcon>

namespace hh::client {

AboutDialog::AboutDialog(QWidget *parent)
  : QDialog(parent) {
  setWindowTitle("Über HasiHasi");
  setMinimumSize(500, 400);
  setup_ui();
}

void AboutDialog::setup_ui() {
  auto *main_layout = new QVBoxLayout(this);
  
  setup_about_section();
  setup_license_section();
  setup_credits_section();
  
  auto *button_layout = new QHBoxLayout();
  license_btn_ = new QPushButton("Lizenz anzeigen");
  github_btn_ = new QPushButton("GitHub");
  close_btn_ = new QPushButton("Schließen");
  
  button_layout->addWidget(license_btn_);
  button_layout->addWidget(github_btn_);
  button_layout->addStretch();
  button_layout->addWidget(close_btn_);
  
  main_layout->addStretch();
  main_layout->addLayout(button_layout);
  
  connect(license_btn_, &QPushButton::clicked, this, &AboutDialog::on_license_clicked);
  connect(github_btn_, &QPushButton::clicked, this, &AboutDialog::on_github_clicked);
  connect(close_btn_, &QPushButton::clicked, this, &AboutDialog::on_close_clicked);
}

void AboutDialog::setup_about_section() {
  auto *layout = new QHBoxLayout();
  
  logo_label_ = new QLabel();
  logo_label_->setFixedSize(64, 64);
  logo_label_->setScaledContents(true);
  // Platzhalter-Logo (kann später durch echtes Logo ersetzt werden)
  QPixmap pixmap(64, 64);
  pixmap.fill(Qt::transparent);
  logo_label_->setPixmap(pixmap);
  
  auto *text_layout = new QVBoxLayout();
  
  title_label_ = new QLabel("HasiHasi");
  title_label_->setFontSize(18);
  title_label_->setFont QFont("Sans Serif", 18, QFont::Bold));
  
  version_label_ = new QLabel("Version 0.1.0");
  version_label_->setFont(QFont("Sans Serif", 10));
  
  description_label_ = new QLabel(
    "Low-Latency Linux Voice Chat\n"
    "SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>\n"
    "SPDX-License-Identifier: GPL-3.0-or-later"
  );
  description_label_->setFont(QFont("Sans Serif", 9));
  description_label_->setWordWrap(true);
  
  text_layout->addWidget(title_label_);
  text_layout->addWidget(version_label_);
  text_layout->addWidget(description_label_);
  
  layout->addWidget(logo_label_);
  layout->addLayout(text_layout);
  
  main_layout->addLayout(layout);
}

void AboutDialog::setup_license_section() {
  license_browser_ = new QTextBrowser();
  license_browser_->setReadOnly(true);
  license_browser_->setHtml(
    "<h3>Lizenz</h3>"
    "<p>HasiHasi ist unter der <a href='https://www.gnu.org/licenses/gpl-3.0-or-later.html'>GPL-3.0-or-later</a> Lizenz verfügbar.</p>"
    "<p>Dies ist ein Freie-Software-Projekt. Sie können es unter den Bedingungen der GNU General Public License, wie von der Free Software Foundation veröffentlicht, weiterverbreiten und/oder modifizieren.</p>"
  );
  license_browser_->setMaximumHeight(100);
  
  main_layout->addWidget(license_browser_);
}

void AboutDialog::setup_credits_section() {
  auto *credits_layout = new QVBoxLayout();
  
  QLabel *credits_label = new QLabel(
    "<h3>Danksagungen</h3>"
    "<p>HasiHasi verwendet folgende Bibliotheken:</p>"
    "<ul>"
    "<li><a href='https://www.opus-codec.org/'>Opus</a> – Audiocodierung</li>"
    "<li><a href='https://www.pipewire.org/'>PipeWire</a> – Audio/Video-Streaming</li>"
    "<li><a href='https://www.qt.io/'>Qt6</a> – GUI-Framework</li>"
    "<li><a href='https://github.com/google/protobuf'>Protobuf</a> – Serialisierung</li>"
    "<li><a href='https://github.com/ninja-build/ninja'>Ninja</a> – Build-System</li>"
    "<li><a href='https://rnnoise.org/'>RNNoise</a> – Rauschunterdrückung</li>"
    "</ul>"
  );
  credits_label->setWordWrap(true);
  credits_label->setOpenExternalLinks(true);
  
  credits_layout->addWidget(credits_label);
  main_layout->addLayout(credits_layout);
}

void AboutDialog::on_license_clicked() {
  // Vollständige Lizenz anzeigen (kann als neue Dialogseite oder in einem neuen Fenster geöffnet werden)
  license_browser_->setHtml(
    "<h3>GPL-3.0-or-later</h3>"
    "<p>Vollständiger Lizenztext unter: <a href='https://www.gnu.org/licenses/gpl-3.0-or-later.html'>https://www.gnu.org/licenses/gpl-3.0-or-later.html</a></p>"
    "<p>Oder in der Datei: LICENSES/GPL-3.0-or-later.txt</p>"
  );
}

void AboutDialog::on_github_clicked() {
  QDesktopServices::openUrl(QUrl("https://github.com/tremorian/hasihasi"));
}

void AboutDialog::on_close_clicked() {
  close();
}

} // namespace hh::client
