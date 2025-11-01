#include <iostream>

bool getAccessToken(const std::string &authCode, const std::string &clientId,
                    const std::string &clientSecret,
                    const std::string &redirectUri);
bool authSpotify();
