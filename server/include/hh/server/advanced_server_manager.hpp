// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

namespace hh::server {

// Erweiterte Server-Features: Multi-Instance, PostgreSQL, Load-Balancing, Migration
class AdvancedServerManager : public QObject {
  Q_OBJECT

public:
  explicit AdvancedServerManager(QObject* parent = nullptr);
  ~AdvancedServerManager() override = default;

  // Nicht kopierbar, bewegbar
  AdvancedServerManager(const AdvancedServerManager&) = delete;
  AdvancedServerManager& operator=(const AdvancedServerManager&) = delete;
  AdvancedServerManager(AdvancedServerManager&&) noexcept;
  AdvancedServerManager& operator=(AdvancedServerManager&&) noexcept;

  // Multi-Instance
  struct InstanceInfo {
    QString id;
    QString name;
    int port;
    int max_users;
    int current_users;
    bool is_running;
    QDateTime started_at;
    QString config_path;
  };

  std::vector<InstanceInfo> get_instances() const;
  InstanceInfo* get_instance(const QString& id);
  const InstanceInfo* get_instance(const QString& id) const;
  bool start_instance(const QString& id);
  bool stop_instance(const QString& id);
  bool restart_instance(const QString& id);
  bool create_instance(const QString& name, int port, int max_users);
  bool delete_instance(const QString& id);

  // PostgreSQL
  enum class DatabaseStatus {
    DISCONNECTED,
    CONNECTED,
    ERROR
  };
  Q_ENUM(DatabaseStatus)

  DatabaseStatus database_status() const;
  bool set_postgresql_connection(const QString& host, int port, const QString& dbname, const QString& user, const QString& password);
  QString postgresql_host() const;
  int postgresql_port() const;
  QString postgresql_dbname() const;

  // Load-Balancing
  enum class LoadBalanceStrategy {
    ROUND_ROBIN,
    LEAST_CONNECTIONS,
    WEIGHTED,
    GEOGRAPHIC
  };
  Q_ENUM(LoadBalanceStrategy)

  LoadBalanceStrategy load_balance_strategy() const;
  bool set_load_balance_strategy(LoadBalanceStrategy strategy);
  
  struct ServerNode {
    QString id;
    QString hostname;
    int port;
    int weight;
    int active_connections;
    bool is_healthy;
    double response_time_ms;
  };

  std::vector<ServerNode> get_server_nodes() const;
  bool add_server_node(const ServerNode& node);
  bool remove_server_node(const QString& node_id);
  ServerNode* get_server_node(const QString& node_id);
  const ServerNode* get_server_node(const QString& node_id) const;
  QString select_next_server();

  // Geo-Replication
  enum class ReplicationStatus {
    DISABLED,
    ACTIVE,
    SYNCING,
    ERROR
  };
  Q_ENUM(ReplicationStatus)

  ReplicationStatus replication_status() const;
  bool enable_geo_replication(const QString& remote_host, int remote_port);
  bool disable_geo_replication();
  QString remote_replication_host() const;
  int remote_replication_port() const;
  qint64 bytes_replicated() const;

  // Migration
  enum class MigrationStatus {
    IDLE,
    RUNNING,
    COMPLETED,
    FAILED
  };
  Q_ENUM(MigrationStatus)

  MigrationStatus migration_status() const;
  struct MigrationResult {
    bool success;
    QString message;
    qint64 items_migrated;
    qint64 items_failed;
    double duration_seconds;
  };
  MigrationResult migrate_from_mysql(const QString& mysql_host, int mysql_port, const QString& mysql_db, const QString& mysql_user, const QString& mysql_password);

  // Konfiguration
  struct ServerConfig {
    int max_connections;
    int voice_packet_size;
    int voice_sample_rate;
    int voice_channels;
    qint64 session_timeout_ms;
    bool enable_logging;
    bool enable_metrics;
    QString log_level;
  };

  ServerConfig get_config() const;
  bool set_config(const ServerConfig& config);

  // Auto-Scaling
  enum class ScalingPolicy {
    FIXED,
    AUTOMATIC,
    MANUAL
  };
  Q_ENUM(ScalingPolicy)

  ScalingPolicy scaling_policy() const;
  bool set_scaling_policy(ScalingPolicy policy);
  
