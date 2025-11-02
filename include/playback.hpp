#pragma once
#include <string>

std::string getActiveAccess_token();
void togglePause();
std::string getDeviceId(const std::string &accessToken,
                        const std::string &deviceName = "spotcli");
