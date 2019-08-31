// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

////////////////////////////
#include "Backend/CPU/CPU.h"
////////////////////////////

#include "Types/JobSubmit.h"

CPU::CPU(
    const HardwareConfig &hardwareConfig,
    const std::function<void(const JobSubmit &jobSubmit)> &submitHashCallback):
    m_hardwareConfig(hardwareConfig),
    m_submitHash(submitHashCallback)
{
}

void CPU::start(const Job &job, const uint32_t initialNonce)
{
    if (!m_threads.empty())
    {
        stop();
    }

    m_shouldStop = false;

    m_nonce = initialNonce;

    m_currentJob = job;

    /* Indicate that there's no new jobs available to other threads */
    m_newJobAvailable = std::vector<bool>(m_hardwareConfig.cpu.threadCount, false);

    for (uint32_t i = 0; i < m_hardwareConfig.cpu.threadCount; i++)
    {
        m_threads.push_back(std::thread(&CPU::hash, this, i));
    }
}

void CPU::stop()
{
    m_shouldStop = true;

    for (int i = 0; i < m_newJobAvailable.size(); i++)
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

void CPU::setNewJob(const Job &job, const uint32_t initialNonce)
{
    /* Set new nonce */
    m_nonce = initialNonce;

    /* Update stored job */
    m_currentJob = job;

    /* Indicate to each thread that there's a new job */
    for (int i = 0; i < m_newJobAvailable.size(); i++)
    {
        m_newJobAvailable[i] = true;
    }
}

std::vector<PerformanceStats> CPU::getPerformanceStats()
{
    return {};
}

void CPU::hash(const uint32_t threadNumber)
{
    while (!m_shouldStop)
    {
        /* Offset the nonce by our thread number so each thread has an individual
           nonce */
        uint32_t localNonce = m_nonce + threadNumber;

        Job job = m_currentJob;

        const bool isNiceHash = job.isNiceHash;

        auto algorithm = ArgonVariant::getCPUMiningAlgorithm(m_currentJob.algorithm);

        /* Let the algorithm perform any necessary initialization */
        algorithm->init(m_currentJob.rawBlob);
        algorithm->reinit(m_currentJob.rawBlob);

        while (!m_newJobAvailable[threadNumber])
        {
            /* If nicehash mode is enabled, we are only allowed to alter 3 bytes
               in the nonce, instead of four. The first byte is reserved for nicehash
               to do with as they like.
               To achieve this, we wipe the top byte (localNonce & 0x00FFFFFF) of
               local nonce. We then wipe the bottom 3 bytes of job.nonce
               (*job.nonce() & 0xFF000000). Finally, we AND them together, so the
               top byte of the nonce is reserved for nicehash.
               See further
               https://github.com/nicehash/Specifications/blob/master/NiceHash_CryptoNight_modification_v1.0.txt Note
               that the above specification indicates that the final byte of the nonce is reserved, but in fact it is
               the first byte that is reserved. */
            if (isNiceHash)
            {
                *job.nonce() = (localNonce & 0x00FFFFFF) | (*job.nonce() & 0xFF000000);
            }
            else
            {
                *job.nonce() = localNonce;
            }

            const auto hash = algorithm->hash(job.rawBlob);

            m_submitHash({hash, job.jobID, *job.nonce(), job.target});

            localNonce += m_hardwareConfig.cpu.threadCount;
        }

        /* Switch to new job. */
        m_newJobAvailable[threadNumber] = false;
    }
}
