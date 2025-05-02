#include "Dextop.h"
#include "Dextop_Defs.h"
#include "uicon/MainUpdates.h"
#include "uicon/MainSearch.h"
#include "uicon/Reader.h"
#include <windows.h>
#include <filesystem>

using namespace std;

static Dextop dextop;

nlohmann::json Dextop::LoadSettings()
{
    if (std::filesystem::exists(std::filesystem::path("settings.json")))
    {   //the file exists, load it
        std::ifstream loadSettings("settings.json");
        nlohmann::json settings = nlohmann::json::parse(loadSettings);
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
        if (!settings.contains("authLastAccessToken") || !settings["authLastAccessToken"].is_string())
        {
            dtlog << "Setting \"authLastAccessToken\" missing or invalid. Resetting." << endl;
            settings["authLastAccessToken"] = "";
        }
        if (!settings.contains("authLastRefreshToken") || !settings["authLastRefreshToken"].is_string())
        {
            dtlog << "Setting \"authLastRefreshToken\" missing or invalid. Resetting." << endl;
            settings["authLastRefreshToken"] = "";
        }
        if (!settings.contains("authLastSuccessTime") || !settings["authLastSuccessTime"].is_number())
        {
            dtlog << "Setting \"authLastSuccessTime\" missing or invalid. Resetting." << endl;
            settings["authLastSuccessTime"] = 0;
        }
        if (!settings.contains("authLastSuccessCID") || !settings["authLastSuccessCID"].is_string())
        {
            dtlog << "Setting \"authLastSuccessCID\" missing or invalid. Resetting." << endl;
            settings["authLastSuccessCID"] = "";
        }
        if (!settings.contains("authLastSuccessCSecret") || !settings["authLastSuccessCSecret"].is_string())
        {
            dtlog << "Setting \"authLastSuccessCSecret\" missing or invalid. Resetting." << endl;
            settings["authLastSuccessCSecret"] = "";
        }
        if (!settings.contains("mainWindowWidth") || !settings["mainWindowWidth"].is_number())
        {
            dtlog << "Setting \"mainWindowWidth\" missing or invalid. Resetting." << endl;
            settings["mainWindowWidth"] = 480;
        }
        if (!settings.contains("mainWindowHeight") || !settings["mainWindowHeight"].is_number())
        {
            dtlog << "Setting \"mainWindowHeight\" missing or invalid. Resetting." << endl;
            settings["mainWindowHeight"] = 320;
        }

        return settings;
    }
    
    //create new settings file
    dtlog << "Settings not found, creating...";
    nlohmann::json makeSettings = {
        {"authCID", ""},
        {"authCSecret", ""},
        {"authPassword", ""},
        {"authUsername", ""},
        {"authLastAccessToken", ""},
        {"authLastRefreshToken", ""},
        {"authLastSuccessTime", 0},
        {"authLastSuccessCID", ""},
        {"authLastSuccessCSecret", ""},
        {"mainWindowWidth", 480},
        {"mainWindowHeight", 320}
    };
    std::ofstream saveSettings("settings.json", std::ofstream::trunc);
    saveSettings << std::setw(4) << makeSettings << std::endl;
    saveSettings.close();
    dtlog << " done." << endl;
    return makeSettings;
}

Dextop* Dextop::GetInstance()
{
    return instance;
}

void Dextop::Run()
{
    //load settings
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

    std::vector<DTReaderController*> readerWindows;

    ui->on_openManga([&readerWindows](slint::SharedString mangaJson) {
        DTReaderController* readerController = new DTReaderController(nlohmann::json::parse(mangaJson.data()));
        readerWindows.push_back(readerController);
    });

    ui->on_openChapter([&readerWindows](slint::SharedString chapterID, slint::SharedString mangaID)
    {
        DTReaderController* readerController = new DTReaderController(chapterID.data(), mangaID.data());
        readerWindows.push_back(readerController);
    });

    ui->on_getUpdates([&](){
        dtThreadPool.detach_task([]()
        {
            DTMainUpdatesController::DoRefreshUpdates();
        });
    });
    
    ui->on_doSearch([&]{
        const char* getTarget = ui->get_searchTarget().data();
        dtThreadPool.detach_task([=]
        {
            DTMainSearchController::DoTitleSearch(getTarget, 10, 0);
        });
    });

    ui->on_changeSearchPage([&](int newPage){
        const char* getTarget = ui->get_searchTarget().data();
        dtThreadPool.detach_task([=]
        {
            DTMainSearchController::DoTitleSearch(getTarget, 10, newPage - 1);
        });
    });

    ui->on_searchListingChapters([&](slint::SharedString id){
        dexxor.GetChapters(id.data());
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
        dexxor.Authenticate(
            ui->get_authUsername().data(),
            ui->get_authPassword().data(),
            ui->get_authCID().data(),
            ui->get_authCSecret().data()
        );
    });

    ui->on_checkAuthStatus([&]{
        dtlog << "Authenticated: " << (dexxor.Authenticated() == true ? "yes" : "no") << endl;
        if (dexxor.Authenticated() == true)
        {
            dtlog << "Access Token: " << dexxor.accessToken << endl;
            dtlog << "Refresh Token: " << dexxor.refreshToken << endl;
            dtlog << "Access Token age (s): " << dexxor.AccessTokenAge() << endl;
        }
    });

    ui->on_refreshAccessToken([&]{
        dexxor.RefreshAccessToken();
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
}

int main(int argc, char **argv)
{
    dextop.Run();
    return 0;
}
