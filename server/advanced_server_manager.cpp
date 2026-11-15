// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "advanced_server_manager.hpp"

#include <QRandomGenerator>

namespace hh::server {

AdvancedServerManager::AdvancedServerManager(QObject* parent)
  : QObject(parent) {
  
  // Standard-Konfiguration
  config_.max_connections = 1000;
  config_.voice_packet_size = 960;
  config_.voice_sample_rate = 48000;
  config_.voice_channels = 1;
  config_.session_timeout_ms = 3600000; // 1 Stunde
  config_.enable_logging = true;
  config_.enable_metrics = true;
  config_.log_level = "INFO";
}

AdvancedServerManager::AdvancedServerManager(AdvancedServerManager&& other) noexcept
  : QObject(other.parent())
  , instances_(std::move(other.instances_))
  , current_instance_id_(std::move(other.current_instance_id_))
  , database_status_(other.database_status_)
  , postgresql_host_(std::move(other.postgresql_host_))
  , postgresql_port_(other.postgresql_port_)
  , postgresql_dbname_(std::move(other.postgresql_dbname_))
  , postgresql_user_(std::move(other.postgresql_user_))
  , postgresql_password_(std::move(other.postgresql_password_))
  , load_balance_strategy_(other.load_balance_strategy_)
  , server_nodes_(std::move(other.server_nodes_))
  , current_node_index_(other.current_node_index_)
  , replication_status_(other.replication_status_)
  , remote_replication_host_(std::move(other.remote_replication_host_))
  , remote_replication_port_(other.remote_replication_port_)
  , bytes_replicated_(other.bytes_replicated_)
  , migration_status_(other.migration_status_)
  , config_(other.config_)
  , scaling_policy_(other.scaling_policy_)
  , scaling_metrics_(other.scaling_metrics_)
  , backup_status_(other.backup_status_)
  , backups_(std::move(other.backups_))
  , current_backup_id_(std::move(other.current_backup_id_))
  , api_endpoints_(std::move(other.api_endpoints_))
  , stats_(other.stats_)
  , instance_callback_(std::move(other.instance_callback_))
  , database_callback_(std::move(other.database_callback_))
  , migration_callback_(std::move(other.migration_callback_)) {
  
  other.database_status_ = DatabaseStatus::DISCONNECTED;
  other.load_balance_strategy_ = LoadBalanceStrategy::ROUND_ROBIN;
  other.current_node_index_ = 0;
  other.replication_status_ = ReplicationStatus::DISABLED;
  other.remote_replication_port_ = 5432;
  other.bytes_replicated_ = 0;
  other.migration_status_ = MigrationStatus::IDLE;
  other.stats_ = Stats{};
}

AdvancedServerManager& AdvancedServerManager::operator=(AdvancedServerManager&& other) noexcept {
  if (this != &other) {
    QObject::operator=(std::move(other));
    instances_ = std::move(other.instances_);
    current_instance_id_ = std::move(other.current_instance_id_);
    database_status_ = other.database_status_;
    postgresql_host_ = std::move(other.postgresql_host_);
    postgresql_port_ = other.postgresql_port_;
    postgresql_dbname_ = std::move(other.postgresql_dbname_);
    postgresql_user_ = std::move(other.postgresql_user_);
    postgresql_password_ = std::move(other.postgresql_password_);
    load_balance_strategy_ = other.load_balance_strategy_;
    server_nodes_ = std::move(other.server_nodes_);
    current_node_index_ = other.current_node_index_;
    replication_status_ = other.replication_status_;
    remote_replication_host_ = std::move(other.remote_replication_host_);
    remote_replication_port_ = other.remote_replication_port_;
    bytes_replicated_ = other.bytes_replicated_;
    migration_status_ = other.migration_status_;
    config_ = other.config_;
    scaling_policy_ = other.scaling_policy_;
    scaling_metrics_ = other.scaling_metrics_;
    backup_status_ = other.backup_status_;
    backups_ = std::move(other.backups_);
    current_backup_id_ = std::move(other.current_backup_id_);
    api_endpoints_ = std::move(other.api_endpoints_);
    stats_ = other.stats_;
    instance_callback_ = std::move(other.instance_callback_);
    database_callback_ = std::move(other.database_callback_);
    migration_callback_ = std::move(other.migration_callback_);
    other.database_status_ = DatabaseStatus::DISCONNECTED;
    other.load_balance_strategy_ = LoadBalanceStrategy::ROUND_ROBIN;
    other.current_node_index_ = 0;
    other.replication_status_ = ReplicationStatus::DISABLED;
    other.remote_replication_port_ = 5432;
    other.bytes_replicated_ = 0;
    other.migration_status_ = MigrationStatus::IDLE;
    other.stats_ = Stats{};
  }
  return *this;
}

// ── Multi-Instance ───────────────────────────────────────────────────

std::vector<AdvancedServerManager::InstanceInfo> AdvancedServerManager::get_instances() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<InstanceInfo> result;
  for (const auto& pair : instances_) {
    result.push_back(pair.second);
  }
  return result;
}

