/// The Dexxor is responsible for implementing most interactions with the MangaDex API.
/// Asset retrieval is not presently under its responsibilities, and is handled by AssetManager.
#pragma once

#include "nlohmann/json.hpp"

class Dexxor
{
public:
    void Initialize();
	void Shutdown();

	std::string accessToken;
	std::string refreshToken;

	void Authenticate(std::string argUsername, std::string argPassword, std::string argClientID, std::string argClientSecret);
	void RefreshAccessToken();
	bool Authenticated();
	int AccessTokenAge();
	bool AccessTokenExpired();

	nlohmann::json Search(std::string argTitle, unsigned short limit = 10, unsigned short page = 0);
	nlohmann::json GetChapters(std::string mangaID, std::string limit = "50", std::string offset = "0");
	nlohmann::json GetUpdates(unsigned short limit = 10, unsigned short page = 0);

	//assets
	nlohmann::json GetChapterImageMeta(std::string chapterID);
};