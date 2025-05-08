#include "Dexxor.h"
#include "Dextop.h"
#include "DextopUtil.h"
#include "Logger.h"

#include <iostream>
#include <chrono>
#include <curl/curl.h>




bool authenticated = false;
std::string accessToken;
std::string refreshToken;
long long tokenTime;
std::string tokenCID;
std::string tokenCSecret;


// copy received data to a string 
size_t WriteCallback(char* argReceivedData, size_t size, size_t nmemb, void* argOutput)
{
	((std::string*)argOutput)->append((char*)argReceivedData, size * nmemb);
	return size * nmemb;
}

void Dexxor::Initialize()
{
	//initialize curl
	curl_global_init(CURL_GLOBAL_ALL);
	dtlog << "Dexxor: curl initialized" << std::endl;

	//pull saved auth info from settings
	Dextop* dextop = Dextop::GetInstance();
	accessToken = dextop->settings["authLastAccessToken"].get<std::string>();
	refreshToken = dextop->settings["authLastRefreshToken"].get<std::string>();
	tokenTime = dextop->settings["authLastSuccessTime"].get<long long>();
	tokenCID = dextop->settings["authLastSuccessCID"];
	tokenCSecret = dextop->settings["authLastSuccessCSecret"];

	//attempt to reauthenticate if logged in previously
	if (refreshToken != "")
	{
		authenticated = true;
		if (AccessTokenExpired())
		{
			dtlog << "Stored auth token is expired (age: " << AccessTokenAge() << "). Refreshing login..." << std::endl;
			RefreshAccessToken();
		}
	}

	if (authenticated == true)
	{
		dtlog << "Authenticated: yes (token age: " << AccessTokenAge() << ")" << std::endl;
	}
	else
	{
		dtlog << "Authenticated: no" << std::endl;
	}
	
	dtlog << "Dexxor init done" << std::endl;
}

void Dexxor::Shutdown()
{
	
}



//-----------------------------------------------------------//
//                    Authentication                         //
//-----------------------------------------------------------//

bool Dexxor::Authenticated()
{
	return authenticated;
}

void Dexxor::Authenticate(std::string argUsername, std::string argPassword, std::string argClientID, std::string argClientSecret)
{
	//are we already authenticated?
	if (authenticated == true)
	{
		dtlog << "Already authenticated!" << std::endl;
		return;
	}
	
	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	curl_easy_setopt(curl, CURLOPT_URL, "https://auth.mangadex.org/realms/mangadex/protocol/openid-connect/token");

	//set data
	std::string fields = std::string("username=") + argUsername
		+ "&password=" + argPassword
		+ "&client_id=" + argClientID
		+ "&client_secret=" + argClientSecret
		+ "&grant_type=" + "password";
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Authentication request failed: " << curl_easy_strerror(result) << std::endl;
		return;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson.contains("access_token"))
	{
		authenticated = true;
		accessToken = responseJson["access_token"];
		refreshToken = responseJson["refresh_token"];
		tokenTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now().time_since_epoch()).count();
		tokenCID = argClientID;
		tokenCSecret = argClientSecret;
		Dextop::GetInstance()->settings["authLastAccessToken"] = accessToken;
		Dextop::GetInstance()->settings["authLastRefreshToken"] = refreshToken;
		Dextop::GetInstance()->settings["authLastSuccessTime"] = tokenTime;
		Dextop::GetInstance()->settings["authLastSuccessCID"] = tokenCID;
		Dextop::GetInstance()->settings["authLastSuccessCSecret"] = tokenCSecret;
		dtlog << "Authentication succeeded." << std::endl;
	}
	else
	{
		authenticated = false;
		dtlog << "Authentication failed. Reason: " << responseJson["error"] << std::endl << responseJson << std::endl;
	}
}

void Dexxor::RefreshAccessToken()
{
	if (Dexxor::AccessTokenExpired() == false)
	{
		dtlog << "Dexxor: access token asked to refresh, but not yet expired! (age: " << AccessTokenAge() << "s)" << std::endl;
		return;
	}
	else if (authenticated == false)
	{
		dtlog << "Dexxor: cannot refresh access token because we're not authenticated to begin with" << std::endl;
	}

	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	curl_easy_setopt(curl, CURLOPT_URL, "https://auth.mangadex.org/realms/mangadex/protocol/openid-connect/token");

	//set data
	std::string fields = std::string("grant_type=refresh_token")
		+ "&refresh_token=" + refreshToken
		+ "&client_id=" + tokenCID
		+ "&client_secret=" + tokenCSecret;
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		InvalidateAuthState();
		dtlog << "Reauthentication request failed: " << curl_easy_strerror(result) << std::endl;
		return;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson.contains("access_token"))
	{
		authenticated = true;
		accessToken = responseJson["access_token"];
		tokenTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now().time_since_epoch()).count();
		Dextop::GetInstance()->settings["authLastAccessToken"] = accessToken;
		Dextop::GetInstance()->settings["authLastSuccessTime"] = tokenTime;
		dtlog << "Reauthentication succeeded." << std::endl;
	}
	else
	{
		InvalidateAuthState();
		dtlog << "Reauthentication failed. Reason: " << responseJson["error"] << std::endl << responseJson << std::endl;
	}
}