AdvancedServerManager::InstanceInfo* AdvancedServerManager::get_instance(const QString& id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = instances_.find(id);
  if (it != instances_.end()) {
    return &it->second;
  }
  return nullptr;
}

const AdvancedServerManager::InstanceInfo* AdvancedServerManager::get_instance(const QString& id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = instances_.find(id);
  if (it != instances_.end()) {
    return &it->second;
  }
  return nullptr;
}

bool AdvancedServerManager::start_instance(const QString& id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = instances_.find(id);
  if (it != instances_.end()) {
    it->second.is_running = true;
    it->second.started_at = QDateTime::currentDateTime();
    stats_.instances_started++;
    
    emit instance_started(id);
    
    if (instance_callback_) {
      instance_callback_(id, true);
    }
    
    return true;
  }
  return false;
}

bool AdvancedServerManager::stop_instance(const QString& id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = instances_.find(id);
  if (it != instances_.end()) {
    it->second.is_running = false;
    stats_.instances_stopped++;
    
    emit instance_stopped(id);
    
    if (instance_callback_) {
      instance_callback_(id, false);
    }
    
    return true;
  }
  return false;
}

bool AdvancedServerManager::restart_instance(const QString& id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = instances_.find(id);
  if (it != instances_.end()) {
    it->second.is_running = false;
    it->second.started_at = QDateTime::currentDateTime();
    stats_.instances_stopped++;
    stats_.instances_started++;
    
    emit instance_started(id);
    return true;
  }
  return false;
}

bool AdvancedServerManager::create_instance(const QString& name, int port, int max_users) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  InstanceInfo info;
  info.id = QUuid::createUuid().toString().mid(1, 8);
  info.name = name;
  info.port = port;
  info.max_users = max_users;
  info.current_users = 0;
  info.is_running = false;
  info.started_at = QDateTime::currentDateTime();
  info.config_path = "/etc/hasihasi/instances/" + info.id + "/config.toml";
  
  instances_[info.id] = info;
  return true;
}

bool AdvancedServerManager::delete_instance(const QString& id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = instances_.find(id);
  if (it != instances_.end()) {
    instances_.erase(it);
    return true;
  }
  return false;
}

// ── PostgreSQL ───────────────────────────────────────────────────────

AdvancedServerManager::DatabaseStatus AdvancedServerManager::database_status() const {
  return database_status_;
}

bool AdvancedServerManager::set_postgresql_connection(const QString& host, int port, const QString& dbname, const QString& user, const QString& password) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  postgresql_host_ = host;
  postgresql_port_ = port;
  postgresql_dbname_ = dbname;
  postgresql_user_ = user;
  postgresql_password_ = password;
  database_status_ = DatabaseStatus::CONNECTED;
  
  emit database_connected();
  
  if (database_callback_) {
    database_callback_(database_status_);
  }
  
  stats_.database_connections++;
  return true;
}

QString AdvancedServerManager::postgresql_host() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return postgresql_host_;
}

int AdvancedServerManager::postgresql_port() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return postgresql_port_;
}

QString AdvancedServerManager::postgresql_dbname() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return postgresql_dbname_;
}

// ── Load-Balancing ───────────────────────────────────────────────────

AdvancedServerManager::LoadBalanceStrategy AdvancedServerManager::load_balance_strategy() const {
  return load_balance_strategy_;
}

bool AdvancedServerManager::set_load_balance_strategy(LoadBalanceStrategy strategy) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (load_balance_strategy_ != strategy) {
    load_balance_strategy_ = strategy;
    return true;
  }
  return false;
}

std::vector<AdvancedServerManager::ServerNode> AdvancedServerManager::get_server_nodes() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return server_nodes_;
}

bool AdvancedServerManager::add_server_node(const ServerNode& node) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  server_nodes_.push_back(node);
  return true;
}

bool AdvancedServerManager::remove_server_node(const QString& node_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto it = server_nodes_.begin(); it != server_nodes_.end(); ++it) {
    if (it->id == node_id) {
      server_nodes_.erase(it);
      return true;
    }
  }
  return false;
}

