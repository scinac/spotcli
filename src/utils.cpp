#include "../include/playlist.hpp"
#include "../include/spotcli.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <ios>
#include <limits>
#include <memory>

std::string urlEncode(const std::string &value) {
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (char c : value) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
    } else if (c == ' ') {
      escaped << "%20";
    } else {
      escaped << '%' << std::uppercase << std::setw(2) << int((unsigned char)c)
              << std::nouppercase;
    }
  }

  return escaped.str();
}

std::unordered_map<std::string, std::string> loadEnv(const std::string &path) {
  std::unordered_map<std::string, std::string> env;
  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    auto pos = line.find('=');
    if (pos == std::string::npos)
      continue;
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);
    env[key] = value;
  }
  return env;
}
const std::string getClientId() {
  std::unordered_map<std::string, std::string> env = loadEnv("../.env");

  return env["SPOTIFY_CLIENT_ID"];
}

const std::string getRedirect_uri() {
  std::unordered_map<std::string, std::string> env = loadEnv("../.env");

  return env["REDIRECT_URI"];
}
const std::string getClientSecret() {
  std::unordered_map<std::string, std::string> env = loadEnv("../.env");

  return env["SPOTIFY_CLIENT_SECRET"];
}
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  std::string *s = static_cast<std::string *>(userp);
  size_t total = size * nmemb;
  s->append(static_cast<char *>(contents), total);
  return total;
}

std::string base64Encode(const std::string &in) {
  static const std::string chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  std::string out;
  int val = 0, valb = -6;
  for (unsigned char c : in) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) {
    out.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
  }
  while (out.size() % 4) {
    out.push_back('=');
  }
  return out;
}
void killAllInstances() {
  std::array<char, 128> buffer{};
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(
      popen("pgrep -x librespot", "r"), pclose);

  if (pipe) {
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
    }
    std::system("pkill -x librespot >/dev/null 2>&1");
  }
}
