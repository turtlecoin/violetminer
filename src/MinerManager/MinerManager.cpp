// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

//////////////////////////////////////
#include "MinerManager/MinerManager.h"
//////////////////////////////////////

#include "Backend/CPU/CPU.h"
#include "Types/JobSubmit.h"
#include "Utilities/ColouredMsg.h"
#include "Utilities/Utilities.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#if defined(NVIDIA_ENABLED)
#include "Backend/Nvidia/Nvidia.h"
#include "MinerManager/Nvidia/NvidiaManager.h"
#endif

MinerManager::MinerManager(const std::shared_ptr<PoolCommunication> pool, const HardwareConfig hardwareConfig):
    m_pool(pool),
    m_hardwareConfig(hardwareConfig),
    m_hashManager(pool),
    m_gen(m_device())
{
    const auto submit = [this](const JobSubmit &jobSubmit) { m_hashManager.submitHash(jobSubmit); };

    if (hardwareConfig.cpu.enabled)
    {
        m_enabledBackends.push_back(std::make_shared<CPU>(hardwareConfig, submit));
    }
    /* TODO: This will print for the dev manager as well. Probably want a bool
       to indicate which manager we're in. */
    else
    {
        std::cout << WarningMsg("CPU mining disabled.") << std::endl;
    }
}

MinerManager::~MinerManager()
{
    stop();
}

void MinerManager::setNewJob(const Job &job)
{
    /* Set new nonce */
    const uint32_t nonce = m_distribution(m_gen);

    for (auto &backend : m_enabledBackends)
    {
        backend->setNewJob(job, nonce);
    }

    m_pool->printPool();

    /* Let the user know we got a new job */
    std::cout << WhiteMsg("New job, diff ") << WhiteMsg(job.shareDifficulty) << std::endl;
}

void MinerManager::start()
{
    if (!m_threads.empty() || m_statsThread.joinable())
    {
        stop();
    }

    m_shouldStop = false;

    /* Hook up the function to set a new job when it arrives */
    m_pool->onNewJob([this](const Job &job) { setNewJob(job); });

    /* Pass through accepted shares to the hash manager */
    m_pool->onHashAccepted([this](const auto &) { m_hashManager.shareAccepted(); });

    /* Start mining when we connect to a pool */
    m_pool->onPoolSwapped([this](const Pool &newPool) { resumeMining(); });

    /* Stop mining when we disconnect */
    m_pool->onPoolDisconnected([this]() { pauseMining(); });

    /* Start listening for messages from the pool */
    m_pool->startManaging();
}

void MinerManager::resumeMining()
{
    if (!m_threads.empty())
    {
        pauseMining();
    }

    m_shouldStop = false;

    std::cout << WhiteMsg("Resuming mining.") << std::endl;

    const auto job = m_pool->getJob();

    m_pool->printPool();
    std::cout << WhiteMsg("New job, diff ") << WhiteMsg(job.shareDifficulty) << std::endl;

    /* Set initial nonce */
    const uint32_t nonce = m_distribution(m_gen);

    for (auto &backend : m_enabledBackends)
    {
        backend->start(job, nonce);
    }

    /* Start nvidia mining if nvidia mining enabled */
    startNvidiaMining();

    /* Launch off the thread to print stats regularly */
    m_statsThread = std::thread(&MinerManager::statPrinter, this);
}

void MinerManager::startNvidiaMining()
{
#if defined(NVIDIA_ENABLED)
    int maxErrorCount = 5;
    int currentErrorCount = 0;

    auto resetErrorCountPeriod = std::chrono::seconds(20);

    auto lastErrorAt = std::chrono::high_resolution_clock::now();

    const bool allNvidiaGPUsDisabled = std::none_of(
        m_hardwareConfig.nvidia.devices.begin(), m_hardwareConfig.nvidia.devices.end(), [](const auto device) {
            return device.enabled;
        });

    while (true)
    {
        /* If we have at least one device, start nvidia mining */
        if (!allNvidiaGPUsDisabled)
        {
            try
            {
                resumeNvidiaMining();
            }
            catch (const std::exception &e)
            {
                const auto now = std::chrono::high_resolution_clock::now();

                /* If we haven't had an error in the last 20 seconds, reset the error
                   count */
                if (now - resetErrorCountPeriod > lastErrorAt)
                {
                    currentErrorCount = 0;
                }
                /* Otherwise, increment the error count */
                else
                {
                    currentErrorCount++;
                }

                /* If we've had more than 5 crashes in ~100 seconds, stop
                   attempting to restart, it ain't working. */
                if (currentErrorCount > maxErrorCount)
                {
                    std::cout << WarningMsg("Too many errors in a short period of time. Cancelling Nvidia Mining.\n")
                              << std::endl;
                    return;
                }

                lastErrorAt = now;

                std::cout << WarningMsg("Error performining mining on Nvidia GPU: " + std::string(e.what()))
                          << std::endl;
                std::cout << InformationMsg("Restarting Nvidia mining...\n") << std::endl;

                /* TODO: Remove */
                return;

                continue;
            }
        }
        else
        {
            std::cout << WarningMsg("No Nvidia GPUs available, or all disabled, not starting Nvidia mining")
                      << std::endl;
            return;
        }

        /* TODO: Remove */
        return;
    }
#endif
}

void MinerManager::pauseMining()
{
    std::cout << WhiteMsg("Pausing mining.") << std::endl;

    m_shouldStop = true;

    /* Pause the hashrate calculator */
    m_hashManager.pause();

    if (m_statsThread.joinable())
    {
        m_statsThread.join();
    }
}

void MinerManager::stop()
{
    m_shouldStop = true;

    /* Pause the hashrate calculator */
    m_hashManager.pause();

    /* Wait for the stats thread to stop */
    if (m_statsThread.joinable())
    {
        m_statsThread.join();
    }

    /* Close the socket connection to the pool */
    if (m_pool)
    {
        m_pool->logout();
    }
}

void MinerManager::printStats()
{
    m_hashManager.printStats();
}

void MinerManager::statPrinter()
{
    m_hashManager.start();

    while (!m_shouldStop)
    {
        Utilities::sleepUnlessStopping(std::chrono::seconds(20), m_shouldStop);
        printStats();
    }
}
