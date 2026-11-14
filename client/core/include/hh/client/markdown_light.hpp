// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <functional>

namespace hh::client {

// Markdown light für Textchat
// Unterstützt: **fett**, *kursiv*, `Code`, ~durchgestrichen~, > Zitate, - Listen, ``` Code-Blöcke
class MarkdownLight {
public:
  MarkdownLight() = default;
  ~MarkdownLight() = default;

  // Markdown zu HTML konvertieren
  std::string to_html(const std::string& markdown) const;
  
  // Markdown zu Plain-Text konvertieren (für Barrierefreiheit)
  std::string to_plain(const std::string& markdown) const;
  
  // Markdown zu Qt-Rich-Text konvertieren
  std::string to_rich_text(const std::string& markdown) const;

  // Syntax-Highlighting für Code-Blöcke
  enum class Language {
    None,
    CPP,
    Python,
    JavaScript,
    Bash,
    Rust
  };

  struct ParsedElement {
    enum class Type {
      Text,
      Bold,
      Italic,
      Code,
      Strikethrough,
      Quote,
      List,
      CodeBlock,
      Link,
      LineBreak
    };

    Type type;
    std::string content;
    std::string language; // Für Code-Blöcke
    std::string url;      // Für Links
  };

  // Markdown in Elemente parsen
  std::vector<ParsedElement> parse(const std::string& markdown) const;

  // Callbacks
  using ElementHandler = std::function<void(const ParsedElement& element)>;
  void render(const std::string& markdown, ElementHandler handler) const;

private:
  // Helper-Funktionen
  std::string escape_html(const std::string& text) const;
  std::string highlight_code(const std::string& code, Language lang) const;
  Language detect_language(const std::string& lang_str) const;
};

} // namespace hh::client
