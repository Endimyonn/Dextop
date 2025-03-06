#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <future>
#include <filesystem>

#include "Dexxor.h"
#include "AssetManager.h"
#include "Logger.h"
#include "DextopPrimaryWindow.h"
#include "Reader.h"

class Dextop
{
	public:
		Logger logger;
		Dexxor localDexxor;
		AssetManager assetManager;
		static slint::ComponentHandle<DextopPrimaryWindow> ui;
};