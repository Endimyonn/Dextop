#pragma once

#include "../Dextop.h"
#include <sstream>



std::string url_encode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

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
            nlohmann::json searchResults = dextop->dexxor.Search(url_encode(title), limit, page);
            size_t resultCount = searchResults["data"].size();
            dtlog << "Search for \"" << title << "\": found " << resultCount << " (limit: " << limit << ", page: " << page << ", total: " << searchResults["total"] << ")" << std::endl;
        
            //pass data to the UI
            std::vector<SearchResult> uiResults;
            std::vector<std::string> coverPaths;
            for (int i = 0; i < resultCount; i++)
            {
                nlohmann::json resultInfo = searchResults["data"][i];
                
                SearchResult makeResult;
                makeResult.title = resultInfo["attributes"]["title"].begin().value();
                if (resultInfo["attributes"]["description"].size() > 0)
                {
                    makeResult.description = resultInfo["attributes"]["description"].begin().value();
                }
                else
                {
                    makeResult.description = "(no description available)";
                }
                makeResult.id = resultInfo["id"].get<std::string>();
                for (int j = 0; j < resultInfo["relationships"].size(); j++)
                {
                    if (resultInfo["relationships"][j]["type"].get<std::string>() == "cover_art")
                    {
                        makeResult.coverFile = resultInfo["relationships"][j]["attributes"]["fileName"].begin().value();
                        coverPaths.push_back(std::string("https://uploads.mangadex.org/covers/") + std::string(makeResult.id) + "/" + resultInfo["relationships"][j]["attributes"]["fileName"].get<std::string>() + Dextop_Cover256Suffix);
                        coverPaths.push_back(resultInfo["relationships"][j]["attributes"]["fileName"].get<std::string>() + Dextop_Cover256Suffix);
                    }
                }
                uiResults.push_back(makeResult);
            }

            //send for the images
            for (int i = 0; i < coverPaths.size(); i += 2)
            {
                dtThreadPool.detach_task([=]
                {
                    dextop->assetManager.GetMangaCover(coverPaths[i], coverPaths[i + 1]);
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
        
                std::vector<std::string> updatePaths;
                for (int i = 0; i < resultCount; i++)
                {
                    SearchResult newResult;
                    newResult.id = dextop->ui->get_results()->row_data(i)->id;
                    newResult.title = dextop->ui->get_results()->row_data(i)->title;
                    newResult.description = dextop->ui->get_results()->row_data(i)->description;
                    newResult.coverFile = dextop->ui->get_results()->row_data(i)->coverFile;
                    newResult.cover = slint::Image::load_from_path("assets/images/placeholders/cover-low.png");
                    dextop->ui->get_results()->set_row_data(i, newResult);
                    updatePaths.push_back(std::string("images/covers/") + std::string(uiResults[i].coverFile + Dextop_Cover256Suffix));
                }
                dtThreadPool.detach_task([=]
                {
                    UpdateResultImages(searchID, updatePaths);
                });
            });
        }

        static void UpdateResultImages(std::string searchID, std::vector<std::string> imagePaths)
        {
            Dextop* dextop = Dextop::GetInstance();
            for (int i = 0; i < imagePaths.size(); i++)
            {
                slint::Image loadImage = dextop->assetManager.ImageLoadWR(imagePaths[i]);
                slint::invoke_from_event_loop([=](){
                    SearchResult newResult;
                    newResult.id = dextop->ui->get_results()->row_data(i)->id;
                    newResult.title = dextop->ui->get_results()->row_data(i)->title;
                    newResult.description = dextop->ui->get_results()->row_data(i)->description;
                    newResult.coverFile = dextop->ui->get_results()->row_data(i)->coverFile;
                    newResult.cover = loadImage;
                    dextop->ui->get_results()->set_row_data(i, newResult);
                });
            }
        }
};
