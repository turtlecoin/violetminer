// Copyright (c) 2018, The TurtleCoin Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include "Utilities/Console.h"

#include <iomanip>
#include <ostream>
#include <string>

template<typename T> class ColouredMsg
{
  public:
    ColouredMsg(const T msg, const Console::Color colour): msg(msg), colour(colour) {}

    ColouredMsg(const T msg, const int padding, const Console::Color colour):
        msg(msg),
        colour(colour),
        padding(padding),
        pad(true)
    {
    }

    /* Set the text colour, write the message, then reset. We use a class
       as it seems the only way to have a valid << operator. We need this
       so we can nicely do something like:

       std::cout << "Hello " << GreenMsg("user") << std::endl;

       Without having to write:

       std::cout << "Hello ";
       GreenMsg("user");
       std::cout << std::endl; */

    friend std::ostream &operator<<(std::ostream &os, const ColouredMsg &m)
    {
        Console::setTextColor(m.colour);

        if (m.pad)
        {
            os << std::left << std::setw(m.padding) << m.msg;
        }
        else
        {
            os << m.msg;
        }

        Console::setTextColor(Console::Color::Default);
        return os;
    }

  protected:
    /* Can be any class that supports the << operator */
    T msg;

    /* The colour to use */
    const Console::Color colour;

    /* The amount to pad the message to */
    const int padding = 0;

    /* If we should pad the message */
    const bool pad = false;
};

template<typename T> class SuccessMsg : public ColouredMsg<T>
{
  public:
    explicit SuccessMsg(T msg): ColouredMsg<T>(msg, Console::Color::Green) {}

    explicit SuccessMsg(T msg, int padding): ColouredMsg<T>(msg, padding, Console::Color::Green) {}
};

template<typename T> class InformationMsg : public ColouredMsg<T>
{
  public:
    explicit InformationMsg(T msg): ColouredMsg<T>(msg, Console::Color::BrightYellow) {}

    explicit InformationMsg(T msg, int padding): ColouredMsg<T>(msg, padding, Console::Color::BrightYellow) {}
};

template<typename T> class WarningMsg : public ColouredMsg<T>
{
  public:
    explicit WarningMsg(T msg): ColouredMsg<T>(msg, Console::Color::BrightRed) {}

    explicit WarningMsg(T msg, int padding): ColouredMsg<T>(msg, padding, Console::Color::BrightRed) {}
};

template<typename T> class WhiteMsg : public ColouredMsg<T>
{
  public:
    explicit WhiteMsg(T msg): ColouredMsg<T>(msg, Console::Color::BrightWhite) {}

    explicit WhiteMsg(T msg, int padding): ColouredMsg<T>(msg, padding, Console::Color::BrightWhite) {}
};

template<typename T> class PurpleMsg : public ColouredMsg<T>
{
  public:
    explicit PurpleMsg(T msg): ColouredMsg<T>(msg, Console::Color::BrightMagenta) {}

    explicit PurpleMsg(T msg, int padding): ColouredMsg<T>(msg, padding, Console::Color::BrightMagenta) {}
};
