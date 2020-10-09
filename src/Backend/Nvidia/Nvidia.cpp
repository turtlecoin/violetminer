// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

//////////////////////////////////
#include "Backend/Nvidia/Nvidia.h"
//////////////////////////////////

#include <iostream>

#include "ArgonVariants/Variants.h"
#include "Backend/Nvidia/NvidiaHash.h"
#include "Utilities/ColouredMsg.h"
#include "Nvidia/Argon2.h"

Nvidia::Nvidia(
    const std::shared_ptr<HardwareConfig> &hardwareConfig,
    const std::function<void(const JobSubmit &jobSubmit)> &submitValidHashCallback,
    const std::function<void(
        const uint32_t hashesPerformed,
        const std::string &deviceName)> &incrementHashesPerformedCallback):
    m_hardwareConfig(hardwareConfig),
    m_submitValidHash(submitValidHashCallback),
    m_incrementHashesPerformed(incrementHashesPerformedCallback)
{
    m_numAvailableGPUs = std::count_if(
        hardwareConfig->nvidia.devices.begin(),
        hardwareConfig->nvidia.devices.end(),
        [](const NvidiaDevice &device)
        {
            return device.enabled;
        }
    );
}

void Nvidia::start(const Job &job, const uint32_t initialNonce)
{
    if (!m_threads.empty())
    {
        stop();
    }

    m_shouldStop = false;

    m_nonce = initialNonce;

    m_currentJob = job;

    /* Indicate that there's no new jobs available to other threads */
    m_newJobAvailable = std::vector<bool>(m_numAvailableGPUs, false);

    for (uint32_t i = 0; i < m_hardwareConfig->nvidia.devices.size(); i++)
    {
        auto &gpu = m_hardwareConfig->nvidia.devices[i];

        if (!gpu.enabled)
        {
            continue;
        }

        const uint32_t gpuLag = getGpuLagMicroseconds(gpu);
        const double seconds = gpuLag / 1000000.0;

        std::cout << WhiteMsg("[GPU " + std::to_string(gpu.id) + "] ")
                  << InformationMsg("Intensity: ") << SuccessMsg(gpu.intensity) << SuccessMsg(", ")
                  << InformationMsg("Desktop Lag: ") << SuccessMsg(gpu.desktopLag) << "\n"
                  << WhiteMsg("[GPU " + std::to_string(gpu.id) + "] ")
                  << InformationMsg("Sleeping for ") << InformationMsg(seconds) << InformationMsg(" seconds between kernel launches")
                  << SuccessMsg(" (") << SuccessMsg(gpuLag) << SuccessMsg(" microseconds)") << std::endl;

        m_threads.push_back(std::thread(&Nvidia::hash, this, std::ref(gpu), i));
    }
}

