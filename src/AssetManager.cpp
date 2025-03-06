#include "AssetManager.h"
#include "Dextop_Defs.h"
#include "Logger.h"

#include "stdio.h"
#include <iostream>
#include <filesystem>
#include <curl/curl.h>
#include <windows.h>



std::string assetsRoot = "assets/";
std::string imgPath = "images/";



AssetManager::AssetManager()
{
    //ensure asset directory skeleton exists
    std::string dirs[4] =
    {
        "assets",
        "assets/images",
        "assets/images/pages",
        "assets/images/covers"
    };
    for (int i = 0; i < 4; i++)
    {
        if (std::filesystem::create_directory(dirs[i]) == true)
        {
            dtlog << "Created ddirectory \"" << dirs[i] << "\"" << std::endl;
        }
    }

    dtlog << "AssetManager initialized" << std::endl;
}

AssetManager::~AssetManager()
{
    dtlog << "AssetManager shut down" << std::endl;
}

size_t WriteAsset(void* ptr, size_t size, size_t nmemb, void* outFile)
{
    return fwrite(ptr, size, nmemb, (FILE*)outFile);
}

void AssetManager::GetImage(std::string url, std::string path)
{
    if (std::filesystem::exists((assetsRoot + path).c_str()))
    {
        dtlog << "File " << path << " exists already" << std::endl;
        return;
    }
    FILE *outFile = fopen((assetsRoot + path).c_str(), "wb");
    CURL* curl = curl_easy_init();
	CURLcode result;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteAsset);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outFile);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, Dextop_UserAgent);

    result = curl_easy_perform(curl);
	if (result != CURLE_OK)
	{
		dtlog << "Request failed: " << curl_easy_strerror(result) << std::endl;
	}
    curl_easy_cleanup(curl);
    fclose(outFile);
}

void AssetManager::GetChapterPage(std::string url, std::string fileName)
{
    GetImage(url, std::string("images/pages/") + fileName);
}

void AssetManager::GetMangaCover(std::string url, std::string fileName)
{
    GetImage(url, std::string("images/covers/") + fileName);
}

void AssetManager::ImageLoad(std::optional<SearchResult>* result, std::string path)
{
    
}