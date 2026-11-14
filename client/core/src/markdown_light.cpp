// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/markdown_light.hpp"

#include "hh/logging.hpp"

#include <sstream>
#include <algorithm>
#include <regex>

namespace hh::client {

std::string MarkdownLight::to_html(const std::string& markdown) const {
  std::string html = "<div>";
  
  auto elements = parse(markdown);
  for (const auto& element : elements) {
    switch (element.type) {
      case ParsedElement::Type::Text:
        html += escape_html(element.content);
        break;
      case ParsedElement::Type::Bold:
        html += "<b>" + escape_html(element.content) + "</b>";
        break;
      case ParsedElement::Type::Italic:
        html += "<i>" + escape_html(element.content) + "</i>";
        break;
      case ParsedElement::Type::Code:
        html += "<code>" + escape_html(element.content) + "</code>";
        break;
      case ParsedElement::Type::Strikethrough:
        html += "<s>" + escape_html(element.content) + "</s>";
        break;
      case ParsedElement::Type::Quote:
        html += "<blockquote>" + escape_html(element.content) + "</blockquote>";
        break;
      case ParsedElement::Type::List:
        html += "<li>" + escape_html(element.content) + "</li>";
        break;
      case ParsedElement::Type::CodeBlock:
        html += "<pre><code>" + escape_html(element.content) + "</code></pre>";
        break;
      case ParsedElement::Type::Link:
        html += "<a href=\"" + element.url + "\">" + escape_html(element.content) + "</a>";
        break;
      case ParsedElement::Type::LineBreak:
        html += "<br>";
        break;
    }
  }
  
  html += "</div>";
  return html;
}

std::string MarkdownLight::to_plain(const std::string& markdown) const {
  std::string plain = markdown;
  
  // Markdown-Syntax entfernen
  plain.erase(std::remove(plain.begin(), plain.end(), '*'), plain.end());
  plain.erase(std::remove(plain.begin(), plain.end(), '`'), plain.end());
  plain.erase(std::remove(plain.begin(), plain.end(), '~'), plain.end());
  plain.erase(std::remove(plain.begin(), plain.end(), '>'), plain.end());
  plain.erase(std::remove(plain.begin(), plain.end(), '-'), plain.end());
  
  return plain;
}

std::string MarkdownLight::to_rich_text(const std::string& markdown) const {
  // Qt Rich Text ist ähnlich zu HTML
  return to_html(markdown);
}

std::vector<MarkdownLight::ParsedElement> MarkdownLight::parse(const std::string& markdown) const {
  std::vector<ParsedElement> elements;
  std::istringstream stream(markdown);
  std::string line;
  
  while (std::getline(stream, line)) {
    // Code-Block
    if (line.substr(0, 3) == "```") {
      ParsedElement elem;
      elem.type = ParsedElement::Type::CodeBlock;
      elem.language = line.substr(3);
      
      // Code bis zum nächsten ``` sammeln
      std::string code;
      while (std::getline(stream, line) && line.substr(0, 3) != "```") {
        code += line + "\n";
      }
      elem.content = code;
      elements.push_back(elem);
      continue;
    }
    
    // Zitat
    if (line.substr(0, 1) == ">") {
      ParsedElement elem;
      elem.type = ParsedElement::Type::Quote;
      elem.content = line.substr(1);
      elements.push_back(elem);
      continue;
    }
    
    // Liste
    if (line.substr(0, 1) == "-") {
      ParsedElement elem;
      elem.type = ParsedElement::Type::List;
      elem.content = line.substr(1);
      elements.push_back(elem);
      continue;
    }
    
    // Leerzeile = LineBreak
    if (line.empty()) {
      ParsedElement elem;
      elem.type = ParsedElement::Type::LineBreak;
      elements.push_back(elem);
      continue;
    }
    
    // Inline-Formatierung
    ParsedElement elem;
    elem.type = ParsedElement::Type::Text;
    elem.content = line;
    elements.push_back(elem);
  }
  
  return elements;
}

void MarkdownLight::render(const std::string& markdown, ElementHandler handler) const {
  auto elements = parse(markdown);
  for (const auto& element : elements) {
    handler(element);
  }
}

std::string MarkdownLight::escape_html(const std::string& text) const {
  std::string result;
  result.reserve(text.size());
  
  for (char c : text) {
    switch (c) {
      case '&': result += "&amp;"; break;
      case '<': result += "&lt;"; break;
      case '>': result += "&gt;"; break;
      case '"': result += "&quot;"; break;
      case '\'': result += "&#39;"; break;
      default: result += c; break;
    }
  }
  
  return result;
}

std::string MarkdownLight::highlight_code(const std::string& code, Language lang) const {
  // Einfaches Syntax-Highlighting (kann später erweitert werden)
  (void)code;
  (void)lang;
  return code;
}

MarkdownLight::Language MarkdownLight::detect_language(const std::string& lang_str) const {
  if (lang_str == "cpp" || lang_str == "c++" || lang_str == "cc") {
    return Language::CPP;
  } else if (lang_str == "python" || lang_str == "py") {
    return Language::Python;
  } else if (lang_str == "javascript" || lang_str == "js") {
    return Language::JavaScript;
  } else if (lang_str == "bash" || lang_str == "sh" || lang_str == "shell") {
    return Language::Bash;
  } else if (lang_str == "rust" || lang_str == "rs") {
    return Language::Rust;
  }
  return Language::None;
}

} // namespace hh::client
