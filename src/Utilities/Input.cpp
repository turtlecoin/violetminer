// Copyright (c) 2019, The TurtleCoin Developers
//
// Please see the included LICENSE file for more information.

////////////////////////////
#include "Utilities/Input.h"
////////////////////////////

#include "Utilities/ColouredMsg.h"
#include "Utilities/String.h"

#include <iostream>

namespace Utilities
{
    bool confirm(const std::string &msg)
    {
        return confirm(msg, true);
    }

    /* defaultToYes = what value we return on hitting enter, i.e. the "expected"
       workflow */
    bool confirm(const std::string &msg, const bool defaultToYes)
    {
        /* In unix programs, the upper case letter indicates the default, for
           example when you hit enter */
        const std::string prompt = defaultToYes ? " (Y/n): " : " (y/N): ";

        while (true)
        {
            std::cout << InformationMsg(msg + prompt);

            std::string answer;
            std::getline(std::cin, answer);

            const char c = ::tolower(answer[0]);

            switch (c)
            {
                /* Lets people spam enter / choose default value */
                case '\0':
                    return defaultToYes;
                case 'y':
                    return true;
                case 'n':
                    return false;
            }

            std::cout << WarningMsg("Bad input: ") << InformationMsg(answer)
                      << WarningMsg(" - please enter either Y or N.") << std::endl;
        }
    }

    bool parseAddressFromString(std::string &host, uint16_t &port, std::string address)
    {
        /* Lets users enter url's instead of host:port */
        address = Utilities::removePrefix(address, "https://");
        address = Utilities::removePrefix(address, "http://");

        std::vector<std::string> parts = Utilities::split(address, ':');

        if (parts.size() != 2)
        {
            return false;
        }

        try
        {
            host = parts.at(0);
            port = std::stoi(parts.at(1));
            return true;
        }
        catch (const std::invalid_argument &)
        {
            return false;
        }
    }
} // namespace Utilities
