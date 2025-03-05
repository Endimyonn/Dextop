#pragma once

#include "Dexxor.h"
#include "DextopPrimaryWindow.h"
#include <string>

class AssetManager
{
    public:
        AssetManager();
        ~AssetManager();

        struct ImageStatus
        {
            bool ready = false;
            std::string data = "";
        };
        void GetImage(std::string url, std::string path);
        void GetChapterPage(std::string url, std::string fileName);
        void GetMangaCover(std::string url, std::string fileName);

        void ImageLoad(std::optional<SearchResult>* result, std::string path);
    private:
        void CreateDirectorySkeleton(std::string baseDir);
};