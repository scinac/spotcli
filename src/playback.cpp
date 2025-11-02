#include "../include/playback.hpp"
#include "../include/authSpotify.hpp"
#include "../include/token.hpp"
#include "../include/utils.hpp"
#include <curl/curl.h>
#include <iostream>
#include <json/json.h>
#include <sstream>

std::string getActiveAccess_token() {
  Tokens tokens;
  loadTokens(tokens);

  if (isExpired(tokens)) {
    tokens.accessToken = refreshAccessToken(tokens.refreshToken, getClientId(),
                                            getClientSecret());
  }

  return tokens.accessToken;
}

static void spotifyPutRequest(const std::string &accessToken,
                              const std::string &url) {

  CURL *curl = curl_easy_init();
  if (!curl)
    return;

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers,
                              ("Authorization: Bearer " + accessToken).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "Spotify request failed: " << curl_easy_strerror(res) << "\n";
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
}

static bool isPlaying(const std::string &accessToken,
                      const std::string &deviceId) {

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  std::string readBuffer;
  std::string url =
      "https://api.spotify.com/v1/me/player?device_id=" + deviceId;

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers,
                              ("Authorization: Bearer " + accessToken).c_str());

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    std::cerr << "Failed to get playback state: " << curl_easy_strerror(res)
              << "\n";
    return false;
  }

  Json::CharReaderBuilder reader;
  Json::Value jsonData;
  std::istringstream s(readBuffer);
  std::string errs;
  if (!Json::parseFromStream(reader, s, &jsonData, &errs)) {
    std::cerr << "Failed to parse JSON: " << errs << "\n";
    return false;
  }

  if (jsonData.isMember("is_playing") && jsonData["is_playing"].isBool()) {
    return jsonData["is_playing"].asBool();
  }

  return false;
}

void togglePause() {
  std::string accessToken = getActiveAccess_token();
  const std::string deviceId = getDeviceId(accessToken);

  if (isPlaying(accessToken, deviceId)) {
    std::string url =
        "https://api.spotify.com/v1/me/player/pause?device_id=" + deviceId;
    spotifyPutRequest(accessToken, url);
    // std::cout << "Paused playback\n";
  } else {
    std::string url =
        "https://api.spotify.com/v1/me/player/play?device_id=" + deviceId;
    spotifyPutRequest(accessToken, url);
    // std::cout << "Resumed playback\n";
  }
}

std::string getDeviceId(const std::string &accessToken,
                        const std::string &deviceName) {

  CURL *curl = curl_easy_init();
  if (!curl)
    return "";

  std::string readBuffer;
  curl_easy_setopt(curl, CURLOPT_URL,
                   "https://api.spotify.com/v1/me/player/devices");
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers,
                              ("Authorization: Bearer " + accessToken).c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  // std::cout << accessToken << readBuffer << '\n';

  if (res != CURLE_OK) {
    std::cerr << "Failed to get devices: " << curl_easy_strerror(res) << "\n";
    return "";
  }

  Json::CharReaderBuilder reader;
  Json::Value jsonData;
  std::istringstream s(readBuffer);
  std::string errs;

  if (!Json::parseFromStream(reader, s, &jsonData, &errs)) {
    std::cerr << "Failed to parse JSON: " << errs << "\n";
    return "";
  }

  // std::cout << readBuffer << '\n';

  for (const auto &device : jsonData["devices"]) {
    if (device["name"].asString() == deviceName) {
      return device["id"].asString();
    }
  }

  std::cerr << "Device not found: " << deviceName << "\n";
  return "";
}
