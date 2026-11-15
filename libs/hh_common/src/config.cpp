// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/config.hpp"

#include <toml++/toml.hpp>

namespace hh {

Config::Config(const std::string& path)
  : path_(path) {
  load();
}

void Config::load() {
  auto parsed = toml::parse(path_);

  for (auto& [k, v] : parsed) {
    if (v.is_string()) {
      const toml::value<std::string>* str_val = v.as_string();
      if (str_val) {
        values_[std::string(k)] = std::string(*str_val);
      }
    }
  }
}

std::string Config::get(const std::string& key, const std::string& default_value) const {
  auto it = values_.find(key);
  return it != values_.end() ? it->second : default_value;
}

int Config::get_int(const std::string& key, int default_value) const {
  auto it = values_.find(key);
  return it != values_.end() ? std::stoi(it->second) : default_value;
}

bool Config::get_bool(const std::string& key, bool default_value) const {
  auto it = values_.find(key);
  if (it == values_.end()) return default_value;
  auto& val = it->second;
  return val == "true" || val == "1" || val == "yes";
}

} // namespace hh
