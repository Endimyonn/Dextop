#include "Dexxor.h"
#include "Dextop_Defs.h"


#include <iostream>
#include <curl/curl.h>

using namespace std;
using json = nlohmann::json;



bool authenticated = false;
std::string accessToken;
std::string refreshToken;
std::string tokenCID;
std::string tokenCSecret;
std::chrono::steady_clock::time_point tokenTime;


// copy received data to a string 
size_t WriteCallback(char* argReceivedData, size_t size, size_t nmemb, void* argOutput)
{
	((std::string*)argOutput)->append((char*)argReceivedData, size * nmemb);
	return size * nmemb;
}

Dexxor::Dexxor()
{
	//initialize curl
	curl_global_init(CURL_GLOBAL_ALL);
	std::cout << "Dexxor init done" << std::endl;
}

void Dexxor::Shutdown()
{
	
}

bool Dexxor::Authenticated()
{
	return authenticated;
}

void Dexxor::Authenticate(std::string argUsername, std::string argPassword, std::string argClientID, std::string argClientSecret)
{
	//are we already authenticated?
	if (authenticated == true)
	{
		cout << "Already authenticated!" << endl;
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
		cout << "Authentication request failed: " << curl_easy_strerror(result) << endl;
		return;
	}

	json responseJson = json::parse(readBuffer);

	if (responseJson["access_token"] != nullptr)
	{
		authenticated = true;
		accessToken = responseJson["access_token"];
		refreshToken = responseJson["refresh_token"];
		tokenCID = argClientID;
		tokenCSecret = argClientSecret;
		tokenTime = std::chrono::high_resolution_clock::now();
		cout << "Authentication succeeded." << endl;
	}
	else
	{
		authenticated = false;
		cout << "Authentication failed. Reason: " << responseJson["error"] << endl << responseJson << endl;
	}
}

void Dexxor::RefreshAccessToken()
{
	if (Dexxor::AccessTokenExpired() == false)
	{
		std::cout << "Dexxor: access token asked to refresh, but not yet expired! (age: " << AccessTokenAge() << "s)" << std::endl;
		return;
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
		cout << "Reauthentication request failed: " << curl_easy_strerror(result) << endl;
		return;
	}

	json responseJson = json::parse(readBuffer);

	if (responseJson["access_token"] != nullptr)
	{
		authenticated = true;
		accessToken = responseJson["access_token"];
		tokenTime = std::chrono::high_resolution_clock::now();
		cout << "Reauthentication succeeded." << endl;
	}
	else
	{
		authenticated = false;
		cout << "Reauthentication failed. Reason: " << responseJson["error"] << endl << responseJson << endl;
	}
}

int Dexxor::AccessTokenAge()
{
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - tokenTime).count();
}

bool Dexxor::AccessTokenExpired()
{
	return AccessTokenAge() >= 900;
}

void RefreshToken()
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
		cout << "Token refresh request failed: " << curl_easy_strerror(result) << endl;
		return;
	}

	json responseJson = json::parse(readBuffer);

	if (responseJson["access_token"] != nullptr)
	{
		authenticated = true;
		accessToken = responseJson["access_token"];
		tokenTime = std::chrono::high_resolution_clock::now();
		cout << "Token refresh succeeded." << endl;
	}
	else
	{
		authenticated = false;
		cout << "Authentication failed. Reason: ";
		if (responseJson["error"] == "invalid_client")
		{
			cout << "invalid credentials. Check provided information." << endl;
		}
		else
		{
			cout << responseJson["error"] << endl << responseJson;
		}
	}
}

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
									+ "&includes%5B%5D=author"
									+ "&includes%5B%5D=artist"
									+ "&includes%5B%5D=cover_art";
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		cout << "Search request failed: " << curl_easy_strerror(result) << endl;
	}

	json responseJson = json::parse(readBuffer);

	if (responseJson["error"] != nullptr)
	{
		cout << "Search request failed. Reason: " << responseJson["error"] << endl;
	}

	return responseJson;
}

nlohmann::json Dexxor::GetChapters(std::string mangaID)
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
									+ "limit=50"
									+ "&manga=" + mangaID
									+ "&translatedLanguage%5B%5D=" + "en"
									+ "&contentRating%5B%5D=" + "safe"
									+ "&contentRating%5B%5D=" + "suggestive"
									+ "&contentRating%5B%5D=" + "erotica"
									+ "&contentRating%5B%5D=" + "pornographic"
									+ "&includeFutureUpdates=" + "0"
									+ "&order%5BcreatedAt%5D=asc&order%5BupdatedAt%5D=asc&order%5BpublishAt%5D=asc&order%5BreadableAt%5D=asc&order%5Bvolume%5D=asc&order%5Bchapter%5D=asc";
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		cout << "Search request failed: " << curl_easy_strerror(result) << endl;
	}

	json responseJson = json::parse(readBuffer);
	std::cout << responseJson << std::endl;

	if (responseJson["error"] != nullptr)
	{
		cout << "Search request failed. Reason: " << responseJson["error"] << endl;
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

	//set url
	std::string url = std::string("https://api.mangadex.org/user/follows/manga/feed")
									+ "?limit=" + std::to_string(limit)
									+ "&offset=" + std::to_string(page * limit);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		cout << "Feed request failed: " << curl_easy_strerror(result) << endl;
	}

	json responseJson = json::parse(readBuffer);

	if (responseJson["error"] != nullptr)
	{
		cout << "Feed request failed. Reason: " << responseJson["error"] << endl;
	}

	return responseJson;
}

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
		cout << "Search request failed: " << curl_easy_strerror(result) << endl;
	}

	json responseJson = json::parse(readBuffer);

	if (responseJson["error"] != nullptr)
	{
		cout << "Search request failed. Reason: " << responseJson["error"] << endl;
	}

	return responseJson;
}
