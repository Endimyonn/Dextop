/// Primary controller of the program, responsible for bootstrapping everything else.
#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <future>
#include <filesystem>

#include "ThreadPool.h"
#include "Dexxor.h"
#include "AssetManager.h"
#include "Logger.h"
#include "DextopPrimaryWindow.h"
#include "Reader.h"

class Dextop
{
	public:
		ThreadPoolContainer threadPool;
		Logger logger;
		Dexxor localDexxor;
		AssetManager assetManager;
		static slint::ComponentHandle<DextopPrimaryWindow> ui;
};