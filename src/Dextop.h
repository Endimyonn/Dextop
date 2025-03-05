#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <future>

#include "Dexxor.h"
#include "AssetManager.h"
#include "Logger.h"
#include "DextopPrimaryWindow.h"

class Dextop
{
	public:
		Dexxor localDexxor;
		AssetManager assetManager;
		static Logger logger;
		static slint::ComponentHandle<DextopPrimaryWindow> ui;
};