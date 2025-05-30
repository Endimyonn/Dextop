/// The Dexxor is responsible for implementing most interactions with the MangaDex API.
/// Asset retrieval is not presently under its responsibilities, and is handled by AssetManager.
#pragma once

#include "nlohmann/json.hpp"

class Dexxor
{
	private:
		void InvalidateAuthState();
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
		nlohmann::json GetManga(std::string mangaID);
		nlohmann::json GetChapters(std::string mangaID, std::string limit = "50", std::string offset = "0");
		nlohmann::json GetChaptersAggregate(std::string mangaID);
		nlohmann::json GetChapter(std::string chapterID);
		nlohmann::json GetUpdates(unsigned short limit = 32, unsigned short page = 0);
		nlohmann::json GetMangaStatistics(std::string mangaID);
		nlohmann::json GetMangaStatistics(std::vector<std::string> mangaIDs);
		nlohmann::json GetBulkReadMarkers(std::vector<std::string> mangaIDs, bool grouped = false);
		nlohmann::json GetReadMarkers(std::string mangaID);

		//assets
		nlohmann::json GetChapterImageMeta(std::string chapterID);
};