// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

#pragma once

#include <functional>
#include <iostream>
#include <string>

#include "ArgonVariants/Variants.h"
#include "Config/Constants.h"
#include "ExternalLibs/json.hpp"
#include "Types/IHashingAlgorithm.h"
#include "Utilities/ColouredMsg.h"

struct Pool
{
    /* Host of the pool */
    /* Required */
    std::string host;

    /* Port of the pool */
    /* Required */
    uint16_t port;

    /* Username to login with */
    /* Required */
    std::string username;

    /* Optional password to login with */
    std::string password;

    /* Optional rig identifier */
    std::string rigID;

    /* The mining algorithm to use with this pool */
    /* Required */
    std::string algorithm;

    /* Custom user agent */
    std::string agent;

    /* The string we use to authenticate us once we have logged in */
    std::string loginID;

    /* Whether to use nicehash style nonces */
    bool niceHash = false;

    /* The priority of this pool in the list of pools */
    size_t priority = 0;

    /* Does this pool require SSL for connecting */
    bool ssl = false;

    /* Disable automatically selecting the correct algorithm from the pool job data.
     * This may be desired if the pool is returning the incorrect value. */
    bool disableAutoAlgoSelect = false;

    std::string getAgent() const
    {
        return agent == "" ? "violetminer/" + Constants::VERSION_NUMBER : agent;
    }

    bool operator==(const Pool& other) const
    {
        return host         == other.host
            && port         == other.port
            && username     == other.username
            && password     == other.password
            && rigID        == other.rigID
            && algorithm    == other.algorithm
            && agent        == other.agent
            && loginID      == other.loginID
            && ssl          == other.ssl;
    }

    bool operator!=(const Pool& other) const
    {
        return !operator==(other);
    }
};

inline void to_json(nlohmann::json &j, const Pool &pool)
{
    j = {
        {"host", pool.host},
        {"port", pool.port},
        {"username", pool.username},
        {"password", pool.password},
        {"rigID", pool.rigID},
        {"algorithm", pool.algorithm},
        {"agent", pool.agent},
        {"niceHash", pool.niceHash},
        {"priority", pool.priority},
        {"ssl", pool.ssl},
        {"disableAutoAlgoSelect", pool.disableAutoAlgoSelect},
    };
}

inline void from_json(const nlohmann::json &j, Pool &pool)
{
    pool.host = j.at("host").get<std::string>();
    pool.port = j.at("port").get<uint16_t>();
    pool.username = j.at("username").get<std::string>();

    if (j.find("password") != j.end())
    {
        pool.password = j.at("password").get<std::string>();
    }

    if (j.find("rigID") != j.end())
    {
        pool.rigID = j.at("rigID").get<std::string>();
    }

    pool.algorithm = j.at("algorithm").get<std::string>();

    try
    {
        ArgonVariant::algorithmNameToCanonical(pool.algorithm);
    }
    catch (const std::exception &)
    {
        throw std::invalid_argument("Algorithm \"" + pool.algorithm + "\" is not a known algorithm!");
    }

    if (j.find("agent") != j.end())
    {
        pool.agent = j.at("agent").get<std::string>();
    }

    if (j.find("niceHash") != j.end())
    {
        pool.niceHash = j.at("niceHash").get<bool>();
    }

    if (j.find("priority") != j.end())
    {
        pool.priority = j.at("priority").get<size_t>();
    }

    if (j.find("ssl") != j.end())
    {
        pool.ssl = j.at("ssl").get<bool>();

        #if !defined(SOCKETWRAPPER_OPENSSL_SUPPORT)
        if (pool.ssl)
        {
            std::cout << WarningMsg("Warning: SSL is enabled for pool " + pool.host + ", but miner was not compiled with SSL support!") << std::endl
                      << WarningMsg("If this pool is indeed SSL only, connecting will fail. Try another port or compile with SSL support.") << std::endl;
        }
        #endif
    }

    if (j.find("disableAutoAlgoSelect") != j.end())
    {
        pool.disableAutoAlgoSelect = j.at("disableAutoAlgoSelect").get<bool>();
    }
}
