#include "../include/playlist.hpp"
#include "../include/authSpotify.hpp"
#include "../include/token.hpp"
#include "../include/utils.hpp"

#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <curl/easy.h>
#include <iostream>
#include <json/config.h>
#include <json/json.h>
#include <json/reader.h>
#include <ncurses.h>
#include <sstream>
#include <string>
#include <thread>
#include <time.h>
#include <vector>

void showPlaylists(const std::string &clientId, const std::string &clientSecret,
                   const std::string &redirectUri) {
  Tokens tokens;

  if (loadTokens(tokens)) {
    std::cout << "using locally saved token" << '\n';
    if (isExpired(tokens)) {
      std::cout << "token isExpired, refreshing..." << '\n';
      tokens.accessToken =
          refreshAccessToken(tokens.refreshToken, clientId, clientSecret);
    }
  } else {
    std::cerr << "not logged in" << '\n';
    return;
  }
  std::string access_token = tokens.accessToken;

  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "ERROR: to init curl" << '\n';
    return;
  }

  std::string readBuffer;

  const char *url = "https://api.spotify.com/v1/me/playlists";

  char authHeader[512];
  std::snprintf(authHeader, sizeof(authHeader), "Authorization: Bearer %s",
                access_token.c_str());

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, authHeader);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    std::cerr << "WARNING: Request failed: " << curl_easy_strerror(res) << '\n';
  } else {
    Json::CharReaderBuilder reader;
    Json::Value jsonData;
    Json::String errs;
    std::istringstream s(readBuffer);

    if (Json::parseFromStream(reader, s, &jsonData, &errs)) {
      std::vector<std::string> playlistNames;
      std::vector<std::string> playlistIds;
      for (const auto &item : jsonData["items"]) {
        playlistNames.push_back(item["name"].asString());
        playlistIds.push_back(item["id"].asString());
      }

      int selectedIndex = selectPlaylist(playlistNames);
      if (selectedIndex >= 0 && selectedIndex < (int)playlistIds.size()) {
        playPlaylist(tokens.accessToken, playlistIds[selectedIndex]);
      }

    } else {
      std::cerr << "WARNING: failed to parse JSON: " << errs << '\n';
      return;
    }
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
}
int selectPlaylist(const std::vector<std::string> &playlists) {
  initscr();
  noecho();
  cbreak();
  keypad(stdscr, TRUE);

  int highlight = 0;
  int choice = -1;
  int ch;

  while (true) {
    clear();
    for (size_t i = 0; i < playlists.size(); i++) {
      if ((int)i == highlight)
        attron(A_REVERSE);
      mvprintw(i, 0, "%s", playlists[i].c_str());
      if ((int)i == highlight)
        attroff(A_REVERSE);
    }
    ch = getch();
    switch (ch) {
    case KEY_UP:
      highlight--;
      if (highlight < 0)
        highlight = playlists.size() - 1;
      break;
    case KEY_DOWN:
      highlight++;
      if (highlight >= (int)playlists.size())
        highlight = 0;
      break;
    case 10:
      choice = highlight;
      goto endLoop;
    }
  }

endLoop:
  endwin();
  return choice;
}

void playPlaylist(std::string &accessToken, std::string &playlistId) {
  std::string deviceName = "spotcli";
  std::string cmd = "nohup librespot --quiet --access-token " + accessToken +
                    " -n \"spotcli\" >/dev/null 2>&1 &";
  std::cout << "Launching librespot: " << cmd << std::endl;
  int resLibrespot = std::system(cmd.c_str());
  if (resLibrespot != 0) {
    std::cerr << "ERROR: librespot failed with code " << resLibrespot
              << std::endl;
  }

  std::this_thread::sleep_for(std::chrono::seconds(10));

  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "ERROR: curl init failed\n";
    return;
  }

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

  std::cout << "Device response: " << readBuffer << std::endl;
  if (res != CURLE_OK) {
    std::cerr << "ERROR: failed to get devices: " << curl_easy_strerror(res)
              << std::endl;
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return;
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  std::string deviceId;
  Json::CharReaderBuilder reader;
  Json::Value jsonData;
  std::istringstream s(readBuffer);
  std::string errs;
  if (!Json::parseFromStream(reader, s, &jsonData, &errs)) {
    std::cerr << "ERROR: failed to parse JSON: " << errs << std::endl;
    return;
  }

  for (const auto &device : jsonData["devices"]) {
    if (device["name"].asString() == deviceName) {
      deviceId = device["id"].asString();
      break;
    }
  }

  if (deviceId.empty()) {
    std::cerr << "ERROR: device not found" << std::endl;
    return;
  }
  curl = curl_easy_init();
  if (!curl)
    return;

  std::string url =
      "https://api.spotify.com/v1/me/player/play?device_id=" + deviceId;
  std::string data =
      "{\"context_uri\":\"spotify:playlist:" + playlistId + "\"}";

  headers = nullptr;
  headers = curl_slist_append(headers,
                              ("Authorization: Bearer " + accessToken).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "ERROR: failed to start playlist: " << curl_easy_strerror(res)
              << std::endl;
  } else {
    std::cout << "Playlist started successfully on device " << deviceName
              << std::endl;
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
}
