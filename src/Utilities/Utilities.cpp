// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

////////////////////////////////
#include "Utilities/Utilities.h"
////////////////////////////////

#include <thread>

namespace Utilities
{
    void sleepUnlessStopping(const std::chrono::milliseconds duration, std::atomic<bool> &condition)
    {
        auto sleptFor = std::chrono::milliseconds::zero();

        /* 0.5 seconds */
        const auto sleepDuration = std::chrono::milliseconds(500);

        while (!condition && sleptFor < duration)
        {
            std::this_thread::sleep_for(sleepDuration);

            sleptFor += sleepDuration;
        }
    }
} // namespace Utilities
