#pragma once

#include <string>
#include <iostream>

#include "nlohmann/json.hpp"
#include "Dexxor.h"
#include "DextopPrimaryWindow.h"



void PopulateChapters(slint::ComponentHandle<DextopReaderWindow>& reader, Dexxor& dexxor, std::string mangaID)
{
    auto getChapters = std::async(&Dexxor::GetChapters, dexxor, mangaID);
    nlohmann::json chapterData = getChapters.get();
    //size_t resultCount = searchResults["data"].size();
    slint::invoke_from_event_loop([=](){

    });
}

void InitReader(slint::SharedString mangaID, Dexxor& dexxor)
{
    slint::ComponentHandle<DextopReaderWindow> reader = DextopReaderWindow::create();
    reader->set_readerTitle(mangaID);
    reader->on_loadChapters([&]{
        //std::thread(PopulateChapters, reader, dexxor, std::string(mangaID)).detach();
    });
    reader->show();
}