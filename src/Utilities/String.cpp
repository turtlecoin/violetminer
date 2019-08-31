// Copyright (c) 2019, The TurtleCoin Developers
//
// Please see the included LICENSE file for more information.

/////////////////////////////
#include "Utilities/String.h"
/////////////////////////////

#include <algorithm>
#include <sstream>

namespace Utilities
{
    /* Erases all instances of c from the string. E.g. 2,000,000 becomes 2000000 */
    void removeCharFromString(std::string &str, const char c)
    {
        str.erase(std::remove(str.begin(), str.end(), c), str.end());
    }

    /* Trims any whitespace from left and right */
    void trim(std::string &str)
    {
        rightTrim(str);
        leftTrim(str);
    }

    void leftTrim(std::string &str)
    {
        std::string whitespace = " \t\n\r\f\v";

        str.erase(0, str.find_first_not_of(whitespace));
    }

    void rightTrim(std::string &str)
    {
        std::string whitespace = " \t\n\r\f\v";

        str.erase(str.find_last_not_of(whitespace) + 1);
    }

    /* Checks if str begins with substring */
    bool startsWith(const std::string &str, const std::string &substring)
    {
        return str.rfind(substring, 0) == 0;
    }

    std::vector<std::string> split(const std::string &str, char delimiter = ' ')
    {
        std::vector<std::string> cont;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter))
        {
            cont.push_back(token);
        }

        return cont;
    }

    std::string removePrefix(const std::string &str, const std::string &prefix)
    {
        const size_t removePos = str.rfind(prefix, 0);

        if (removePos == 0)
        {
            /* erase is in place */
            std::string copy = str;

            copy.erase(0, prefix.length());

            return copy;
        }

        return str;
    }

    int char2int(char input)
    {
        if (input >= '0' && input <= '9')
        {
            return input - '0';
        }

        if (input >= 'A' && input <= 'F')
        {
            return input - 'A' + 10;
        }

        if (input >= 'a' && input <= 'f')
        {
            return input - 'a' + 10;
        }

        return -1;
    }

    void toHex(const unsigned char *input, char *output, size_t inputLength)
    {
        char hexval[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

        for (int i = 0; i < inputLength; i++)
        {
            output[i * 2] = hexval[((input[i] >> 4) & 0xF)];
            output[(i * 2) + 1] = hexval[(input[i]) & 0x0F];
        }
    }

    std::string toHex(const uint32_t &input)
    {
        std::string result;
        result.resize(sizeof(input) * 2);

        toHex(reinterpret_cast<const unsigned char *>(&input), result.data(), sizeof(input));

        return result;
    }

    std::string toHex(const std::vector<uint8_t> &input)
    {
        std::string result;
        result.resize(input.size() * 2);

        toHex(input.data(), result.data(), input.size());

        return result;
    }

    std::vector<uint8_t> fromHex(const std::string &input)
    {
        const size_t outputLength = input.size() / 2;

        std::vector<uint8_t> output(outputLength);

        for (int i = 0; i < outputLength; i++)
        {
            output[i] = char2int(input[i * 2]) * 16 + char2int(input[(i * 2) + 1]);
        }

        return output;
    }

    void fromHex(const char *input, size_t inputLength, unsigned char *output)
    {
        for (int i = 0; i < inputLength / 2; i++)
        {
            output[i] = char2int(input[i * 2]) * 16 + char2int(input[(i * 2) + 1]);
        }
    }


} // namespace Utilities
