/// Primary controller of the program, responsible for bootstrapping everything else.
#pragma once

#include <iostream>
#include <fstream>
#include <future>
#include <filesystem>

#include "ThreadPool.h"
#include "Dexxor.h"
#include "AssetManager.h"
#include "Logger.h"
#include "DextopPrimaryWindow.h"

class Dextop
{
	private:
		inline static Dextop* instance = nullptr;

		nlohmann::json LoadSettings();
	public:
		Dextop()
		{
			instance = this;

			dexxor.Initialize();
		}

		static Dextop* GetInstance();

		ThreadPoolContainer threadPool;
		Logger logger = Logger(std::string("Dextop.log"));
		nlohmann::json settings = LoadSettings();
		Dexxor dexxor;
		AssetManager assetManager;
		slint::ComponentHandle<DextopPrimaryWindow> ui = DextopPrimaryWindow::create();

		void Run();
};