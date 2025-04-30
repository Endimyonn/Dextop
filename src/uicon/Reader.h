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
        struct CurrentChapterData
        {
            std::string chapterID;
            std::string title;
            int pageCount;
            int fetchOffset;
            time_t serverAcquiredAt;
            std::string baseURL;
            std::string hash;
            std::vector<std::string> imageNames;
            std::vector<std::string> dataSaverImageNames;
            std::string adjacentChapters[2];
            int adjacentChapterPageCounts[2];
        };
        CurrentChapterData currentChapterData;


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
                    makeChapter.fetchOffset = (page * limit) + (i);
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
                slint::Image loadImage = dextop->assetManager.ImageLoadWR("images/covers/" + coverLocalPath);
                slint::invoke_from_event_loop([=]()
                {
                    this->windowHandle->set_coverArt(loadImage);
                });
            }
        }

        void RVLoadChapter(const std::string chapterID, const int chapterFetchOffset, const int startPage)
        {
            dtlog << "RVLoadChapter: loading chapter " << chapterID << std::endl;

            Dextop* dextop = Dextop::GetInstance();
            slint::blocking_invoke_from_event_loop([&]
            {
                windowHandle->set_readerActive(true);
                windowHandle->set_readerImage(slint::Image());
            });

            //fetch image asset info
            nlohmann::json chapterAssetInfo = dextop->dexxor.GetChapterImageMeta(chapterID);
            time(&currentChapterData.serverAcquiredAt);
            currentChapterData.baseURL = chapterAssetInfo["baseUrl"].get<std::string>();
            currentChapterData.hash = chapterAssetInfo["chapter"]["hash"].get<std::string>();
            currentChapterData.imageNames.clear();
            currentChapterData.dataSaverImageNames.clear();
            for (int i = 0; i < chapterAssetInfo["chapter"]["data"].size(); i++)
            {
                currentChapterData.imageNames.push_back(chapterAssetInfo["chapter"]["data"].at(i).get<std::string>());
                currentChapterData.dataSaverImageNames.push_back(chapterAssetInfo["chapter"]["dataSaver"].at(i).get<std::string>());
            }

            //fetch chapter (+ adjacent) data
            currentChapterData.adjacentChapters[0] = "";
            currentChapterData.adjacentChapterPageCounts[0] = 0;
            currentChapterData.adjacentChapters[1] = "";
            currentChapterData.adjacentChapterPageCounts[1] = 0;
            nlohmann::json getChapters = dextop->dexxor.GetChapters(mangaID, "3", std::to_string(chapterFetchOffset));
            for (int i = 0; i < getChapters["data"].size(); i++)
            {
                if (getChapters["data"].at(i)["id"].get<std::string>() != chapterID)
                {
                    if (i == 0)
                    {
                        currentChapterData.adjacentChapters[0] = getChapters["data"].at(i)["id"].get<std::string>();
                        currentChapterData.adjacentChapterPageCounts[0] = getChapters["data"].at(i)["attributes"]["pages"].get<int>();
                    }
                    else
                    {
                        currentChapterData.adjacentChapters[1] = getChapters["data"].at(i)["id"].get<std::string>();
                        currentChapterData.adjacentChapterPageCounts[1] = getChapters["data"].at(i)["attributes"]["pages"].get<int>();
                        break;
                    }
                }
                else
                {
                    currentChapterData.title = "";
                    if (getChapters["data"].at(i)["attributes"]["chapter"].is_null() == false)
                    {
                        currentChapterData.title = ("Chapter " + std::string(getChapters["data"].at(i)["attributes"]["chapter"].begin().value()));
                    }
                    if (getChapters["data"].at(i)["attributes"]["title"].is_null() == false && getChapters["data"].at(i)["attributes"]["title"].begin().value() != "")
                    {
                        currentChapterData.title = currentChapterData.title + ": " + getChapters["data"].at(i)["attributes"]["title"].get<std::string>();
                    }
                    currentChapterData.pageCount = getChapters["data"].at(i)["attributes"]["pages"].get<int>();
                }
            }

            currentChapterData.chapterID = chapterID;
            currentChapterData.fetchOffset = chapterFetchOffset;

            //activate reader UI
            slint::invoke_from_event_loop([&]()
            {
                if (currentChapterData.title != "")
                {
                    windowHandle->set_windowTitle(slint::SharedString(title + " " + currentChapterData.title));
                }
                windowHandle->set_readerChapterPageCount(currentChapterData.pageCount);
                windowHandle->set_readerActive(true);
            });

            dtThreadPool.detach_task([=]()
            {
                RVChangeImage(startPage);
            });
        }
        
        void RVChangeImage(int index)
        {
            index = std::clamp(index, -1, currentChapterData.pageCount);
            if (index == -1 && currentChapterData.adjacentChapters[0] != "")
            {
                //load previous chapter
                dtlog << "RVChangeImage: loading previous chapter" << std::endl;

                RVLoadChapter(
                    currentChapterData.adjacentChapters[0],
                    std::clamp(currentChapterData.fetchOffset - 1, 0, INT32_MAX),
                    currentChapterData.adjacentChapterPageCounts[0] - 1);
            }
            else if (index == currentChapterData.pageCount && currentChapterData.adjacentChapters[1] != "")
            {
                //load next chapter
                dtlog << "RVChangeImage: loading next chapter" << std::endl;

                RVLoadChapter(
                    currentChapterData.adjacentChapters[1],
                    currentChapterData.fetchOffset + 1,
                    0);
            }
            else
            {
                //change the image intra-chapter
                dtlog << "RVChangeImage: changing image to image " << index << " (max: " << currentChapterData.pageCount - 1 << ")" << std::endl;

                //send for the needed image
                Dextop* dextop = Dextop::GetInstance();
                dextop->assetManager.GetChapterPage(
                    currentChapterData.baseURL + "/data/" + currentChapterData.hash + "/" + currentChapterData.imageNames[index],
                    currentChapterData.chapterID,
                    currentChapterData.imageNames[index]
                );
                slint::Image loadImage = dextop->assetManager.ImageLoadWR("images/pages/" + currentChapterData.chapterID + "/" + currentChapterData.imageNames[index]);
                slint::invoke_from_event_loop([=]()
                {
                    this->windowHandle->set_readerImage(loadImage);
                    this->windowHandle->set_readerChapterPageIndex(index);
                    this->windowHandle->set_readerCanGoPrev(index > 0 || currentChapterData.adjacentChapters[0] != "");
                    this->windowHandle->set_readerCanGoNext(index < currentChapterData.pageCount || currentChapterData.adjacentChapters[1] != "");
                });

                //send for directly adjacent images
                if (index > 0)
                {
                    dtThreadPool.detach_task([=]()
                    {
                        dextop->assetManager.GetChapterPage(
                            currentChapterData.baseURL + "/data/" + currentChapterData.hash + "/" + currentChapterData.imageNames[index - 1],
                            currentChapterData.chapterID,
                            currentChapterData.imageNames[index - 1]
                        );
                    });
                }
                if (index < currentChapterData.imageNames.size() - 1)
                {
                    dtThreadPool.detach_task([=]()
                    {
                        dextop->assetManager.GetChapterPage(
                            currentChapterData.baseURL + "/data/" + currentChapterData.hash + "/" + currentChapterData.imageNames[index + 1],
                            currentChapterData.chapterID,
                            currentChapterData.imageNames[index + 1]
                        );
                    });
                }
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
            windowHandle->on_populateChapters([&](int page)
            {
                dtThreadPool.detach_task([=]
                {
                    PopulateChapters(50, page - 1);
                });
            });
            windowHandle->invoke_populateChapters(1);
            windowHandle->on_loadChapter([&](slint::SharedString chapterID, int chapterFetchOffset, int startPage)
            {
                dtThreadPool.detach_task([=]()
                {
                    dtlog << "pre: " << (chapterFetchOffset - 1) << std::endl;
                    dtlog << "pos: " << std::clamp(chapterFetchOffset - 1, 0, INT32_MAX) << std::endl;
                    RVLoadChapter(chapterID.data(), std::clamp(chapterFetchOffset - 1, 0, INT32_MAX), startPage);
                });
            });

            //prepare chapter reader view
            windowHandle->on_readerChangePage([&](int page)
            {
                dtThreadPool.detach_task([=]()
                {
                    RVChangeImage(page);
                });
            });
            windowHandle->on_readerClose([&]()
            {
                windowHandle->set_windowTitle(slint::SharedString(title));
                windowHandle->set_readerActive(false);
            });

            windowHandle->show();
        }
};