// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

#pragma once

#include <optional>
#include <string>
#include <variant>

#include "ArgonVariants/Variants.h"
#include "ExternalLibs/json.hpp"
#include "Utilities/String.h"

struct PoolError
{
    /* The programmatic error code */
    int32_t errorCode;

    /* Human readable error */
    std::string errorMessage;
};

struct Job
{
    /* The mining job to work on, un-hexified */
    std::vector<uint8_t> rawBlob;

    /* Identifier for this job */
    std::string jobID;

    /* The difficulty above which to submit shares */
    uint64_t shareDifficulty;

    uint64_t target;

    /* The height of the block we are attempting to form */
    std::optional<uint64_t> height;

    /* The block major version. Duh. */
    std::optional<uint8_t> blockMajorVersion;

    /* Yeah, you can figure this one out champ. */
    std::optional<uint8_t> blockMinorVersion;

    std::optional<uint8_t> rootMajorVersion;

    std::optional<uint8_t> rootMinorVersion;

    /* Is this job a nicehash job - Set by the pool config */
    bool isNiceHash = false;

    /* What is the algorithm required for this pool - Set by the pool config,
       or by the pool if supported */
    std::string algorithm;

    inline uint32_t *nonce()
    {
        return reinterpret_cast<uint32_t *>(rawBlob.data() + 39);
    }

    inline const uint32_t *nonce() const
    {
        return reinterpret_cast<const uint32_t *>(rawBlob.data() + 39);
    }
};

struct PoolMessage
{
    /* The version of json_rpc the server is using */
    std::string jsonRpc;
};

struct JobMessage : PoolMessage
{
    Job job;

    std::string method;
};

struct ErrorMessage : PoolMessage
{
    /* The same id we sent in the original message */
    std::string ID;

    /* Potential error from the operation */
    PoolError error;
};

struct LoginMessage : PoolMessage
{
    /* The same id we sent in the original message */
    std::string ID;

    /* The ID to use to authenticate us as, well, us */
    std::string loginID;

    /* Whether the operation succeeded */
    std::string status;

    Job job;
};

struct StatusMessage : PoolMessage
{
    /* The same id we sent in the submission/keep alive message */
    std::string ID;

    /* The status type, OK for share accepted, KEEPALIVED for keepalived */
    std::string status;
};

inline void from_json(const nlohmann::json &j, PoolError &p)
{
    p.errorCode = j.at("code").get<int32_t>();
    p.errorMessage = j.at("message").get<std::string>();
}

inline void from_json(const nlohmann::json &j, Job &job)
{
    std::string blob = j.at("blob").get<std::string>();

    job.jobID = j.at("job_id").get<std::string>();

    if (blob.size() % 2 != 0)
    {
        throw std::invalid_argument("Blob length must be multiple of 2!");
    }

    if (blob.size() < 76)
    {
        throw std::invalid_argument("Blob length must be at least 76 bytes!");
    }

    job.rawBlob = Utilities::fromHex(blob);

    const std::string target = j.at("target").get<std::string>();

    if (target.length() <= 8)
    {
        uint32_t tmp = 0;

        char str[8];

        std::memcpy(str, target.data(), target.size());

        Utilities::fromHex(str, 8, reinterpret_cast<unsigned char *>(&tmp));

        job.target = 0xFFFFFFFFFFFFFFFFULL / (0xFFFFFFFFULL / static_cast<uint64_t>(tmp));
    }
    else if (target.length() <= 16)
    {
        job.target = 0;

        char str[16];

        std::memcpy(str, target.data(), target.size());

        Utilities::fromHex(str, 16, reinterpret_cast<unsigned char *>(&job.target));
    }
    else
    {
        throw std::invalid_argument("Target cannot be longer than 16 bytes!");
    }

    job.shareDifficulty = 0xFFFFFFFFFFFFFFFFULL / job.target;

    if (j.find("height") != j.end())
    {
        job.height = j.at("height").get<uint64_t>();
    }

    if (j.find("blockMajorVersion") != j.end())
    {
        job.blockMajorVersion = j.at("blockMajorVersion").get<uint8_t>();
    }

    if (j.find("blockMinorVersion") != j.end())
    {
        job.blockMinorVersion = j.at("blockMinorVersion").get<uint8_t>();
    }

    if (j.find("rootMajorVersion") != j.end())
    {
        job.rootMajorVersion = j.at("rootMajorVersion").get<uint8_t>();
    }

    if (j.find("rootMinorVersion") != j.end())
    {
        job.rootMinorVersion = j.at("rootMinorVersion").get<uint8_t>();
    }

    if (j.find("algo") != j.end())
    {
        const auto poolAlgorithm = j.at("algo").get<std::string>();

        if (ArgonVariant::isSupportedAlgorithm(poolAlgorithm))
        {
            job.algorithm = poolAlgorithm;
        }
    }
}

inline void from_json(const nlohmann::json &j, PoolMessage &p)
{
    p.jsonRpc = j.at("jsonrpc").get<std::string>();
}

inline void from_json(const nlohmann::json &j, LoginMessage &l)
{
    from_json(j, static_cast<PoolMessage &>(l));

    if (j.find("id") != j.end())
    {
        if (j.at("id").is_string())
        {
            l.ID = j.at("id").get<std::string>();
        }
        else
        {
            l.ID = std::to_string(j.at("id").get<uint32_t>());
        }
    }

    const auto result = j.at("result");

    l.loginID = result.at("id").get<std::string>();

    if (result.find("status") != result.end())
    {
        l.status = result.at("status").get<std::string>();
    }

    l.job = result.at("job").get<Job>();
}

inline void from_json(const nlohmann::json &j, JobMessage &job)
{
    from_json(j, static_cast<PoolMessage &>(job));

    job.method = j.at("method").get<std::string>();
    job.job = j.at("params").get<Job>();
}

inline void from_json(const nlohmann::json &j, ErrorMessage &e)
{
    from_json(j, static_cast<PoolMessage &>(e));

    if (j.find("id") != j.end())
    {
        if (j.at("id").is_string())
        {
            e.ID = j.at("id").get<std::string>();
        }
        else
        {
            e.ID = std::to_string(j.at("id").get<uint32_t>());
        }
    }

    e.error = j.at("error").get<PoolError>();
}

inline void from_json(const nlohmann::json &j, StatusMessage &s)
{
    from_json(j, static_cast<PoolMessage &>(s));

    if (j.find("id") != j.end())
    {
        if (j.at("id").is_string())
        {
            s.ID = j.at("id").get<std::string>();
        }
        else
        {
            s.ID = std::to_string(j.at("id").get<uint32_t>());
        }
    }

    const auto result = j.at("result");

    s.status = result.at("status").get<std::string>();
}

std::variant<
    JobMessage,
    ErrorMessage,
    LoginMessage,
    StatusMessage
> parsePoolMessage(const std::string &message);
