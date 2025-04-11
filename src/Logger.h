#pragma once

#include <iostream>
#include <fstream>

class Logger
{
	private:
		std::ofstream stream;
	public:
		inline static Logger* Log = nullptr;
		Logger(const std::string& path)
		{
			stream = std::ofstream(path, std::ofstream::trunc);
			Log = this;
		}

		Logger()
		{
			stream = std::ofstream("program.log", std::ofstream::trunc);
			Log = this;
		}

		Logger& operator=(const Logger&)
		{
			stream = std::ofstream("program.log", std::ofstream::trunc);
			Log = this;
		}

		~Logger()
		{
			stream.close();
		}

		template<class T>
		Logger& operator<<(const T& what)
		{
			stream << what;
			stream.flush();
			std::cout << what;
			return *this;
		}

		Logger& operator<<(std::ostream& (*os)(std::ostream&))
		{
			stream << std::endl;
			stream.flush();
			std::cout << std::endl;
			return *this;
		}
};
#define dtlog *Logger::Log