void Nvidia::stop()
{
    m_shouldStop = true;

    for (int i = 0; i < m_numAvailableGPUs; i++)
    {
        m_newJobAvailable[i] = true;
    }

    /* Wait for all the threads to stop */
    for (auto &thread : m_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    /* Empty the threads vector for later re-creation */
    m_threads.clear();
}

void Nvidia::setNewJob(const Job &job, const uint32_t initialNonce)
{
    /* Set new nonce */
    m_nonce = initialNonce;

    /* Update stored job */
    m_currentJob = job;

    /* Indicate to each thread that there's a new job */
    for (int i = 0; i < m_numAvailableGPUs; i++)
    {
        m_newJobAvailable[i] = true;
    }
}

std::vector<PerformanceStats> Nvidia::getPerformanceStats()
{
    return {};
}

std::shared_ptr<NvidiaHash> getNvidiaMiningAlgorithm(const std::string &algorithm)
{
    switch(ArgonVariant::algorithmNameToCanonical(algorithm))
    {
        case ArgonVariant::Chukwa:
        {
            return std::make_shared<NvidiaHash>(512, 3);
        }
        case ArgonVariant::ChukwaV2:
        {
            return std::make_shared<NvidiaHash>(1024, 4);
        }
        case ArgonVariant::ChukwaWrkz:
        {
            return std::make_shared<NvidiaHash>(256, 4);
        }
        default:
        {
            throw std::runtime_error("Developer fucked up. Sorry!");
        }
    }
}

uint32_t Nvidia::getGpuLagMicroseconds(const NvidiaDevice &gpu)
{
    /* woooo magical shitty formula */
    return 45 * (pow(2, ((100 - gpu.desktopLag) * 0.2)) - 1);
}

void Nvidia::hash(NvidiaDevice &gpu, const uint32_t threadNumber)
{
    NvidiaState state;

    std::string currentAlgorithm;

    const std::string gpuName = gpu.name + "-" + std::to_string(gpu.id);

    NonceInfo nonceInfo;

    const uint32_t gpuLag = getGpuLagMicroseconds(gpu);

    bool failure = false;

    while (!m_shouldStop)
    {
        Job job = m_currentJob;

        auto algorithm = getNvidiaMiningAlgorithm(job.algorithm);

        /* New job, reinitialize memory, etc */
        if (job.algorithm != currentAlgorithm)
        {
            freeState(state);

            state = initializeState(
                gpu.id,
                algorithm->getMemory(),
                algorithm->getIterations(),
                gpu.intensity
            );

            {
                /* Aquire lock to ensure multiple GPU's don't interleave output */
                std::scoped_lock lock(m_outputMutex);

                std::cout << WhiteMsg("[GPU " + std::to_string(gpu.id) + "] ")
                          << InformationMsg("Allocating ")
                          << SuccessMsg(static_cast<double>(state.launchParams.memSize) / (1024 * 1024 * 1024))
                          << SuccessMsg("GB") << InformationMsg(" of GPU memory.") << "\n"
                          << WhiteMsg("[GPU " + std::to_string(gpu.id) + "] ")
                          << InformationMsg("Performing ")
                          << SuccessMsg(state.launchParams.noncesPerRun)
                          << InformationMsg(" iterations per kernel launch, with ")
                          << SuccessMsg(state.launchParams.jobsPerBlock)
                          << InformationMsg(" jobs per block.")
                          << std::endl;
            }

            currentAlgorithm = job.algorithm;

            gpu.noncesPerRound = state.launchParams.noncesPerRun;
            gpu.checkedIn = true;

            nonceInfo = m_hardwareConfig->getNonceOffsetInfo("nvidia", gpu.id);
        }

        state.isNiceHash = job.isNiceHash;

        std::vector<uint8_t> salt(job.rawBlob.begin(), job.rawBlob.begin() + 16);

        uint32_t localNonce = m_nonce;

        initJob(state, job.rawBlob, salt, job.target);

        /* Let the algorithm perform any necessary initialization */
        algorithm->init(state);

        int i = 0;

        while (!m_newJobAvailable[threadNumber])
        {
            const uint32_t ourNonce = localNonce + (i * nonceInfo.noncesPerRound) + nonceInfo.nonceOffset;

            try
            {
                const auto hashResult = algorithm->hash(ourNonce);

                /* Increment the number of hashes we performed so the hashrate
                   printer is accurate */
                m_incrementHashesPerformed(state.launchParams.noncesPerRun, gpuName);

                /* Woot, found a valid share, submit it */
                if (hashResult.success)
                {
                    m_submitValidHash({ hashResult.hash, job.jobID, hashResult.nonce, job.target, gpuName });
                }

                if (gpuLag > 0)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(gpuLag));
                }

                failure = false;
            }
            catch (const std::exception &e)
            {
                std::cout << WarningMsg("Caught unexpected error from GPU hasher: " + std::string(e.what())) << std::endl;
                std::cout << WarningMsg("Stopping mining on " + gpuName) << std::endl;

                /* We allow one failure, as non sticky errors are recoverable.
                 * Sticky errors however, require the process to be relaunched. */
                if (failure)
                {
                    return;
                }

                failure = true;
            }

            i++;

            /* If not all hardware has checked in with the new job, keep attempting
             * to fetch it to ensure we're not doing duplicate work. */
            if (!nonceInfo.allHardwareInitialized)
            {
                nonceInfo = m_hardwareConfig->getNonceOffsetInfo("nvidia", gpu.id);
            }
        }

        /* Switch to new job. */
        m_newJobAvailable[threadNumber] = false;
    }

    freeState(state);
}
