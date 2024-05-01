// Dextop.cpp : Defines the entry point for the application.
//

#include "Dextop.h"
#include "Logger.h"

using namespace std;
using json = nlohmann::json;

Dexxor localDexxor;



int setup(int argc, char *argv[])
{
	localDexxor.Initialize();
	cout << "Started." << endl;

	

	
	

	cout << "Enter search query: ";
	std::string searchQuery;
	cin >> searchQuery;
	auto doSearch = std::async(&Dexxor::Search, &localDexxor, searchQuery.c_str());
	json searchResults = doSearch.get();
	int resultCount = searchResults["data"].size();

	cout << resultCount << " search results:" << endl;
	for (int i = 0; i < resultCount; i++)
	{
		json resultInfo = searchResults["data"][i];
		std::string name = (std::string)resultInfo["attributes"]["title"].begin().value();
		cout << (i + 1) << ": " << name << endl;
		
		//lang
		bool containsEn = false;
		for (int k = 0; k < resultInfo["attributes"]["availableTranslatedLanguages"].size(); k++)
		{
			if (resultInfo["attributes"]["availableTranslatedLanguages"].at(k) == "en")
			{
				containsEn = true;
			}
		}
		std::string enAvailability = (containsEn == true) ? "Yes" : "No";
		cout << "\tAvailable in English: " << enAvailability << endl;

		//alternate titles
		if (resultInfo["attributes"]["altTitles"].size() > 0)
		{
			cout << "\tAlternate titles:" << endl;
			for (int j = 0; j < resultInfo["attributes"]["altTitles"].size(); j++)
			{
				cout << "\t\t"
					<< "(" << (std::string)resultInfo["attributes"]["altTitles"][j].begin().key() << ") "
					<< (std::string)resultInfo["attributes"]["altTitles"][j].begin().value() << endl;
			}
		}

		//description
		if (resultInfo["attributes"]["description"].size() > 0)
		{
			cout << "\tDescription:" << endl;
			cout << "\t\t" << (std::string)resultInfo["attributes"]["description"].begin().value() << endl;
		}
	}

	return 0;
}


