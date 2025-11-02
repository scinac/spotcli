#include <ctime>
#include <fstream>
#include <iostream>
#include <json/json.h>
#include <string>

struct Tokens {
  std::string accessToken;
  std::string refreshToken;
  time_t expiresAt;
};

void saveTokens(const Tokens &tokens, const std::string &file = "tokens.json") {
  Json::Value root;
  std::cout << "new: " << tokens.accessToken << '\n';
  root["access_token"] = tokens.accessToken;
  root["refresh_token"] = tokens.refreshToken;
  root["expires_at"] = static_cast<Json::Int64>(tokens.expiresAt);

  std::ofstream fileOut(file);
  fileOut << root;
}

bool loadTokens(Tokens &tokens, const std::string &file = "tokens.json") {
  std::ifstream fileIn(file);
  if (!fileIn.is_open())
    return false;

  Json::Value root;
  fileIn >> root;

  tokens.accessToken = root.get("access_token", "").asString();
  tokens.refreshToken = root.get("refresh_token", "").asString();
  tokens.expiresAt = root.get("expires_at", 0).asInt64();

  return !tokens.accessToken.empty();
}

bool isExpired(const Tokens &tokens) {
  return std::time(nullptr) >= tokens.expiresAt;
}
