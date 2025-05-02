#pragma once

#include "../Dextop.h"
#include <chrono>



class DTMainUpdatesController
{
    private:
        //mangadex returns chapter timestamps as "YYYY-MM-DDThh:mm:ss+oo:oo", where T is useless and o is offset
        static std::string FormatAge(std::string timestamp)
        {
            timestamp = timestamp.substr(0, 10) + " " + timestamp.substr(11, 19);
            std::istringstream tsStream{timestamp};
            std::chrono::utc_clock::time_point convTimestamp;
            tsStream >> std::chrono::parse("%F %T", convTimestamp);
            std::chrono::time_point now = std::chrono::utc_clock::now();
            
            if (auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - convTimestamp).count(); diff < 60)
            {
                return std::to_string(diff) + (diff != 1 ? " seconds" : " second");
            }
            else if (auto diff = std::chrono::duration_cast<std::chrono::minutes>(now - convTimestamp).count(); diff < 60)
            {
                return std::to_string(diff) + (diff != 1 ? " minutes" : " minute");
            }
            else if (auto diff = std::chrono::duration_cast<std::chrono::hours>(now - convTimestamp).count(); diff < 24)
            {
                return std::to_string(diff) + (diff != 1 ? " hours" : " hour");
            }
            else if (auto diff = std::chrono::duration_cast<std::chrono::days>(now - convTimestamp).count(); diff < 365)
            {
                return std::to_string(diff) + (diff != 1 ? " days" : " day");
            }
            else
            {
                return std::to_string(std::chrono::duration_cast<std::chrono::years>(now - convTimestamp).count()) + (diff != 1 ? " years" : " year");
            }
        }

    public:
        static void DoRefreshUpdates()
        {
            Dextop* dextop = Dextop::GetInstance();
            slint::invoke_from_event_loop([=](){
                dextop->ui->set_updatesRefreshButtonEnabled(false);
                dextop->ui->set_updatesGroups(std::make_shared<slint::VectorModel<UpdatesGroup>>());
                dextop->ui->set_updatesBGText(slint::SharedString("Loading..."));
            });

            nlohmann::json updatesJson = dextop->dexxor.GetUpdates();

            std::vector<UpdatesGroup> uGroups;
            std::vector<std::vector<UpdatesItem>> uGroupItems;
            std::vector<std::string> createdGroups;
            for (int i = 0; i < updatesJson["data"].size(); i++)
            {
                nlohmann::json itemJson = updatesJson["data"].at(i);

                //isolate relationships (of which the relationship index shifts)
                nlohmann::json relMangaJson, relGroupJson, relUserJson;
                for (int k = 0; k < itemJson["relationships"].size(); k++)
                {
                    std::string getType = itemJson["relationships"].at(k)["type"].get<std::string>();
                    if (getType == "manga")
                    {
                        relMangaJson = itemJson["relationships"].at(k);
                    }
                    else if (getType == "scanlation_group")
                    {
                        relGroupJson = itemJson["relationships"].at(k);
                    }
                    else if (getType == "user")
                    {
                        relUserJson = itemJson["relationships"].at(k);
                    }
                }

                //determine which manga group this chapter belongs to
                std::string mangaID = relMangaJson["id"].get<std::string>();
                short groupIndex = -1;
                for (int j = 0; j < createdGroups.size(); j++)
                {
                    if (createdGroups[j] == mangaID)
                    {
                        groupIndex = j;
                    }
                }
                if (groupIndex == -1) //no group has been created for the manga this chapter belongs to; make one
                {
                    UpdatesGroup newGroup;
                    newGroup.id = relMangaJson["id"].get<std::string>();
                    newGroup.title = relMangaJson["attributes"]["title"].begin().value();
                    newGroup.chapterCount = 0;
                    uGroups.push_back(newGroup);
                    createdGroups.push_back(mangaID);
                    groupIndex = uGroups.size() - 1;
                }

                //create the update item
                UpdatesItem newItem;
                newItem.id = itemJson["id"].get<std::string>();
                newItem.title = std::string("Unknown Chapter");
                if (itemJson["attributes"]["chapter"].is_null() == false)
                {
                    newItem.title = ("Ch. " + std::string(itemJson["attributes"]["chapter"].begin().value()));
                }
                if (itemJson["attributes"]["title"].is_null() == false && itemJson["attributes"]["title"].begin().value() != "")
                {
                    newItem.title = newItem.title + ": " + itemJson["attributes"]["title"].begin().value();
                }
                newItem.groupName = "No Group";
                if (relGroupJson != nullptr)
                {
                    newItem.groupName = relGroupJson["attributes"]["name"].get<std::string>();
                }
                newItem.age = FormatAge(itemJson["attributes"]["publishAt"].get<std::string>());
                newItem.uploader = relUserJson["attributes"]["username"].get<std::string>();
                newItem.pageCount = itemJson["attributes"]["pages"].get<int>();

                //add the chapter to the group items collection
                if (uGroupItems.size() == groupIndex)
                {
                    uGroupItems.push_back(std::vector<UpdatesItem>());
                }
                uGroupItems[groupIndex].push_back(newItem);
            }

            for (int i = 0; i < uGroups.size(); i++)
            {
                uGroups[i].chapterCount = uGroupItems[i].size();
                uGroups[i].chapters = std::make_shared<slint::VectorModel<UpdatesItem>>(uGroupItems[i]);
            }

            slint::invoke_from_event_loop([=](){
                dextop->ui->set_updatesGroups(std::make_shared<slint::VectorModel<UpdatesGroup>>(uGroups));
                dextop->ui->set_updatesBGText(slint::SharedString(""));
                dextop->ui->set_updatesRefreshButtonEnabled(true);
            });
        }
};