AdvancedServerManager::ServerNode* AdvancedServerManager::get_server_node(const QString& node_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& node : server_nodes_) {
    if (node.id == node_id) {
      return &node;
    }
  }
  return nullptr;
}

const AdvancedServerManager::ServerNode* AdvancedServerManager::get_server_node(const QString& node_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (const auto& node : server_nodes_) {
    if (node.id == node_id) {
      return &node;
    }
  }
  return nullptr;
}

QString AdvancedServerManager::select_next_server() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (server_nodes_.empty()) {
    return QString();
  }
  
  QString selected;
  
  switch (load_balance_strategy_) {
    case LoadBalanceStrategy::ROUND_ROBIN:
      selected = server_nodes_[current_node_index_ % server_nodes_.size()].id;
      current_node_index_++;
      break;
      
    case LoadBalanceStrategy::LEAST_CONNECTIONS: {
      int min_connections = std::numeric_limits<int>::max();
      for (const auto& node : server_nodes_) {
        if (node.active_connections < min_connections) {
          min_connections = node.active_connections;
          selected = node.id;
        }
      }
      break;
    }
      
    case LoadBalanceStrategy::WEIGHTED: {
      int total_weight = 0;
      for (const auto& node : server_nodes_) {
        total_weight += node.weight;
      }
      
      int random_value = QRandomGenerator::system()->bounded(total_weight);
      int cumulative = 0;
      for (const auto& node : server_nodes_) {
        cumulative += node.weight;
        if (random_value < cumulative) {
          selected = node.id;
          break;
        }
      }
      break;
    }
      
    case LoadBalanceStrategy::GEOGRAPHIC: {
      // Simuliert: Wähle ersten gesunden Server
      for (const auto& node : server_nodes_) {
        if (node.is_healthy) {
          selected = node.id;
          break;
        }
      }
      break;
    }
  }
  
  stats_.load_balanced_requests++;
  return selected;
}

// ── Geo-Replication ──────────────────────────────────────────────────

AdvancedServerManager::ReplicationStatus AdvancedServerManager::replication_status() const {
  return replication_status_;
}

bool AdvancedServerManager::enable_geo_replication(const QString& remote_host, int remote_port) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  remote_replication_host_ = remote_host;
  remote_replication_port_ = remote_port;
  replication_status_ = ReplicationStatus::ACTIVE;
  
  emit replication_started();
  return true;
}

bool AdvancedServerManager::disable_geo_replication() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  replication_status_ = ReplicationStatus::DISABLED;
  
  emit replication_stopped();
  return true;
}

QString AdvancedServerManager::remote_replication_host() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return remote_replication_host_;
}

int AdvancedServerManager::remote_replication_port() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return remote_replication_port_;
}

qint64 AdvancedServerManager::bytes_replicated() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return bytes_replicated_;
}

// ── Migration ────────────────────────────────────────────────────────

AdvancedServerManager::MigrationStatus AdvancedServerManager::migration_status() const {
  return migration_status_;
}

AdvancedServerManager::MigrationResult AdvancedServerManager::migrate_from_mysql(const QString& mysql_host, int mysql_port, const QString& mysql_db, const QString& mysql_user, const QString& mysql_password) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  MigrationResult result;
  result.success = true;
  result.message = "Migration erfolgreich";
  result.items_migrated = 1000; // Simuliert
  result.items_failed = 0;
  result.duration_seconds = 5.0; // Simuliert
  
  migration_status_ = MigrationStatus::COMPLETED;
  stats_.migrations_completed++;
  
  emit migration_completed(result);
  
  if (migration_callback_) {
    migration_callback_(result);
  }
  
  return result;
}

// ── Konfiguration ────────────────────────────────────────────────────

AdvancedServerManager::ServerConfig AdvancedServerManager::get_config() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return config_;
}

bool AdvancedServerManager::set_config(const ServerConfig& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  config_ = config;
  return true;
}

// ── Statistik ────────────────────────────────────────────────────────

AdvancedServerManager::Stats AdvancedServerManager::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return stats_;
}

void AdvancedServerManager::reset_stats() {
  std::lock_guard<std::mutex> lock(mutex_);
  stats_ = Stats{};
}

// ── Callbacks ────────────────────────────────────────────────────────

void AdvancedServerManager::set_instance_callback(InstanceCallback callback) {
  instance_callback_ = std::move(callback);
}

void AdvancedServerManager::set_database_callback(DatabaseCallback callback) {
  database_callback_ = std::move(callback);
}

void AdvancedServerManager::set_migration_callback(MigrationCallback callback) {
  migration_callback_ = std::move(callback);
}

