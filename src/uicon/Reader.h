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
            time_t serverAcquiredAt;
            std::string baseURL;
            std::string hash;
            std::vector<std::string> imageNames;
            std::vector<std::string> dataSaverImageNames;
        };
        CurrentChapterData currentChapterData;

        struct AggregateChapterData
        {
            std::string volume;
            std::string number;
            std::string chapterID;
            std::vector<std::string> others;
        };
        std::vector<AggregateChapterData> chapterAggData;

        std::string AdjacentChapter(const std::string chapterID, const int offset)
        {
            for (int i = 0; i < chapterAggData.size(); i++)
            {
                if (chapterAggData[i].chapterID == chapterID)
                {
                    if (i + offset >= 0 && i + offset <= chapterAggData.size())
                    {
                        return chapterAggData[i + offset].chapterID;
                    }
                }
            }
            return "";
        }

        bool IsFirstChapter(const std::string chapterID)
        {
            return chapterAggData.front().chapterID == chapterID;
        }

        bool IsLastChapter(const std::string chapterID)
        {
            return chapterAggData.back().chapterID == chapterID;
        }

        void Initialize(nlohmann::json mangaJson)
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

            //prepare manga info/chapter list view
            windowHandle->set_windowTitle(slint::SharedString(title));
            dtThreadPool.detach_task([&]()
            {
                FetchCover();
            });
            dtThreadPool.detach_task([&]()
            {
                FetchMangaStats();
            });
            std::future<void> chapterAggTask = dtThreadPool.submit_task([&]()
            {
                FetchChapterAggData();
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
            windowHandle->on_loadChapter([&](slint::SharedString chapterID, int startPage)
            {
                dtThreadPool.detach_task([=]()
                {
                    RVLoadChapter(chapterID.data(), startPage);
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

            chapterAggTask.wait();
        }


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
                    makeChapter.pageCount = chapterInfo["attributes"]["pages"].get<int>();
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
        void FetchCover()
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

        //Retrieves and sets the rating statistic
        void FetchMangaStats()
        {
            Dextop* dextop = Dextop::GetInstance();
            nlohmann::json statsJson = dextop->dexxor.GetMangaStatistics(mangaID);
            if (statsJson.contains("rating")) //avoid bad responses
            {
                slint::invoke_from_event_loop([=]()
                {
                    windowHandle->set_rating(slint::SharedString(std::to_string(statsJson["rating"]["bayesian"].get<float>()).substr(0, 4)));
                });
            }
        }

        //Retrieve aggregate volume/chapter info
        void FetchChapterAggData()
        {
            Dextop* dextop = Dextop::GetInstance();
            nlohmann::json aggData = dextop->dexxor.GetChaptersAggregate(mangaID);

            for (auto iterVolume = aggData.begin(); iterVolume != aggData.end(); iterVolume++)
            {
                nlohmann::json chapterSet = (*iterVolume)["chapters"];
                for (auto iterChapter = chapterSet.begin(); iterChapter != chapterSet.end(); iterChapter++)
                {
                    AggregateChapterData newData;
                    newData.volume = (*iterVolume)["volume"].get<std::string>();
                    newData.number = (*iterChapter)["chapter"].get<std::string>();
                    newData.chapterID = (*iterChapter)["id"].get<std::string>();
                    if ((*iterChapter)["others"].size() > 0)
                    {
                        for (auto iterOthers = (*iterChapter)["others"].begin(); iterOthers != (*iterChapter)["others"].end(); iterOthers++)
                        {
                            newData.others.push_back((*iterOthers).get<std::string>());
                        }
                    }
                    chapterAggData.push_back(newData);
                }
            }
        }

        //Loads a chapter by its ID. If startPage is -1, it will start at the end.
        void RVLoadChapter(const std::string chapterID, const int startPage = 0)
        {
            dtlog << "RVLoadChapter: loading chapter " << chapterID << std::endl;

            Dextop* dextop = Dextop::GetInstance();
            slint::blocking_invoke_from_event_loop([&]
            {
                windowHandle->set_readerActive(true);
                windowHandle->set_readerImage(slint::Image());
            });

            //send requests, wait for both to come back
            std::future<nlohmann::json> getChapterAssetInfo = dtThreadPool.submit_task([&]
            {
                return dextop->dexxor.GetChapterImageMeta(chapterID);
            });
            std::future<nlohmann::json> getChapterData = dtThreadPool.submit_task([&]
            {
                return dextop->dexxor.GetChapter(chapterID);
            });
            nlohmann::json chapterAssetInfo = getChapterAssetInfo.get();
            nlohmann::json chapterData = getChapterData.get();
            

            //fetch image asset info
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
            currentChapterData.title = "";
            if (chapterData["data"].at(0)["attributes"]["chapter"].is_null() == false)
            {
                currentChapterData.title = ("Chapter " + std::string(chapterData["data"].at(0)["attributes"]["chapter"].begin().value()));
            }
            if (chapterData["data"].at(0)["attributes"]["title"].is_null() == false && chapterData["data"].at(0)["attributes"]["title"].begin().value() != "")
            {
                currentChapterData.title = currentChapterData.title + ": " + chapterData["data"].at(0)["attributes"]["title"].get<std::string>();
            }
            currentChapterData.pageCount = chapterData["data"].at(0)["attributes"]["pages"].get<int>();

            currentChapterData.chapterID = chapterID;

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
                if (startPage != -1)
                {
                    RVChangeImage(startPage);
                }
                else
                {
                    RVChangeImage(currentChapterData.pageCount - 1);
                }
            });
        }
        
        void RVChangeImage(int index)
        {
            index = std::clamp(index, -1, currentChapterData.pageCount);
            if (index == -1 && !IsFirstChapter(currentChapterData.chapterID))
            {
                //load previous chapter
                dtlog << "RVChangeImage: loading previous chapter" << std::endl;

                RVLoadChapter(
                    AdjacentChapter(currentChapterData.chapterID, -1),
                    -1);
            }
            else if (index >= currentChapterData.pageCount && !IsLastChapter(currentChapterData.chapterID))
            {
                //load next chapter
                dtlog << "RVChangeImage: loading next chapter" << std::endl;

                RVLoadChapter(
                    AdjacentChapter(currentChapterData.chapterID, 1),
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

                //apply the image
                slint::invoke_from_event_loop([=]()
                {
                    this->windowHandle->set_readerImage(loadImage);
                    this->windowHandle->set_readerChapterPageIndex(index);
                    this->windowHandle->set_readerCanGoPrev(index > 0 || !IsFirstChapter(currentChapterData.chapterID));
                    this->windowHandle->set_readerCanGoNext(index < currentChapterData.pageCount - 1 || !IsLastChapter(currentChapterData.chapterID));
                });

                //send for up to 4 adjacent images
                if (index > 0) //page - 1
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
                if (index > 1) //page - 2
                {
                    dtThreadPool.detach_task([=]()
                    {
                        dextop->assetManager.GetChapterPage(
                            currentChapterData.baseURL + "/data/" + currentChapterData.hash + "/" + currentChapterData.imageNames[index - 2],
                            currentChapterData.chapterID,
                            currentChapterData.imageNames[index - 2]
                        );
                    });
                }
                /*if (index < currentChapterData.imageNames.size() - 1) //page + 1
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
                if (index < currentChapterData.imageNames.size() - 2) //page + 2
                {
                    dtThreadPool.detach_task([=]()
                    {
                        dextop->assetManager.GetChapterPage(
                            currentChapterData.baseURL + "/data/" + currentChapterData.hash + "/" + currentChapterData.imageNames[index + 2],
                            currentChapterData.chapterID,
                            currentChapterData.imageNames[index + 2]
                        );
                    });
                }*/
            }
        }
    public:
        nlohmann::json json;

        DTReaderController(nlohmann::json mangaJson)
        {
            Initialize(mangaJson);
            windowHandle->show();
        }

        DTReaderController(std::string chapterID, std::string mangaID)
        {
            Dextop* dextop = Dextop::GetInstance();
            Initialize(dextop->dexxor.GetManga(mangaID));
            windowHandle->invoke_loadChapter(slint::SharedString(chapterID), 0);
            windowHandle->show();
        }
};