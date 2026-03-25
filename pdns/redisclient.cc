#include "redisclient.hh"

#include <cstdint>
#include <stdexcept>
#include <utility>

RedisSettings getRedisSettingsFromArgs()
{
  RedisSettings settings;
  settings.enabled = ::arg().mustDo("redis-enabled");
  settings.host = ::arg()["redis-host"];
  settings.port = ::arg().asNum("redis-port");
  settings.password = ::arg()["redis-password"];
  settings.database = ::arg().asNum("redis-db");
  return settings;
}

RedisClient::RedisClient(RedisSettings settings) :
  d_settings(std::move(settings))
{
}

bool RedisClient::enabled() const
{
  return d_settings.enabled;
}

bool RedisClient::ensureConnected()
{
  if (!d_settings.enabled) {
    return false;
  }

  if (d_stream) {
    return true;
  }

  d_stream = rediscpp::make_stream(d_settings.host, std::to_string(d_settings.port));

  if (!d_settings.password.empty()) {
    (void)rediscpp::execute(*d_stream, "AUTH", d_settings.password).as<std::string>();
  }
  if (d_settings.database != 0) {
    (void)rediscpp::execute(*d_stream, "SELECT", std::to_string(d_settings.database)).as<std::string>();
  }
  return true;
}

bool RedisClient::ping()
{
  if (!ensureConnected()) {
    return false;
  }
  return rediscpp::execute(*d_stream, "PING").as<std::string>() == "PONG";
}

bool RedisClient::setNxEx(const std::string& key, const std::string& value, uint32_t ttlSeconds)
{
  if (!ensureConnected()) {
    return false;
  }

  try {
    const auto response = rediscpp::execute(*d_stream, "SET", key, value, "NX", "EX", std::to_string(ttlSeconds)).as<std::string>();
    return response == "OK";
  }
  catch (const std::logic_error&) {
    // SET ... NX returns null bulk string if key exists.
    return false;
  }
}

bool RedisClient::set(const std::string& key, const std::string& value, std::optional<uint32_t> ttlSeconds)
{
  if (!ensureConnected()) {
    return false;
  }

  std::string response;
  if (ttlSeconds.has_value()) {
    response = rediscpp::execute(*d_stream, "SET", key, value, "EX", std::to_string(*ttlSeconds)).as<std::string>();
  }
  else {
    response = rediscpp::execute(*d_stream, "SET", key, value).as<std::string>();
  }

  return response == "OK";
}

std::optional<std::string> RedisClient::get(const std::string& key)
{
  if (!ensureConnected()) {
    return std::nullopt;
  }

  try {
    return rediscpp::execute(*d_stream, "GET", key).as<std::string>();
  }
  catch (const std::logic_error&) {
    // GET can return null bulk string for missing key.
    return std::nullopt;
  }
}

bool RedisClient::del(const std::string& key)
{
  if (!ensureConnected()) {
    return false;
  }

  return rediscpp::execute(*d_stream, "DEL", key).as<int64_t>() > 0;
}