// ── Slots ────────────────────────────────────────────────────────────

void AdvancedServerManager::on_migration_progress(qint64 items_migrated, qint64 total_items) {
  // Simuliere Migrations-Fortschritt
  Q_UNUSED(items_migrated);
  Q_UNUSED(total_items);
}

// ── Auto-Scaling ─────────────────────────────────────────────────────

AdvancedServerManager::ScalingPolicy AdvancedServerManager::scaling_policy() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return scaling_policy_;
}

bool AdvancedServerManager::set_scaling_policy(ScalingPolicy policy) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (scaling_policy_ != policy) {
    scaling_policy_ = policy;
    return true;
  }
  return false;
}

AdvancedServerManager::ScalingMetrics AdvancedServerManager::get_scaling_metrics() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  // Simuliere Metriken
  ScalingMetrics metrics;
  metrics.cpu_usage_percent = static_cast<double>(QRandomGenerator::system()->bounded(100));
  metrics.memory_usage_percent = static_cast<double>(QRandomGenerator::system()->bounded(100));
  metrics.active_connections = static_cast<int>(QRandomGenerator::system()->bounded(1000));
  metrics.active_instances = static_cast<int>(instances_.size());
  metrics.recommended_instances = metrics.active_instances;
  metrics.network_bandwidth_mbps = static_cast<double>(QRandomGenerator::system()->bounded(1000));
  
  return metrics;
}

bool AdvancedServerManager::scale_instances(int target_count) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  // Simuliere Scaling
  Q_UNUSED(target_count);
  return true;
}

int AdvancedServerManager::current_instance_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<int>(instances_.size());
}

int AdvancedServerManager::recommended_instance_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  // Simuliere Empfehlung
  return static_cast<int>(instances_.size());
}

// ── Backup/Restore ───────────────────────────────────────────────────

AdvancedServerManager::BackupStatus AdvancedServerManager::backup_status() const {
  return backup_status_;
}

bool AdvancedServerManager::create_backup(const QString& path) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  BackupInfo info;
  info.id = QUuid::createUuid().toString().mid(1, 8);
  info.path = path;
  info.size_bytes = 1024 * 1024; // 1 MB simuliert
  info.created_at = QDateTime::currentDateTime();
  info.is_valid = true;
  
  backups_.push_back(info);
  current_backup_id_ = info.id;
  backup_status_ = BackupStatus::COMPLETED;
  
  return true;
}

bool AdvancedServerManager::restore_backup(const QString& path) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  Q_UNUSED(path);
  backup_status_ = BackupStatus::COMPLETED;
  return true;
}

bool AdvancedServerManager::delete_backup(const QString& id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto it = backups_.begin(); it != backups_.end(); ++it) {
    if (it->id == id) {
      backups_.erase(it);
      return true;
    }
  }
  return false;
}

std::vector<AdvancedServerManager::BackupInfo> AdvancedServerManager::get_backups() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return backups_;
}

AdvancedServerManager::BackupInfo* AdvancedServerManager::get_backup(const QString& id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& backup : backups_) {
    if (backup.id == id) {
      return &backup;
    }
  }
  return nullptr;
}

const AdvancedServerManager::BackupInfo* AdvancedServerManager::get_backup(const QString& id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (const auto& backup : backups_) {
    if (backup.id == id) {
      return &backup;
    }
  }
  return nullptr;
}

// ── API-Gateway ──────────────────────────────────────────────────────

bool AdvancedServerManager::add_api_endpoint(const APIEndpointConfig& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  api_endpoints_.push_back(config);
  return true;
}

bool AdvancedServerManager::remove_api_endpoint(const QString& path) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto it = api_endpoints_.begin(); it != api_endpoints_.end(); ++it) {
    if (it->path == path) {
      api_endpoints_.erase(it);
      return true;
    }
  }
  return false;
}

std::vector<AdvancedServerManager::APIEndpointConfig> AdvancedServerManager::get_api_endpoints() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return api_endpoints_;
}

AdvancedServerManager::APIEndpointConfig* AdvancedServerManager::get_api_endpoint(const QString& path) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& endpoint : api_endpoints_) {
    if (endpoint.path == path) {
      return &endpoint;
    }
  }
  return nullptr;
}

const AdvancedServerManager::APIEndpointConfig* AdvancedServerManager::get_api_endpoint(const QString& path) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (const auto& endpoint : api_endpoints_) {
    if (endpoint.path == path) {
      return &endpoint;
    }
  }
  return nullptr;
}

} // namespace hh::server
