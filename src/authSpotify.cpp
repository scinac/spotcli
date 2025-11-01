#include "../include/token.hpp"
#include "../include/utils.hpp"
#include <curl/curl.h>
#include <curl/easy.h>
#include <iostream>
#include <json/config.h>
#include <json/json.h>
#include <json/reader.h>
#include <string>

bool getAccessToken(const std::string &authCode, const std::string &clientId,
                    const std::string &clientSecret,
                    const std::string &redirectUri) {

  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "ERROR: curl init failed\n";
    return false;
  }

  std::string readBuffer;

  std::string postFields = "grant_type=authorization_code&code=" + authCode +
                           "&redirect_uri=" + redirectUri;

  std::string authStr = clientId + ":" + clientSecret;

  std::string encodedAuth = "Authorization: Basic " + base64Encode(authStr);

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, encodedAuth.c_str());
  headers = curl_slist_append(
      headers, "content-Type: application/x-www-form-urlencoded");

  curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.spotify.com/api/token");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    std::cerr << "ERROR: curl request failed: " << curl_easy_strerror(res)
              << "\n";
    return false;
  }

  // std::cout << "token response: " << readBuffer << "\n";
  Json::CharReaderBuilder reader;
  Json::Value jsonData;
  std::istringstream s(readBuffer);
  std::string errs;

  if (!Json::parseFromStream(reader, s, &jsonData, &errs)) {
    std::cerr << "ERROR: Failed to parse JSON: " << errs << "\n";
    return false;
  }

  if (!jsonData["access_token"].isString()) {
    std::cerr << "ERROR: access_token missing in response\n";
    return false;
  }

  std::string access_token = jsonData["access_token"].asString();
  std::string refreshToken;
  if (jsonData.isMember("refresh_token") &&
      jsonData["refresh_token"].isString()) {
    refreshToken = jsonData["refresh_token"].asString();
  } else {
    std::cerr << "didnt get refresh token from request" << '\n';
  }

  Tokens tokens;

  tokens.accessToken = access_token;
  tokens.refreshToken = refreshToken;
  tokens.expiresAt = std::time(nullptr) + 3600;

  return true;
}

bool authSpotify() {
  std::cout << "Copy the code under 'code', close the browser and parse it, "
               "Enter to confirm "
            << '\n';

  const std::string client_id = getClientId();
  const std::string redirect_uri = getRedirect_uri();
  const std::string scope =
      "user-library-read playlist-read-private streaming "
      "user-read-playback-state user-modify-playback-state";

  std::ostringstream url;
  url << "https://accounts.spotify.com/authorize?"
      << "client_id=" << urlEncode(client_id) << "&response_type=code"
      << "&redirect_uri=" << urlEncode(redirect_uri)
      << "&scope=" << urlEncode(scope);

  const std::string urlStr = url.str();
  std::cout << "Opening: " << urlStr << '\n';

#if defined(_WIN32) || defined(_WIN64)
  system(("start \"\" \"" + urlStr + "\"").c_str());
#elif defined(__APPLE__)
  system(("open \"" + urlStr + "\"").c_str());
#else
  system(("xdg-open \"" + urlStr + "\" 2>/dev/null").c_str());
#endif

  std::string authCode;
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::getline(std::cin, authCode);

  if (!getAccessToken(authCode, client_id, getClientSecret(), redirect_uri)) {
    std::cerr << "something failed while getting the access token" << '\n';
    return false;
  }

  std::cout << "authorized successfully" << '\n';
  return true;
}

std::string refreshAccessToken(const std::string &refreshToken,
                               const std::string &clientId,
                               const std::string &clientSecret) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "ERROR: curl init failed\n";
    return "";
  }

  std::string readBuffer;
  std::string postFields =
      "grant_type=refresh_token&refresh_token=" + refreshToken;
  std::string authStr = clientId + ":" + clientSecret;
  std::string encodedAuth = "Authorization: Basic " + base64Encode(authStr);

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, encodedAuth.c_str());
  headers = curl_slist_append(
      headers, "Content-Type: application/x-www-form-urlencoded");

  curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.spotify.com/api/token");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    std::cerr << "ERROR: curl request failed: " << curl_easy_strerror(res)
              << "\n";
    return "";
  }

  Json::CharReaderBuilder reader;
  Json::Value jsonData;
  std::istringstream s(readBuffer);
  std::string errs;

  if (!Json::parseFromStream(reader, s, &jsonData, &errs)) {
    std::cerr << "ERROR: Failed to parse JSON: " << errs << "\n";
    return "";
  }

  if (!jsonData["access_token"].isString()) {
    std::cerr << "ERROR: access_token missing in refresh response\n";
    return "";
  }

  std::string newAccessToken = jsonData["access_token"].asString();

  // Spotify sometimes gives a new refresh_token — update if so
  if (jsonData.isMember("refresh_token")) {
    std::string newRefresh = jsonData["refresh_token"].asString();
    Tokens tokens;
    tokens.accessToken = newAccessToken;
    tokens.refreshToken = newRefresh;
    tokens.expiresAt = std::time(nullptr) + 3600;
    saveTokens(tokens);
  }

  return newAccessToken;
}
