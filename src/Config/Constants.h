// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

#pragma once

#include "Argon2/Constants.h"

namespace Constants
{
    /* How many times to attempt logging into a pool before giving up and trying
       the next */
    const int MAX_LOGIN_ATTEMPTS = 3;

    /* How long to wait before trying again after a failed login, in milliseconds */
    const int POOL_LOGIN_RETRY_INTERVAL = 5000;

    /* The percentage of time to spend mining for the miner developer */
    const float DEV_FEE_PERCENT = 0;

    /* Program version */
    const std::string VERSION_NUMBER = "0.2.2";

    const std::string VERSION = "v" + VERSION_NUMBER + " with Argon2 version " + std::to_string(CURRENT_ARGON_VERSION);

    /* Name of config file to look for */
    const std::string CONFIG_FILE_NAME = "config.json";
}
