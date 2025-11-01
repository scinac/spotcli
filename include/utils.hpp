#include "../include/playlist.hpp"
#include "../include/spotcli.hpp"
#include <ios>
#include <limits>

std::string urlEncode(const std::string &value);
std::unordered_map<std::string, std::string> loadEnv(const std::string &path);
const std::string getClientId();
const std::string getRedirect_uri();
const std::string getClientSecret();
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
std::string base64Encode(const std::string &in);
void killAllInstances();
