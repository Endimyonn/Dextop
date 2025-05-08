#pragma once

#include "../Dextop.h"
#include "../DextopUtil.h"



class DTMainSearchController
{
    private:
        inline static std::string searchID = "";
    public:
        static void DoTitleSearch(std::string title, unsigned short limit = 10, unsigned short page = 0)
        {
            searchID = title + std::to_string(limit) + std::to_string(page);
            Dextop* dextop = Dextop::GetInstance();

            slint::invoke_from_event_loop([=](){
                dextop->ui->set_results(std::make_shared<slint::VectorModel<SearchResult>>(std::vector<SearchResult>()));
                dextop->ui->set_searchLoading(true);
                dextop->ui->set_currentSearch(slint::SharedString(title));
            });
            nlohmann::json searchResults = dextop->dexxor.Search(DextopUtil::EncodeURL(title), limit, page);
            size_t resultCount = searchResults["data"].size();
            dtlog << "Search for \"" << title << "\": found " << resultCount << " (limit: " << limit << ", page: " << page << ", total: " << searchResults["total"] << ")" << std::endl;
        
            //pass data to the UI
            std::vector<SearchResult> uiResults;
            std::vector<std::string> coverURLs;
            std::vector<std::string> coverNames;
            for (int i = 0; i < resultCount; i++)
            {
                nlohmann::json resultInfo = searchResults["data"][i];
                
                SearchResult makeResult;
                makeResult.json = resultInfo.dump();
                makeResult.title = resultInfo["attributes"]["title"].begin().value();
                if (resultInfo["attributes"]["description"].size() > 0)
                {
                    makeResult.description = resultInfo["attributes"]["description"].begin().value();
                }
                else
                {
                    makeResult.description = "(no description available)";
                }
                makeResult.cover = slint::Image::load_from_path("assets/images/placeholders/cover-low.png");

                for (int j = 0; j < resultInfo["relationships"].size(); j++)
                {
                    std::string relType = resultInfo["relationships"][j]["type"].get<std::string>();
                    if (relType == "cover_art")
                    {
                        coverURLs.push_back(std::string("https://uploads.mangadex.org/covers/") + resultInfo["id"].get<std::string>() + "/" + resultInfo["relationships"][j]["attributes"]["fileName"].get<std::string>() + Dextop_Cover256Suffix);
                        coverNames.push_back(resultInfo["relationships"][j]["attributes"]["fileName"].get<std::string>() + Dextop_Cover256Suffix);
                    }
                }
                uiResults.push_back(makeResult);
            }

            //send for the cover images
            for (int i = 0; i < coverURLs.size(); i++)
            {
                dtThreadPool.detach_task([=]
                {
                    dextop->assetManager.GetMangaCover(coverURLs[i], coverNames[i]);
                });
            }

            slint::invoke_from_event_loop([=](){
                dextop->ui->set_results(std::make_shared<slint::VectorModel<SearchResult>>(uiResults));
                dextop->ui->set_searchLoading(false);
        
                int fLimit = searchResults["limit"].get<int>();
                int fTotal = searchResults["total"].get<int>();
                int pageCount = 0;
                if (fTotal > 0)
                {
                    div_t pageDiv = std::div(fTotal, fLimit);
                    pageCount = pageDiv.quot + (pageDiv.rem > 0 ? 1 : 0);
                }
                dextop->ui->set_pageCount(pageCount);
        
                //queue up cover-application
                std::vector<std::string> updatePaths;
                for (int i = 0; i < coverNames.size(); i++)
                {
                    updatePaths.push_back(std::string("images/covers/") + coverNames[i]);
                }
                dtThreadPool.detach_task([=]
                {
                    UpdateResultImages(searchID, updatePaths);
                });
            });
        }

        static void UpdateResultImages(std::string curSearchID, std::vector<std::string> imagePaths)
        {
            Dextop* dextop = Dextop::GetInstance();
            for (int i = 0; i < imagePaths.size(); i++)
            {
                slint::Image loadImage = dextop->assetManager.ImageLoadWR(imagePaths[i]);
                slint::invoke_from_event_loop([=](){
                    if (curSearchID == searchID)
                    {
                        auto oldResult = dextop->ui->get_results()->row_data(i);
                        SearchResult newResult;
                        newResult.json = oldResult->json;
                        newResult.title = oldResult->title;
                        newResult.description = oldResult->description;
                        newResult.cover = loadImage;
                        dextop->ui->get_results()->set_row_data(i, newResult);
                    }
                });
            }
        }
};
