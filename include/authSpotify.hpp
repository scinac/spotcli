#include <iostream>

bool getAccessToken(const std::string &authCode, const std::string &clientId,
                    const std::string &clientSecret,
                    const std::string &redirectUri);
bool authSpotify();
std::string refreshAccessToken(const std::string &refreshToken,
                               const std::string &clientId,
                               const std::string &clientSecret);
