// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

#pragma once

#include "Miner/GetConfig.h"
#include "Types/PerformanceStats.h"
#include "Types/PoolMessage.h"

class IBackend
{
  public:
    virtual void start(const Job &job, const uint32_t initialNonce) = 0;

    virtual void stop() = 0;

    virtual void setNewJob(const Job &job, const uint32_t initialNonce) = 0;

    virtual std::vector<PerformanceStats> getPerformanceStats() = 0;

    virtual ~IBackend() {};
};
