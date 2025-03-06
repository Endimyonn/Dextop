#include "Dextop.h"
#include "Dextop_Defs.h"
#include <windows.h>
#include <filesystem>

using namespace std;
using json = nlohmann::json;

Logger logger(std::string("Dextop.log"));
Dexxor localDexxor;
AssetManager assetManager;
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
    dtlog << "Search for \"" << title << "\": found " << resultCount << " (limit: " << limit << ", page: " << page << ", total: " << searchResults["total"] << ")" << endl;

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
                dtlog << "adapted coverURL: " << coverURL << endl;
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
            dtlog << "image-set row " << i << endl;
            //assetManager.ImageLoad(&getRow, std::string("assets/images/covers/") + std::string(uiResults[i].coverFile));
        }
    });
}

nlohmann::json GetSettings()
{
    if (std::filesystem::exists(std::filesystem::path("settings.json")))
    {   //the file exists, load it
        std::ifstream loadSettings("settings.json");
        json settings = json::parse(loadSettings);
        loadSettings.close();

        //validate the contents
        if (!settings.contains("authCID") || !settings["authCID"].is_string())
        {
            dtlog << "Setting \"authCID\" missing or invalid. Resetting." << endl;
            settings["authCID"] = "";
        }
        if (!settings.contains("authCSecret") || !settings["authCSecret"].is_string())
        {
            dtlog << "Setting \"authCSecret\" missing or invalid. Resetting." << endl;
            settings["authCSecret"] = "";
        }
        if (!settings.contains("authPassword") || !settings["authPassword"].is_string())
        {
            dtlog << "Setting \"authPassword\" missing or invalid. Resetting." << endl;
            settings["authPassword"] = "";
        }
        if (!settings.contains("authUsername") || !settings["authUsername"].is_string())
        {
            dtlog << "Setting \"authUsername\" missing or invalid. Resetting." << endl;
            settings["authUsername"] = "";
        }
        if (!settings.contains("mainWindowWidth") || !settings["mainWindowWidth"].is_number_integer())
        {
            dtlog << "Setting \"mainWindowWidth\" missing or invalid. Resetting." << endl;
            settings["mainWindowWidth"] = 480;
        }
        if (!settings.contains("mainWindowHeight") || !settings["mainWindowHeight"].is_number_integer())
        {
            dtlog << "Setting \"mainWindowHeight\" missing or invalid. Resetting." << endl;
            settings["mainWindowHeight"] = 320;
        }

        return settings;
    }
    
    //create new settings file
    dtlog << "Settings not found, creating...";
    json makeSettings = {
        {"authCID", ""},
        {"authCSecret", ""},
        {"authPassword", ""},
        {"authUsername", ""},
        {"mainWindowWidth", 480},
        {"mainWindowHeight", 320}
    };
    std::ofstream saveSettings("settings.json", std::ofstream::trunc);
    saveSettings << std::setw(4) << makeSettings << std::endl;
    saveSettings.close();
    dtlog << " done." << endl;
    return makeSettings;
}

int main(int argc, char **argv)
{
    //load settings
    json settings = GetSettings();

    ui->set_authUsername(settings["authUsername"].get<string>().c_str());
    ui->set_authPassword(settings["authPassword"].get<string>().c_str());
    ui->set_authCID(settings["authCID"].get<string>().c_str());
    ui->set_authCSecret(settings["authCSecret"].get<string>().c_str());
    slint::Size<uint32_t> windowSize;
    windowSize.width = settings["mainWindowWidth"].get<int>();
    windowSize.height = settings["mainWindowHeight"].get<int>();
    ui->window().set_size(slint::PhysicalSize(windowSize));

    dtlog << "Settings loaded." << endl;
    
    //UI
    ui->set_platform(
#ifdef _WIN64
        "windows"
#elif __MACH__
        "macos"
#elif __linux__
        "linux"
#endif
    );

    ui->on_openManga([&](slint::SharedString mangaID) {
        InitReader(mangaID, localDexxor);
    });

    ui->on_getUpdates([&](){
        dtlog << "updates feature not done yet" << endl;
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
        FILE* outDummy;
        freopen_s(&outDummy, "CONOUT$", "w", stdout);
        freopen_s(&outDummy, "CONOUT$", "w", stderr);
        std::cout.clear();
        std::clog.clear();
        std::cerr.clear();
        ui->set_consoleOpen(true);
    });

    ui->on_authenticate([&]{
        localDexxor.Authenticate(
            ui->get_authUsername().data(),
            ui->get_authPassword().data(),
            ui->get_authCID().data(),
            ui->get_authCSecret().data()
        );
    });

    ui->on_checkAuthStatus([&]{
        dtlog << "Authenticated: " << (localDexxor.Authenticated() == true ? "yes" : "no") << endl;
        if (localDexxor.Authenticated() == true)
        {
            dtlog << "Access Token: " << localDexxor.accessToken << endl;
            dtlog << "Refresh Token: " << localDexxor.refreshToken << endl;
            dtlog << "Access Token age (s): " << localDexxor.AccessTokenAge() << endl;
        }
    });

    ui->on_refreshAccessToken([&]{
        localDexxor.RefreshAccessToken();
    });

    ui->run();

    dtlog << "Shutting down..." << endl;

    //store settings
    settings["mainWindowWidth"] = ui->window().size().width;
    settings["mainWindowHeight"] = ui->window().size().height;
    std::ofstream settingsSave("settings.json", std::ofstream::trunc);
    settingsSave << std::setw(4) << settings << std::endl;
    settingsSave.close();
    dtlog << "settings saved" << endl;
    return 0;
}