  struct ScalingMetrics {
    double cpu_usage_percent;
    double memory_usage_percent;
    int active_connections;
    int active_instances;
    int recommended_instances;
    qint64 network_bandwidth_mbps;
  };

  ScalingMetrics get_scaling_metrics() const;
  bool scale_instances(int target_count);
  int current_instance_count() const;
  int recommended_instance_count() const;

  // Backup/Restore
  enum class BackupStatus {
    IDLE,
    BACKING_UP,
    COMPLETED,
    FAILED,
    ERROR
  };
  Q_ENUM(BackupStatus)

  BackupStatus backup_status() const;
  struct BackupInfo {
    QString id;
    QString path;
    qint64 size_bytes;
    QDateTime created_at;
    bool is_valid;
  };

  bool create_backup(const QString& path);
  bool restore_backup(const QString& path);
  bool delete_backup(const QString& id);
  std::vector<BackupInfo> get_backups() const;
  BackupInfo* get_backup(const QString& id);
  const BackupInfo* get_backup(const QString& id) const;

  // API-Gateway
  enum class APIEndpoint {
    REST,
    GRAPHQL,
    WebSocket
  };
  Q_ENUM(APIEndpoint)

  struct APIEndpointConfig {
    QString path;
    APIEndpoint type;
    bool is_enabled;
    int rate_limit_per_minute;
    QString auth_method; // none, token, jwt, oauth
  };

  bool add_api_endpoint(const APIEndpointConfig& config);
  bool remove_api_endpoint(const QString& path);
  std::vector<APIEndpointConfig> get_api_endpoints() const;
  APIEndpointConfig* get_api_endpoint(const QString& path);
  const APIEndpointConfig* get_api_endpoint(const QString& path) const;

  // Statistik
  struct Stats {
    size_t instances_started;
    size_t instances_stopped;
    size_t database_connections;
    size_t load_balanced_requests;
    size_t replication_bytes;
    size_t migrations_completed;
    qint64 total_uptime_ms;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using InstanceCallback = std::function<void(const QString& id, bool started)>;
  using DatabaseCallback = std::function<void(DatabaseStatus status)>;
  using MigrationCallback = std::function<void(const MigrationResult& result)>;

  void set_instance_callback(InstanceCallback callback);
  void set_database_callback(DatabaseCallback callback);
  void set_migration_callback(MigrationCallback callback);

signals:
  void instance_started(const QString& id);
  void instance_stopped(const QString& id);
  void database_connected();
  void database_disconnected();
  void migration_completed(const MigrationResult& result);
  void replication_started();
  void replication_stopped();

private slots:
  void on_migration_progress(qint64 items_migrated, qint64 total_items);

private:
  // Multi-Instance
  std::map<QString, InstanceInfo> instances_;
  QString current_instance_id_;
  
  // PostgreSQL
  DatabaseStatus database_status_ = DatabaseStatus::DISCONNECTED;
  QString postgresql_host_;
  int postgresql_port_ = 5432;
  QString postgresql_dbname_;
  QString postgresql_user_;
  QString postgresql_password_;
  
  // Load-Balancing
  LoadBalanceStrategy load_balance_strategy_ = LoadBalanceStrategy::ROUND_ROBIN;
  std::vector<ServerNode> server_nodes_;
  size_t current_node_index_;
  
  // Geo-Replication
  ReplicationStatus replication_status_ = ReplicationStatus::DISABLED;
  QString remote_replication_host_;
  int remote_replication_port_ = 5432;
  qint64 bytes_replicated_ = 0;
  
  // Migration
  MigrationStatus migration_status_ = MigrationStatus::IDLE;
  
  // Konfiguration
  ServerConfig config_;
  
  // Auto-Scaling
  ScalingPolicy scaling_policy_ = ScalingPolicy::FIXED;
  ScalingMetrics scaling_metrics_;
  
  // Backup/Restore
  BackupStatus backup_status_ = BackupStatus::IDLE;
  std::vector<BackupInfo> backups_;
  QString current_backup_id_;
  
  // API-Gateway
  std::vector<APIEndpointConfig> api_endpoints_;
  
  // Statistik
  Stats stats_;
  
  InstanceCallback instance_callback_;
  DatabaseCallback database_callback_;
  MigrationCallback migration_callback_;
  
  mutable std::mutex mutex_;
};

} // namespace hh::server