int Dexxor::AccessTokenAge()
{
	long long nowTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now().time_since_epoch()).count();
	return nowTime - tokenTime;
}

bool Dexxor::AccessTokenExpired()
{
	if (tokenTime != 0)
	{
		return AccessTokenAge() >= 899;
	}
	return true;
}

void Dexxor::InvalidateAuthState()
{
	authenticated = false;
	accessToken = "";
	refreshToken = "";
	tokenTime = 0;
	Dextop::GetInstance()->settings["authLastAccessToken"] = accessToken;
	Dextop::GetInstance()->settings["authLastRefreshToken"] = refreshToken;
	Dextop::GetInstance()->settings["authLastSuccessTime"] = tokenTime;
}



//-----------------------------------------------------------//
//                        General                            //
//-----------------------------------------------------------//

nlohmann::json Dexxor::Search(std::string argTitle, unsigned short limit, unsigned short page)
{
	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	std::string url = std::string("https://api.mangadex.org/manga")
									+ "?title=" + argTitle
									+ "&limit=" + std::to_string(limit)
									+ "&offset=" + std::to_string(page * limit)
									+ "&includes[]=author"
									+ "&includes[]=artist"
									+ "&includes[]=cover_art";
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Search request failed: " << curl_easy_strerror(result) << std::endl;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson.contains("error"))
	{
		dtlog << "Search request failed. Reason: " << responseJson["error"] << std::endl;
	}

	return responseJson;
}

nlohmann::json Dexxor::GetManga(std::string mangaID)
{
	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	std::string url = std::string("https://api.mangadex.org/manga/") + mangaID
									+ "?includes[]=author"
									+ "&includes[]=artist"
									+ "&includes[]=cover_art";
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Manga request failed (ID: " << mangaID << "): " << curl_easy_strerror(result) << std::endl;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson.contains("error"))
	{
		dtlog << "Manga request failed. Reason: " << responseJson["error"] << std::endl;
	}

	return responseJson["data"];
}

nlohmann::json Dexxor::GetChapters(std::string mangaID, std::string limit, std::string offset)
{
	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	std::string url = std::string("https://api.mangadex.org/chapter?") +
									+ "limit=" + limit
									+ "&offset=" + offset
									+ "&manga=" + mangaID
									+ "&translatedLanguage[]=" + "en"
									+ "&contentRating[]=" + "safe"
									+ "&contentRating[]=" + "suggestive"
									+ "&contentRating[]=" + "erotica"
									+ "&contentRating[]=" + "pornographic"
									+ "&includeFutureUpdates=" + "0"
									+ "&order[createdAt]=asc&order[updatedAt]=asc&order[publishAt]=asc&order[readableAt]=asc&order[volume]=asc&order[chapter]=asc"
									+ "&includes[]=manga"
									+ "&includes[]=scanlation_group"
									+ "&includes[]=user";
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Chapters request failed: " << curl_easy_strerror(result) << std::endl;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson.contains("error"))
	{
		dtlog << "Chapters request failed. Reason: " << responseJson["error"] << std::endl;
	}

	return responseJson;
}

nlohmann::json Dexxor::GetChaptersAggregate(std::string mangaID)
{
	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	std::string url = std::string("https://api.mangadex.org/manga/") + mangaID + "/aggregate"
									+ "?translatedLanguage[]=" + "en";
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Chapters (aggregate) request failed: " << curl_easy_strerror(result) << std::endl;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	return responseJson["volumes"];
}

nlohmann::json Dexxor::GetChapter(std::string chapterID)
{
	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	std::string url = std::string("https://api.mangadex.org/chapter?") +
									+ "limit=1"
									+ "&ids[]=" + chapterID
									+ "&translatedLanguage[]=" + "en"
									+ "&contentRating[]=" + "safe"
									+ "&contentRating[]=" + "suggestive"
									+ "&contentRating[]=" + "erotica"
									+ "&contentRating[]=" + "pornographic"
									+ "&order[createdAt]=asc&order[updatedAt]=asc&order[publishAt]=asc&order[readableAt]=asc&order[volume]=asc&order[chapter]=asc"
									+ "&includes[]=manga"
									+ "&includes[]=scanlation_group"
									+ "&includes[]=user";
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Chapter request failed: " << curl_easy_strerror(result) << std::endl;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson.contains("error"))
	{
		dtlog << "Chapter request failed. Reason: " << responseJson["error"] << std::endl;
	}

	return responseJson;
}

