#pragma once

#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

class Dexxor
{
public:
	void Initialize();
	void Shutdown();

	void Authenticate(const char* argUsername, const char* argPassword, const char* argClientID, const char* argClientSecret);
	bool Authenticated();

	nlohmann::json Search(const char* argTitle);

private:
	void PrepareCurl(CURL* argCurl);
};