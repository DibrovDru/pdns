#pragma once

#include <memory>
#include <optional>
#include <string>
#include <iosfwd>

#include "arguments.hh"
#include "namespaces.hh"

#ifndef REDISCPP_HEADER_ONLY
#define REDISCPP_HEADER_ONLY
#endif
#include <redis-cpp/execute.h>
#include <redis-cpp/stream.h>

struct RedisSettings
{
  bool enabled{false};
  std::string host{"127.0.0.1"};
  int port{6379};
  std::string password;
  int database{0};
};

RedisSettings getRedisSettingsFromArgs();

class RedisClient
{
public:
  explicit RedisClient(RedisSettings settings);

  bool enabled() const;
  bool ping();
  bool setNxEx(const std::string& key, const std::string& value, uint32_t ttlSeconds);
  bool set(const std::string& key, const std::string& value, std::optional<uint32_t> ttlSeconds = std::nullopt);
  std::optional<std::string> get(const std::string& key);
  bool del(const std::string& key);

private:
  bool ensureConnected();

  RedisSettings d_settings;
  std::shared_ptr<std::iostream> d_stream;
};
