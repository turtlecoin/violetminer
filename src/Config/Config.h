// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

#include "Argon2/Constants.h"

#include <string>

namespace Config
{
    class Config
    {
      public:
        Config() {};

        Constants::OptimizationMethod optimizationMethod;
    };

    extern Config config;
} // namespace Config
