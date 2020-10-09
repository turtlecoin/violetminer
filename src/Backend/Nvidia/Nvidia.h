// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include "Backend/IBackend.h"
#include "Types/JobSubmit.h"

class Nvidia : virtual public IBackend
{
  public:
    Nvidia(
        const std::shared_ptr<HardwareConfig> &hardwareConfig,
        const std::function<void(const JobSubmit &jobSubmit)> &submitValidHashCallback,
        const std::function<void(
            const uint32_t hashesPerformed,
            const std::string &deviceName)> &incrementHashesPerformedCallback);

    virtual void start(const Job &job, const uint32_t initialNonce);

    virtual void stop();

    virtual void setNewJob(const Job &job, const uint32_t initialNonce);

    virtual std::vector<PerformanceStats> getPerformanceStats();

  private:

    void hash(NvidiaDevice &gpu, const uint32_t threadNumber);

    uint32_t getGpuLagMicroseconds(const NvidiaDevice &gpu);

    /* Current job to be working on */
    Job m_currentJob;

    /* Nonce to begin hashing at */
    uint32_t m_nonce;

    /* Should we stop the worker funcs */
    std::atomic<bool> m_shouldStop = false;

    /* Threads to launch, whether CPU/GPU is enabled, etc */
    std::shared_ptr<HardwareConfig> m_hardwareConfig;

    /* Worker threads */
    std::vector<std::thread> m_threads;

    /* A bool for each thread indicating if they should swap to a new job */
    std::vector<bool> m_newJobAvailable;

    /* Used to submit a valid hash back to the miner manager */
    const std::function<void(const JobSubmit &jobSubmit)> m_submitValidHash;

    /* Used to increment the number of hashes we've performed */
    const std::function<void(
        const uint32_t hashesPerformed,
        const std::string &deviceName)> m_incrementHashesPerformed;

    size_t m_numAvailableGPUs;

    /* Mutex to ensure output is not interleaved */
    std::mutex m_outputMutex;
};
