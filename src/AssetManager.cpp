#include "AssetManager.h"
#include "DextopUtil.h"
#include "Logger.h"

#include "stdio.h"
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <thread>
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

/// @brief Locks an asset path for reading/writing while being downloaded
/// @param path The local path of the asset
/// @return True if it was locked successfully, false if it's been locked elsewhere
bool AssetManager::AddAssetFetchLock(std::string path)
{
    if (std::find(assetFetchLocks.begin(), assetFetchLocks.end(), path) == assetFetchLocks.end())
    {
        assetFetchLocks.push_back(path);
        dtlog << "assetFetchLocks +" << path << std::endl;
        return true;
    }
    return false;
}

void AssetManager::RemoveAssetFetchLock(std::string path)
{
    std::vector<std::string>::iterator getElement = std::find(assetFetchLocks.begin(), assetFetchLocks.end(), path);
    if (getElement != assetFetchLocks.end())
    {
        assetFetchLocks.erase(getElement);
    }
    dtlog << "assetFetchLocks -" << path << std::endl;
}

/// @brief Checks if an asset path is locked
/// @param path The local path of the asset
/// @return True if it's currently locked, otherwise false
bool AssetManager::AssetFetchLocked(std::string path)
{
    return std::find(assetFetchLocks.begin(), assetFetchLocks.end(), path) != assetFetchLocks.end();
}

void AssetManager::GetImage(std::string url, std::string path)
{
    if (std::filesystem::exists((assetsRoot + path).c_str()))
    {
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
    fclose(outFile);
	if (result != CURLE_OK)
	{
		dtlog << "Request failed: " << curl_easy_strerror(result) << std::endl;
	}
    curl_easy_cleanup(curl);
}

void AssetManager::GetChapterPage(std::string url, std::string chapterID, std::string fileName)
{
    std::filesystem::create_directory(std::string("assets/images/pages/") + chapterID); //make sure the chapter's directory exists
    GetImage(url, std::string("images/pages/") + chapterID + "/" + fileName);
}

void AssetManager::GetMangaCover(std::string url, std::string fileName)
{
    GetImage(url, std::string("images/covers/") + fileName);
}

//Image Load When Ready: wait for an image to be saved locally before loading it
slint::Image AssetManager::ImageLoadWR(std::string path)
{
    while (!std::filesystem::exists((assetsRoot + path).c_str()))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    while (std::filesystem::file_size((assetsRoot + path).c_str()) == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    while (AssetFetchLocked(path) == true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    slint::Image loadImage = slint::Image::load_from_path((assetsRoot + path).c_str());

    //check if the file loaded correctly, and retry after a bit if it didn't
    if (loadImage.size().width == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return slint::Image::load_from_path((assetsRoot + path).c_str());
    }
    return loadImage;
}