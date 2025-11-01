#pragma once
#include <string>

void togglePause(const std::string &deviceId, const std::string &accessToken);
std::string getDeviceId(const std::string &accessToken,
                        const std::string &deviceName = "spotcli");
