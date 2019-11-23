// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

#pragma once

#include <string>
#include <thread>

#include "Types/Pool.h"
#include "Argon2/Constants.h"

#if defined(NVIDIA_ENABLED)
#include "Backend/Nvidia/NvidiaUtils.h"
#endif

struct NvidiaDevice
{
    /* Is this device enabled for mining. */
    bool enabled = true;

    /* The name of this device to display to the user. */
    std::string name;

    /* The internal ID of this device. In Nvidia, these are zero indexed offsets.
     * The first GPU is 0, second is 1, etc. */
    uint16_t id;

    /* Has this device checked in since we last received a job. If not, it
     * may be inaccurate. */
    bool checkedIn = false;

    /* How many nonces per hashing round does this device calculate. This is
     * used to calculate how much each device should increment and offset
     * it's nonces. */
    uint32_t noncesPerRound = 0;

    /* Multiplier to decide how much memory / threads to launch. 0-100. */
    float intensity = 100.0;

    /* Determines how much we sleep between kernel launches. Helps the desktop
     * not be such a laggy POS while mining. */
    float desktopLag = 100.0;
};

struct AmdDevice
{
    bool enabled = true;

    std::string name;

    uint16_t id;

    bool checkedIn = true;

    uint32_t noncesPerRound = 0;

    float intensity = 100.0;

    float desktopLag = 100.0;
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

struct NonceInfo
{
    uint32_t noncesPerRound = 0;

    uint32_t nonceOffset = 0;

    bool allHardwareInitialized = true;
};

struct HardwareConfig
{
    CpuConfig cpu;
    NvidiaConfig nvidia;
    AmdConfig amd;

    uint32_t noncesPerRound;

    NonceInfo getNonceOffsetInfo(const std::string device, const uint32_t gpuIndex = 0)
    {
        NonceInfo nonceInfo;

        bool foundOurDevice = false;

        if (cpu.enabled)
        {
            /* CPU will process one nonce per round, per thread */
            nonceInfo.noncesPerRound += cpu.threadCount;

            /* CPU will start processing nonces with no offset. */
            if (device != "cpu")
            {
                nonceInfo.nonceOffset += cpu.threadCount;
            }
            else
            {
                foundOurDevice = true;
            }
        }

        for (auto &gpu : nvidia.devices)
        {
            if (gpu.enabled)
            {
                nonceInfo.noncesPerRound += gpu.noncesPerRound;

                /* Each GPU will need to check in with it's new nonce per
                 * round calculation. Otherwise, offsets may be incorrect
                 * for example if scratchpad size changed. Therefore, if
                 * all hardware has not been initialized, we'll keep
                 * fetching new offsets. */
                if (!gpu.checkedIn)
                {
                    nonceInfo.allHardwareInitialized = false;
                }

                /* No more changes to nonce offset, found our device */
                if (device == "nvidia" && gpuIndex == gpu.id)
                {
                    foundOurDevice = true;
                }
                /* If we haven't found our device yet, keep incrementing nonce offset */
                else if (!foundOurDevice)
                {
                    nonceInfo.nonceOffset += gpu.noncesPerRound;
                }
            }
        }

        for (auto &gpu : amd.devices)
        {
            if (gpu.enabled)
            {
                nonceInfo.noncesPerRound += gpu.noncesPerRound;

                if (!gpu.checkedIn)
                {
                    nonceInfo.allHardwareInitialized = false;
                }

                /* No more changes to nonce offset, found our device */
                if (device == "amd" && gpuIndex == gpu.id)
                {
                    foundOurDevice = true;
                }
                /* If we haven't found our device yet, keep incrementing nonce offset */
                else if (!foundOurDevice)
                {
                    nonceInfo.nonceOffset += gpu.noncesPerRound;
                }
            }
        }

        return nonceInfo;
    }
};

struct MinerConfig
{
    std::vector<Pool> pools;

    std::string configLocation;

    std::shared_ptr<HardwareConfig> hardwareConfiguration = std::make_shared<HardwareConfig>();
};

void to_json(nlohmann::json &j, const MinerConfig &config);

void from_json(const nlohmann::json &j, MinerConfig &config);

std::vector<Constants::OptimizationMethod> getAvailableOptimizations();

Constants::OptimizationMethod getAutoChosenOptimization();

Pool getPool();

std::vector<Pool> getPools();

void writeConfigToDisk(MinerConfig config, const std::string &configLocation);

MinerConfig getConfigInteractively();

MinerConfig getConfigFromJSON(const std::string &configLocation);

MinerConfig getMinerConfig(int argc, char **argv);
