/// Responsible for providing functionality for the Reader window.
#pragma once

#include <string>
#include <iostream>

#include "nlohmann/json.hpp"
#include "Logger.h"
#include "Dexxor.h"
#include "DextopPrimaryWindow.h"



void PopulateChapters(slint::ComponentHandle<DextopReaderWindow> reader, Dexxor& dexxor, std::string mangaID)
{
    dtlog << "Retrieving chapter data for \"" << mangaID << "\"" << std::endl;
    auto getChapters = std::async(&Dexxor::GetChapters, dexxor, mangaID, "50", "0");
    nlohmann::json chapterData = getChapters.get();
    size_t resultCount = chapterData["data"].size();
    dtlog << "Done (got " << resultCount << "). Populating UI..." << std::endl;

    std::vector<ReaderMVChapter> chapterSet;
    for (int i = 0; i < resultCount; i++)
    {
        nlohmann::json chapterInfo = chapterData["data"][i];
        ReaderMVChapter makeChapter;

        makeChapter.chapter = chapterInfo["attributes"]["chapter"].begin().value();
        makeChapter.title = std::string("DUMMY");
        if (chapterInfo["attributes"]["title"].is_null() == false && chapterInfo["attributes"]["title"].begin().value() != "")
        {
            makeChapter.title = chapterInfo["attributes"]["title"].begin().value();
        }
        makeChapter.id = chapterInfo["id"].begin().value();
        chapterSet.push_back(makeChapter);
    }
    size_t abcd = chapterSet.size();
    dtlog << "chapterSet size: " << abcd << "!" << std::endl;
    
    slint::invoke_from_event_loop([=](){
        reader->set_chapterSet(std::make_shared<slint::VectorModel<ReaderMVChapter>>(chapterSet));
        for (int i = 0; i < chapterSet.size(); i++)
        {
            dtlog << i << ": " << chapterSet[i].title << "/" << chapterSet[i].id << "/" << chapterSet[i].chapter << std::endl;
        }
    });
    dtlog << "Done." << std::endl;
}

void InitReader(slint::SharedString mangaID, Dexxor& dexxor)
{
    slint::ComponentHandle<DextopReaderWindow> reader = DextopReaderWindow::create();
    reader->set_readerTitle(mangaID);
    reader->on_loadChapters([&]{
        std::thread(PopulateChapters, reader, std::ref(dexxor), std::string(mangaID)).detach();
    });
    reader->show();
    reader->invoke_loadChapters();
}