/// Responsible for providing functionality for the Reader window.
#pragma once

#include <string>
#include <iostream>

#include "nlohmann/json.hpp"
#include "Logger.h"
#include "Dextop.h"
#include "Dexxor.h"
#include "DextopPrimaryWindow.h"



class DTReaderController
{
    private:
        slint::ComponentHandle<DextopReaderWindow> windowHandle = DextopReaderWindow::create();

        //chapter list view data
        std::string mangaID;
        std::string title;
        std::vector<slint::SharedString> authors;
        std::vector<slint::SharedString> artists;

        //reader view data



        //Populates the list of chapters
        void PopulateChapters(const int limit, const int page)
        {
            dtlog << "Retrieving chapter data for \"" << mangaID << "\" (limit: " << limit << ", offset: " << (page * limit) << ")" << std::endl;
            slint::blocking_invoke_from_event_loop([&]
            {
                windowHandle->set_chapterSet(std::make_shared<slint::VectorModel<ReaderCVChapter>>(std::vector<ReaderCVChapter>()));
                windowHandle->set_chapterListLoading(true);
                windowHandle->set_chapterListBGText(slint::SharedString("Loading..."));
            });

            nlohmann::json chapterData = Dextop::GetInstance()->dexxor.GetChapters(mangaID.data(), std::to_string(limit), std::to_string(page * limit));
            size_t resultCount = chapterData["data"].size();
            dtlog << "Done (got " << resultCount << " of total " << chapterData["total"].get<int>() << "). Populating UI..." << std::endl;

            if (resultCount > 0)
            {
                std::vector<ReaderCVChapter> chapterSet;
                for (int i = 0; i < resultCount; i++)
                {
                    nlohmann::json chapterInfo = chapterData["data"][i];
                    ReaderCVChapter makeChapter;

                    
                    makeChapter.id = chapterInfo["id"].begin().value();
                    makeChapter.title = std::string("Unknown Chapter");
                    if (chapterInfo["attributes"]["chapter"].is_null() == false)
                    {
                        makeChapter.title = ("Ch. " + std::string(chapterInfo["attributes"]["chapter"].begin().value()));
                    }
                    if (chapterInfo["attributes"]["title"].is_null() == false && chapterInfo["attributes"]["title"].begin().value() != "")
                    {
                        makeChapter.title = makeChapter.title + ": " + chapterInfo["attributes"]["title"].begin().value();
                    }
                    for (int i = 0; i < chapterInfo["relationships"].size(); i++)
                    {
                        if (chapterInfo["relationships"].at(i)["type"].begin().value() == "scanlation_group")
                        {
                            makeChapter.groupName = chapterInfo["relationships"].at(i)["attributes"]["name"].begin().value();
                        }
                    }
                    chapterSet.push_back(makeChapter);
                }

                //determine page count
                int fLimit = chapterData["limit"].get<int>();
                int fTotal = chapterData["total"].get<int>();
                int pageCount = 0;
                if (fTotal > 0)
                {
                    div_t pageDiv = std::div(fTotal, fLimit);
                    pageCount = pageDiv.quot + (pageDiv.rem > 0 ? 1 : 0);
                }
                
                //apply to UI
                slint::invoke_from_event_loop([=]
                {
                    windowHandle->set_chapterSet(std::make_shared<slint::VectorModel<ReaderCVChapter>>(chapterSet));
                    windowHandle->set_chapterListBGText(slint::SharedString(""));
                    windowHandle->set_chapterListLoading(false);
                    windowHandle->set_chapterListPageCount(pageCount);
                });
            }
            else
            {
                slint::invoke_from_event_loop([&]
                {
                    windowHandle->set_chapterListBGText(slint::SharedString("No chapters available"));
                    windowHandle->set_chapterListLoading(false);
                });
            }
        }

        //Retrieves and sets the cover art on the chapter list
        void SetCover()
        {
            Dextop* dextop = Dextop::GetInstance();
            std::string coverLocalPath = "";
            for (int j = 0; j < this->json["relationships"].size(); j++)
            {
                std::string relType = this->json["relationships"][j]["type"].get<std::string>();
                if (relType == "cover_art")
                {
                    coverLocalPath = this->json["relationships"][j]["attributes"]["fileName"].get<std::string>();
                    dextop->assetManager.GetMangaCover(
                        std::string("https://uploads.mangadex.org/covers/") + this->mangaID + "/" + coverLocalPath,
                        coverLocalPath
                    );
                }
            }
            if (coverLocalPath != "")
            {
                dtlog << "setting coverart... " << coverLocalPath << std::endl;
                slint::Image loadImage = dextop->assetManager.ImageLoadWR("images/covers/" + coverLocalPath);
                dtlog << "coverart loaded!" << std::endl;
                slint::invoke_from_event_loop([=]()
                {
                    this->windowHandle->set_coverArt(loadImage);
                });
                dtlog << "coverart done!" << std::endl;
            }
        }

        void RVLoadChapter(const std::string chapterID, const int pageIndex)
        {

        }
        
        void RVChangeImage(const int index)
        {

        }
    public:
        nlohmann::json json;

        DTReaderController(nlohmann::json mangaJson)
        {
            //parse json
            json = mangaJson;

            mangaID = json["id"].begin().value();
            title = json["attributes"]["title"].begin().value();
            for (int i = 0; i < json["relationships"].size(); i++)
            {
                if (json["relationships"].at(i)["type"].begin().value() == "author")
                {
                    authors.push_back(slint::SharedString(json["relationships"].at(i)["attributes"]["name"].get<std::string>()));
                }
                else if (json["relationships"].at(i)["type"].begin().value() == "artist")
                {
                    artists.push_back(slint::SharedString(json["relationships"].at(i)["attributes"]["name"].get<std::string>()));
                }
            }

            //init window
            //close handling
            windowHandle->window().on_close_requested([&]{
                delete this;
                return slint::CloseRequestResponse::HideWindow;
            });

            //prepare chapter list view
            windowHandle->set_windowTitle(slint::SharedString(title));
            dtThreadPool.detach_task([&]
            {
                SetCover();
            });
            windowHandle->set_authors(std::make_shared<slint::VectorModel<slint::SharedString>>(authors));
            windowHandle->set_artists(std::make_shared<slint::VectorModel<slint::SharedString>>(artists));
            windowHandle->on_loadChapters([&](int page)
            {
                dtThreadPool.detach_task([=]
                {
                    PopulateChapters(50, page - 1);
                });
            });
            windowHandle->invoke_loadChapters(1);

            //prepare chapter reader view
            windowHandle->on_readerChangePage([&](int page)
            {
                
            });

            windowHandle->show();
        }
};