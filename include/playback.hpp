#pragma once
#include <string>

void skipToPrevSong();
std::string getActiveAccess_token();
void togglePause();
void skipToNextSong();
std::string getDeviceId(const std::string &accessToken,
                        const std::string &deviceName = "spotcli");
