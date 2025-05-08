#pragma once
#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <shellapi.h>
#endif

#define Dextop_UserAgent "Dextop/1.0"
#define Dextop_Cover256Suffix ".256.jpg"
#define Dextop_LogFile "Dextop.log"

class DextopUtil
{
    public:
        static std::string EncodeURL(const std::string &value) {
            std::ostringstream escaped;
            escaped.fill('0');
            escaped << std::hex;
        
            for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
                std::string::value_type c = (*i);
        
                // Keep alphanumeric and other accepted characters intact
                if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                    escaped << c;
                    continue;
                }
        
                // Any other characters are percent-encoded
                escaped << std::uppercase;
                escaped << '%' << std::setw(2) << int((unsigned char) c);
                escaped << std::nouppercase;
            }
        
            return escaped.str();
        }

        static void OpenURL(const std::string url)
        {
        #ifdef _WIN64
            ShellExecute(0, 0, url.c_str(), 0, 0, SW_SHOW);
        #elif __MACH__
            system((std::string("open ") + url).c_str());
        #elif __linux__
            system((std::string("xdg-open ") + url).c_str());
        #endif
        }
};