nlohmann::json Dexxor::GetUpdates(unsigned short limit, unsigned short page)
{
	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	struct curl_slist* headers = curl_slist_append(NULL, (std::string("Authorization: Bearer ") + accessToken).c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	//set url
	std::string url = std::string("https://api.mangadex.org/user/follows/manga/feed")
									+ "?limit=" + std::to_string(limit)
									+ "&offset=" + std::to_string(page * limit)
									+ "&translatedLanguage[]=" + "en"
									+ "&contentRating[]=" + "safe"
									+ "&contentRating[]=" + "suggestive"
									+ "&contentRating[]=" + "erotica"
									+ "&contentRating[]=" + "pornographic"
									+ "&includeFutureUpdates=" + "0"
									+ "&order[createdAt]=desc&order[updatedAt]=desc&order[publishAt]=desc&order[readableAt]=desc&order[volume]=desc&order[chapter]=desc"
									+ "&includes[]=manga"
									+ "&includes[]=scanlation_group"
									+ "&includes[]=user";
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Updates request failed: " << curl_easy_strerror(result) << std::endl;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson.contains("error"))
	{
		dtlog << "Updates request failed. Reason: " << responseJson["error"] << std::endl;
	}

	return responseJson;
}

nlohmann::json Dexxor::GetMangaStatistics(std::string mangaID)
{
	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	std::string url = std::string("https://api.mangadex.org/statistics/manga/") + mangaID;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Manga statistics request (ID: " << mangaID << ") failed: " << curl_easy_strerror(result) << std::endl;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson["result"].get<std::string>() == "error")
	{
		dtlog << "Manga statistics request failed. Response: " << std::endl << responseJson.dump(4) << std::endl;
	}

	return responseJson["statistics"][mangaID];
}

nlohmann::json Dexxor::GetMangaStatistics(std::vector<std::string> mangaIDs)
{
	if (mangaIDs.size() == 0)
	{
		dtlog << "Dexxor::GetMangaStatistics (multiple): ID set is empty!" << std::endl;
		return nlohmann::json();
	}

	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	std::string url = std::string("https://api.mangadex.org/statistics/manga?manga[]=") + mangaIDs[0];
	for (int i = 1; i < mangaIDs.size(); i++)
	{
		url = url + "&manga[]=" + mangaIDs[i];
	}
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Manga statistics (multiple) request failed: " << curl_easy_strerror(result) << std::endl;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson["result"].get<std::string>() == "error")
	{
		dtlog << "Manga statistics (multiple) request failed. Response: " << std::endl << responseJson.dump(4) << std::endl;
	}

	return responseJson["statistics"];
}

nlohmann::json Dexxor::GetBulkReadMarkers(std::vector<std::string> mangaIDs, bool grouped)
{
	if (mangaIDs.size() == 0)
	{
		dtlog << "Dexxor::GetBulkReadMarkers: ID set is empty!" << std::endl;
		return nlohmann::json();
	}

	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	struct curl_slist* headers = curl_slist_append(NULL, (std::string("Authorization: Bearer ") + accessToken).c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	//set url
	std::string url = std::string("https://api.mangadex.org/manga/read?ids[]=") + mangaIDs[0];
	for (int i = 1; i < mangaIDs.size(); i++)
	{
		url = url + "&ids[]=" + mangaIDs[i];
	}
	url = url + "&grouped=" + (grouped == true ? "true" : "false");
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "GetBulkReadMarkers request failed: " << curl_easy_strerror(result) << std::endl;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson["result"].get<std::string>() != "ok")
	{
		dtlog << "GetBulkReadMarkers request failed. Response: " << std::endl << responseJson.dump(4) << std::endl;
	}

	return responseJson["data"];
}

nlohmann::json Dexxor::GetReadMarkers(std::string mangaID)
{
	return GetBulkReadMarkers(
		std::vector<std::string>{mangaID},
		false
	);
}



//-----------------------------------------------------------//
//                        Assets                             //
//-----------------------------------------------------------//

nlohmann::json Dexxor::GetChapterImageMeta(std::string chapterID)
{
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	std::string url = std::string("https://api.mangadex.org/at-home/server/") + chapterID;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Search request failed: " << curl_easy_strerror(result) << std::endl;
	}

	nlohmann::json responseJson = nlohmann::json::parse(readBuffer);

	if (responseJson.contains("error"))
	{
		dtlog << "Search request failed. Reason: " << responseJson["error"] << std::endl;
	}

	return responseJson;
}
