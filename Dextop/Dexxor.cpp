#include "Dexxor.h"

using namespace std;
using json = nlohmann::json;



bool authenticated = false;
std::string accessToken;
std::string refreshToken;


size_t WriteCallback(char* argReceivedData, size_t size, size_t nmemb, void* argOutput)
{
	((std::string*)argOutput)->append((char*)argReceivedData, size * nmemb);
	return size * nmemb;
}

void Dexxor::Initialize()
{
	//initialize curl
	curl_global_init(CURL_GLOBAL_ALL);
}

void Dexxor::Shutdown()
{
	
}

void Dexxor::PrepareCurl(CURL* argCurl)
{
	
}

bool Dexxor::Authenticated()
{
	return authenticated;
}

void Dexxor::Authenticate(const char* argUsername, const char* argPassword, const char* argClientID, const char* argClientSecret)
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
	std::string readBuffer;
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	PrepareCurl(&curl);

	//set url
	curl_easy_setopt(curl, CURLOPT_URL, "https://auth.mangadex.org/realms/mangadex/protocol/openid-connect/token");

	//set data
	std::string fields = std::string("grant_type=password&username=") + argUsername
		+ "&password=" + argPassword
		+ "&client_id=" + argClientID
		+ "&client_secret=" + argClientSecret;
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &fields);

	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		cout << "Authentication request failed: " << curl_easy_strerror(result) << endl;
	}

	json responseJson = json::parse(readBuffer);

	if (responseJson["access_token"] != nullptr)
	{
		authenticated = true;
		accessToken = responseJson["access_token"];
		refreshToken = responseJson["refresh_token"];
		cout << "Authentication succeeded." << endl;
	}
	else
	{
		cout << "Authentication failed. Reason: ";
		if (responseJson["error"] == "invalid_client")
		{
			cout << "invalid credentials. Check provided information." << endl;
		}
	}
}

nlohmann::json Dexxor::Search(const char* argTitle)
{
	//prepare
	CURL* curl = curl_easy_init();
	CURLcode result;
	std::string readBuffer = "";
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Dextop/1.0");
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	//set url
	std::string url = std::string("https://api.mangadex.org/manga?title=") + argTitle;
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
	else
	{
		//cout << "Search response: " << responseJson << endl;
	}

	return responseJson;
}


