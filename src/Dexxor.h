#pragma once

#include "nlohmann/json.hpp"

class Dexxor
{
public:
    Dexxor();
	void Shutdown();

	std::string accessToken;
	std::string refreshToken;

	void Authenticate(std::string argUsername, std::string argPassword, std::string argClientID, std::string argClientSecret);
	int AuthTokenAge();
	bool Authenticated();

	nlohmann::json Search(std::string argTitle, unsigned short limit = 10, unsigned short page = 0);
	nlohmann::json GetChapters(std::string mangaID);
	nlohmann::json GetUpdates(unsigned short limit = 10, unsigned short page = 0);

	//assets
	nlohmann::json GetChapterImageMeta(std::string chapterID);
};