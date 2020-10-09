// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

////////////////////////////////////////////////
#include "PoolCommunication/PoolCommunication.h"
////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <thread>

#include "Config/Constants.h"
#include "ExternalLibs/json.hpp"
#include "SocketWrapper/SocketWrapper.h"
#include "Types/PoolMessage.h"
#include "Utilities/ColouredMsg.h"
#include "Utilities/Utilities.h"

PoolCommunication::PoolCommunication(std::vector<Pool> allPools)
{
    /* Sort pools based on their priority */
    std::sort(allPools.begin(), allPools.end(), [](const auto a, const auto b)
    {
        return a.priority < b.priority;
    });

    m_allPools = allPools;
}

std::string formatPool(const Pool pool)
{
    return "[" + pool.host + ":" + std::to_string(pool.port) + "] ";
}

void loginFailed(
    const Pool pool,
    const int loginAttempt,
    const bool connectFail,
    const std::string customMessage = "")
{
    std::stringstream stream;

    stream << "Failed to " << (connectFail ? "connect" : "login")
           << " to pool, attempt ";

    std::cout << InformationMsg(formatPool(pool)) << WarningMsg(stream.str())
              << InformationMsg(loginAttempt)
              << InformationMsg("/")
              << InformationMsg(Constants::MAX_LOGIN_ATTEMPTS) << std::endl;

    if (customMessage != "")
    {
        std::cout << InformationMsg(formatPool(pool))
                  << WarningMsg("Error: " + customMessage) << std::endl;
    }

    if (loginAttempt != Constants::MAX_LOGIN_ATTEMPTS)
    {
        std::cout << InformationMsg(formatPool(pool)) << "Will try again in "
                  << InformationMsg(Constants::POOL_LOGIN_RETRY_INTERVAL / 1000)
                  << " seconds." << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(Constants::POOL_LOGIN_RETRY_INTERVAL));
}

void PoolCommunication::printPool() const
{
    std::cout << InformationMsg(formatPool(m_currentPool));
}

PoolCommunication::~PoolCommunication()
{
    logout();
}

void PoolCommunication::logout()
{
    m_shouldStop = true;

    m_findNewPool.notify_all();

    if (m_socket)
    {
        m_socket->stop();
    }

    if (m_managerThread.joinable())
    {
        m_managerThread.join();
    }
}

void PoolCommunication::getNewJob()
{
    const nlohmann::json newJobMsg = {
        {"method", "getjob"},
        {"params", {
            {"id", m_currentPool.loginID},
            {"rigid", m_currentPool.rigID},
            {"agent", m_currentPool.getAgent()},
        }},
        {"id", 1}
    };

    m_socket->sendMessage(newJobMsg.dump() + "\n");
}

