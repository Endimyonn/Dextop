/// The AssetManager is responsible for retrieval and storage of remote assets (mostly images), as
/// well as filesystem management and lazy-image-loading.
#pragma once

#include "Dexxor.h"
#include "DextopPrimaryWindow.h"
#include <slint.h>
#include <string>

class AssetManager
{
    public:
        AssetManager();
        ~AssetManager();

        void GetImage(std::string url, std::string path);
        void GetChapterPage(std::string url, std::string chapterID, std::string fileName);
        void GetMangaCover(std::string url, std::string fileName);

        slint::Image ImageLoadWR(std::string path);
    private:
        void CreateDirectorySkeleton(std::string baseDir);
        std::vector<std::string> assetFetchLocks;
        bool AddAssetFetchLock(std::string path);
        void RemoveAssetFetchLock(std::string path);
        bool AssetFetchLocked(std::string path);
};