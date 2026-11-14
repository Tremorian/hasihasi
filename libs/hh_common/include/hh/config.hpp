// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <unordered_map>

namespace hh {

class Config {
public:
  explicit Config(const std::string& path);

  std::string get(const std::string& key, const std::string& default_value = {}) const;
  int get_int(const std::string& key, int default_value = 0) const;
  bool get_bool(const std::string& key, bool default_value = false) const;

  const std::string& path() const { return path_; }

private:
  std::string path_;
  std::unordered_map<std::string, std::string> values_;

  void load();
};

} // namespace hh