void PoolCommunication::registerHandlers()
{
    m_socket->onMessage([this](std::string message) {
        try
        {
            Utilities::trim(message);
            Utilities::removeCharFromString(message, '\n');
            Utilities::removeCharFromString(message, '\0');

            if (message == "")
            {
                return;
            }

            auto poolMessage = parsePoolMessage(message);

            if (auto job = std::get_if<JobMessage>(&poolMessage))
            {
                updateJobInfoFromPool(job->job);

                m_currentJob = job->job;

                if (m_onNewJob)
                {
                    m_onNewJob(job->job);
                }
            }
            else if (auto status = std::get_if<StatusMessage>(&poolMessage))
            {
                if (status->status == "OK" && m_onHashAccepted)
                {
                    m_onHashAccepted(status->ID);
                }
                else if (status->status == "KEEPALIVED")
                {
                    // kept alive
                }
                else
                {
                    std::cout << WarningMsg("Unknown status message: " + status->status);
                }
            }
            else if (auto error = std::get_if<ErrorMessage>(&poolMessage))
            {
                const auto errorMessage = error->error.errorMessage;

                std::cout << InformationMsg("Error message received from pool: ") << WarningMsg(errorMessage) << std::endl;

                if (errorMessage == "Low difficulty share")
                {
                    std::cout << WarningMsg("Probably a stale job, unless you are only getting rejected shares") << std::endl
                              << WarningMsg("If this is the case, ensure you are using the correct mining algorithm for this pool.") << std::endl;
                }
                else if (errorMessage == "Invalid nonce; is miner not compatible with NiceHash?")
                {
                    std::cout << WarningMsg("Make sure \"niceHash\" is set to true in your config file.") << std::endl;
                }
                else if (errorMessage == "Invalid job id")
                {
                    getNewJob();
                }
            }
            else
            {
                std::cout << WarningMsg("Unexpected message: " + message) << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cout << WarningMsg(e.what()) << std::endl;
        }
    });

    /* Socket closed */
    m_socket->onSocketClosed([this]() {
        std::cout << WarningMsg("Lost connection with pool.") << std::endl;

        /* Let the miner know to stop mining */
        if (m_onPoolDisconnected) {
            m_onPoolDisconnected();
        }

        std::unique_lock<std::mutex> lock(m_mutex);

        m_shouldFindNewPool = true;

        m_findNewPool.notify_all();
    });
}

Job PoolCommunication::getJob()
{
    return m_currentJob;
}

void PoolCommunication::submitShare(
    const uint8_t *hash,
    const std::string jobID,
    const uint32_t nonce)
{
    const nlohmann::json submitMsg = {
        {"method", "submit"},
        {"params", {
            {"id", m_currentPool.loginID},
            {"job_id", jobID},
            {"nonce", Utilities::toHex(nonce)},
            {"result", Utilities::toHex(hash, 32)},
            {"rigid", m_currentPool.rigID},
            {"agent", m_currentPool.getAgent()},
        }},
        {"id", 1}
    };

    m_socket->sendMessage(submitMsg.dump() + "\n");
}

void PoolCommunication::onNewJob(const std::function<void(const Job &job)> callback)
{
    m_onNewJob = callback;
}

void PoolCommunication::onHashAccepted(const std::function<void(const std::string &shareID)> callback)
{
    m_onHashAccepted = callback;
}

/* Called whenever we disconnected from the current pool, and connected to a new pool */
void PoolCommunication::onPoolSwapped(const std::function<void(const Pool &pool)> callback)
{
    m_onPoolSwapped = callback;
}

/* Called whenever we disconnected from the current pool */
void PoolCommunication::onPoolDisconnected(const std::function<void(void)> callback)
{
    m_onPoolDisconnected = callback;
}

/* Start managing the pool communication, handle messages, socket closing,
   reconnecting */
void PoolCommunication::startManaging()
{
    m_shouldStop = true;

    if (m_managerThread.joinable())
    {
        m_managerThread.join();
    }

    m_shouldStop = false;
    m_shouldFindNewPool = true;

    m_managerThread = std::thread(&PoolCommunication::managePools, this);
}

bool PoolCommunication::tryLogin(const Pool &pool)
{
    std::shared_ptr<sockwrapper::SocketWrapper> socket;

    #if defined(SOCKETWRAPPER_OPENSSL_SUPPORT)
    if (pool.ssl)
    {
        socket = std::make_shared<sockwrapper::SSLSocketWrapper>(
            pool.host.c_str(), pool.port, '\n', Constants::POOL_LOGIN_RETRY_INTERVAL / 1000
        );
    }
    else
    {
    #endif
        socket = std::make_shared<sockwrapper::SocketWrapper>(
            pool.host.c_str(), pool.port, '\n', Constants::POOL_LOGIN_RETRY_INTERVAL / 1000
        );
    #if defined(SOCKETWRAPPER_OPENSSL_SUPPORT)
    }
    #endif

    std::cout << InformationMsg(formatPool(pool)) << SuccessMsg("Attempting to connect to pool...") << std::endl;

    for (int i = 1; i <= Constants::MAX_LOGIN_ATTEMPTS; i++)
    {
        const bool success = socket->start();

        if (!success)
        {
            loginFailed(pool, i, true);
            continue;
        }

        const nlohmann::json loginMsg = {
            {"method", "login"},
            {"params", {
                {"login", pool.username},
                {"pass", pool.password},
                {"rigid", pool.rigID},
                {"agent", pool.getAgent()}
            }},
            {"id", 1},
            {"jsonrpc", "2.0"}
        };

        const auto res = socket->sendMessageAndGetResponse(loginMsg.dump() + "\n");

        if (res)
        {
            LoginMessage message;

            try
            {
                message = nlohmann::json::parse(*res);
            }
            catch (const std::exception &e)
            {
                try
                {
                    /* Failed to parse as LoginMessage. Maybe it's an error message? */
                    const ErrorMessage errMessage = nlohmann::json::parse(*res);
                    loginFailed(pool, i, false, errMessage.error.errorMessage);
                }
                catch (const std::exception &)
                {
                    loginFailed(pool, i, false, "Failed to parse message from pool (" + std::string(e.what()) + ") (" + *res + ")");
                }

                continue;
            }

            try
            {
                std::cout << InformationMsg(formatPool(pool)) << SuccessMsg("Logged in.") << std::endl;

                if (m_socket)
                {
                    m_socket->stop();
                }

                m_socket = socket;
                m_currentPool = pool;
                m_currentPool.loginID = message.loginID;
                updateJobInfoFromPool(message.job);
                m_currentJob = message.job;

                if (*message.job.nonce() != 0)
                {
                    m_currentPool.niceHash = true;
                }

                registerHandlers();

                if (m_onPoolSwapped)
                {
                    m_onPoolSwapped(pool);
                }

                return true;
            }
            catch (const std::exception &e)
            {
                loginFailed(pool, i, false, std::string(e.what()));
                continue;
            }
        }
        else
        {
            loginFailed(pool, i, false);
            continue;
        }
    }

    std::cout << InformationMsg(formatPool(pool)) << WarningMsg("All login/connect attempts failed.") << std::endl;

    return false;
}

void PoolCommunication::managePools()
{
    auto lastKeptAlive = std::chrono::high_resolution_clock::now();

    while (!m_shouldStop)
    {
        if (m_shouldFindNewPool) {
            m_currentPoolIndex = m_allPools.size();
        }

        /* Most preferred pool = 0, current pool is = current pool index, so if we're
           not connected to the most preferred pool, we step down the list, in
           order of preference, trying to reconnect to each. */
        for (size_t poolPreference = 0; poolPreference < m_currentPoolIndex; poolPreference++)
        {
            if (m_shouldStop)
            {
                return;
            }

            /* Grab the pool */
            const auto pool = m_allPools[poolPreference];

            /* Try and login */
            const bool loginSuccess = tryLogin(pool);

            if (loginSuccess)
            {
                /* Cool, got a more preferred pool. */
                m_currentPoolIndex = poolPreference;
                m_shouldFindNewPool = false;
                break;
            }
        }

        /* Still not found a pool. Go again. */
        if (m_shouldFindNewPool)
        {
            continue;
        }
        else if (lastKeptAlive + std::chrono::seconds(120) < std::chrono::high_resolution_clock::now())
        {
            keepAlive();
            lastKeptAlive = std::chrono::high_resolution_clock::now();
        }

        std::unique_lock<std::mutex> lock(m_mutex);

        /* Nice, found a pool. Wait for the timeout, or for a pool to disconnect,
           then we'll retry any possibly more preferred pools. */
        m_findNewPool.wait_for(lock, std::chrono::seconds(5), [&]{
            if (m_shouldStop)
            {
                return true;
            }

            return m_shouldFindNewPool;
        });
    }
}

void PoolCommunication::keepAlive()
{
    const nlohmann::json pingMsg = {
        {"method", "keepalived"},
        {"params", {
            {"id", m_currentPool.loginID},
            {"rigid", m_currentPool.rigID},
            {"agent", m_currentPool.getAgent()},
        }},
        {"id", 1}
    };

    m_socket->sendMessage(pingMsg.dump() + "\n");
}

void PoolCommunication::updateJobInfoFromPool(Job &job) const
{
    job.isNiceHash = isNiceHash();

    if (job.algorithm.empty() || m_currentPool.disableAutoAlgoSelect)
    {
        job.algorithm = m_currentPool.algorithm;
    }
}

bool PoolCommunication::isNiceHash() const
{
    return m_currentPool.niceHash;
}
