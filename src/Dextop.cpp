#include "Dextop.h"
#include "Dextop_Defs.h"
#include <windows.h>
#include <filesystem>

using namespace std;
using json = nlohmann::json;

Dexxor localDexxor;
AssetManager assetManager;
static Logger logger;
static slint::ComponentHandle<DextopPrimaryWindow> ui = DextopPrimaryWindow::create();

string url_encode(const string &value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char) c);
        escaped << nouppercase;
    }

    return escaped.str();
}

void DoTitleSearch(std::string title, unsigned short limit = 10, unsigned short page = 0)
{
    slint::invoke_from_event_loop([=](){
        ui->set_results(std::make_shared<slint::VectorModel<SearchResult>>(std::vector<SearchResult>()));
        ui->set_searchLoading(true);
        ui->set_currentSearch(slint::SharedString(title));
    });
    auto doSearch = std::async(&Dexxor::Search, &localDexxor, url_encode(title), limit, page);
    json searchResults = doSearch.get();
    size_t resultCount = searchResults["data"].size();
    cout << "Search for \"" << title << "\": found " << resultCount << " (limit: " << limit << ", page: " << page << ", total: " << searchResults["total"] << ")" << endl;

    //pass data to the UI
    std::vector<SearchResult> uiResults;
    for (int i = 0; i < resultCount; i++)
    {
        json resultInfo = searchResults["data"][i];
        
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
        makeResult.id = resultInfo["id"].get<string>();
        for (int j = 0; j < resultInfo["relationships"].size(); j++)
        {
            if (resultInfo["relationships"][j]["type"].get<string>() == "cover_art")
            {
                makeResult.coverFile = resultInfo["relationships"][j]["attributes"]["fileName"].begin().value();
                std::string coverURL = std::string("https://uploads.mangadex.org/covers/") + std::string(makeResult.id) + "/" + resultInfo["relationships"][j]["attributes"]["fileName"].get<string>() + Dextop_Cover256Suffix;
                cout << "adapted coverURL: " << coverURL << endl;
                assetManager.GetMangaCover(coverURL, resultInfo["relationships"][j]["attributes"]["fileName"].get<string>() + Dextop_Cover256Suffix);
            }
        }
        uiResults.push_back(makeResult);
    }
    slint::invoke_from_event_loop([=](){
        ui->set_results(std::make_shared<slint::VectorModel<SearchResult>>(uiResults));
        ui->set_searchLoading(false);

        int fLimit = searchResults["limit"].get<int>();
        int fTotal = searchResults["total"].get<int>();
        int pageCount = 0;
        if (fTotal > 0)
        {
            div_t pageDiv = std::div(fTotal, fLimit);
            pageCount = pageDiv.quot + (pageDiv.rem > 0 ? 1 : 0);
        }
        ui->set_pageCount(pageCount);

        for (int i = 0; i < resultCount; i++)
        {
            SearchResult newResult;
            newResult.id = ui->get_results()->row_data(i)->id;
            newResult.title = ui->get_results()->row_data(i)->title;
            newResult.description = ui->get_results()->row_data(i)->description;
            newResult.coverFile = ui->get_results()->row_data(i)->coverFile;
            newResult.cover = slint::Image::load_from_path((std::string("assets/images/covers/") + std::string(ui->get_results()->row_data(i)->coverFile) + Dextop_Cover256Suffix).c_str());
            ui->get_results()->set_row_data(i, newResult);
            cout << "image-set row " << i << endl;
            //assetManager.ImageLoad(&getRow, std::string("assets/images/covers/") + std::string(uiResults[i].coverFile));
        }
    });
}

int main(int argc, char **argv)
{
    logger.Initialize();

    //load settings
    std::ifstream settingsFS("settings.json");
    json settings = json::parse(settingsFS);
    settingsFS.close();

    ui->set_authUsername(settings["authUsername"].get<string>().c_str());
    ui->set_authPassword(settings["authPassword"].get<string>().c_str());
    ui->set_authCID(settings["authCID"].get<string>().c_str());
    ui->set_authCSecret(settings["authCSecret"].get<string>().c_str());
    slint::Size<uint32_t> windowSize;
    windowSize.width = settings["mainWindowSizeX"].get<int>();
    windowSize.height = settings["mainWindowSizeY"].get<int>();
    ui->window().set_size(slint::PhysicalSize(windowSize));

    cout << "Settings loaded." << endl;
    
    //UI
    ui->on_openManga([&](slint::SharedString mangaID) {
        InitReader(mangaID, localDexxor);
    });

    ui->on_getUpdates([&](){
        cout << "updates feature not done yet" << endl;
    });
    
    ui->on_doSearch([&]{
        std::thread(DoTitleSearch, ui->get_searchTarget().data(), 10, 0).detach();
    });

    ui->on_changeSearchPage([&](int newPage){
        std::thread(DoTitleSearch, ui->get_currentSearch().data(), 10, newPage - 1).detach();
    });

    ui->on_searchListingChapters([&](slint::SharedString id){
        localDexxor.GetChapters(id.data());
    });

    ui->on_openConsole([&]{
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        ui->set_consoleOpen(true);
    });

    ui->on_doAuthenticate([&]{
        localDexxor.Authenticate(
            ui->get_authUsername().data(),
            ui->get_authPassword().data(),
            ui->get_authCID().data(),
            ui->get_authCSecret().data()
        );
    });

    ui->on_authStatus([&]{
        cout << "Authenticated: " << (localDexxor.Authenticated() == true ? "yes" : "no") << endl;
        if (localDexxor.Authenticated() == true)
        {
            cout << "Access Token: " << localDexxor.accessToken << endl;
            cout << "Refresh Token: " << localDexxor.refreshToken << endl;
            cout << "Access Token age (s): " << localDexxor.AccessTokenAge() << endl;
        }
    });

    ui->on_reAuthenticate([&]{
        localDexxor.RefreshAccessToken();
    });

    ui->run();

    cout << "finishing up" << endl;

    //store settings
    settings["mainWindowSizeX"] = ui->window().size().width;
    settings["mainWindowSizeY"] = ui->window().size().height;
    std::ofstream settingsSave("settings.json", std::ofstream::trunc);
    settingsSave << std::setw(4) << settings << std::endl;
    settingsSave.close();
    cout << "settings saved" << endl;
    return 0;
}
