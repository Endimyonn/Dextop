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
        std::string mangaID;
        std::string title;
        std::vector<slint::SharedString> authors;
        std::vector<slint::SharedString> artists;

        void PopulateChapters(int limit, int page)
        {
            slint::blocking_invoke_from_event_loop([&]
            {
                windowHandle->set_chapterListBGText(slint::SharedString("Loading..."));
            });

            dtlog << "Retrieving chapter data for \"" << mangaID << "\"" << std::endl;
            nlohmann::json chapterData = Dextop::GetInstance()->dexxor.GetChapters(mangaID.data(), std::to_string(limit), std::to_string(page * limit));
            dtlog << "out: " << chapterData.dump() << std::endl;
            size_t resultCount = chapterData["data"].size();
            dtlog << "Done (got " << resultCount << "). Populating UI..." << std::endl;

            if (resultCount > 0)
            {
                std::vector<ReaderMVChapter> chapterSet;
                for (int i = 0; i < resultCount; i++)
                {
                    nlohmann::json chapterInfo = chapterData["data"][i];
                    ReaderMVChapter makeChapter;

                    
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
                
                slint::invoke_from_event_loop([=]{
                    windowHandle->set_chapterSet(std::make_shared<slint::VectorModel<ReaderMVChapter>>(chapterSet));
                });
            }
            else
            {
                dtlog << "a !" << std::endl;
                slint::invoke_from_event_loop([&]
                {
                    dtlog << "b ." << std::endl;
                    windowHandle->set_chapterListBGText(slint::SharedString("No chapters available"));
                    dtlog << "victory is mine" << std::endl;
                });
            }
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
                    authors.push_back(json["relationships"].at(i)["attributes"]["name"].get<std::string>());
                }
                else if (json["relationships"].at(i)["type"].begin().value() == "artist")
                {
                    artists.push_back(json["relationships"].at(i)["attributes"]["name"].get<std::string>());
                }
            }

            //init window
            windowHandle->window().on_close_requested([&]{
                delete this;
                return slint::CloseRequestResponse::HideWindow;
            });

            windowHandle->set_readerTitle(slint::SharedString(title));
            windowHandle->set_authors(std::make_shared<slint::Model<slint::SharedString>>(authors));
            windowHandle->set_artists(std::make_shared<slint::Model<slint::SharedString>>(artists));
            windowHandle->on_loadChapters([&]{
                dtThreadPool.detach_task([&]
                {
                    PopulateChapters(50, 0);
                });
            });
            windowHandle->invoke_loadChapters();
            windowHandle->show();
        }
};