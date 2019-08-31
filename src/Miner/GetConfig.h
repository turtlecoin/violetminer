// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

#pragma once

#include "Argon2/Constants.h"
#include "Types/Pool.h"

#include <string>
#include <thread>

struct NvidiaDevice
{
    bool enabled = true;

    std::string name;

    uint16_t id;
};

struct AmdDevice
{
    bool enabled = true;

    std::string name;

    uint16_t id;
};

struct CpuConfig
{
    bool enabled = true;

    uint32_t threadCount = std::thread::hardware_concurrency();

    Constants::OptimizationMethod optimizationMethod = Constants::OptimizationMethod::AUTO;
};

struct NvidiaConfig
{
    std::vector<NvidiaDevice> devices;
};

struct AmdConfig
{
    std::vector<AmdDevice> devices;
};

struct HardwareConfig
{
    CpuConfig cpu;
    NvidiaConfig nvidia;
    AmdConfig amd;
};

struct MinerConfig
{
    std::vector<Pool> pools;

    std::string configLocation;

    bool interactive = false;

    HardwareConfig hardwareConfiguration;
};

void to_json(nlohmann::json &j, const MinerConfig &config);

void from_json(const nlohmann::json &j, MinerConfig &config);

std::vector<Constants::OptimizationMethod> getAvailableOptimizations();

Constants::OptimizationMethod getAutoChosenOptimization();

Pool getPool();

std::vector<Pool> getPools();

MinerConfig getConfigInteractively();

MinerConfig getConfigFromJSON(const std::string &configLocation);

MinerConfig getMinerConfig(int argc, char **argv);
