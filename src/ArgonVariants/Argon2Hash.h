// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

#pragma once

#include "Argon2/Argon2.h"
#include "Types/IHashingAlgorithm.h"

class Argon2Hash : virtual public IHashingAlgorithm
{
  public:
    Argon2Hash(
        const uint32_t memoryKB,
        const uint32_t iterations,
        const uint32_t threads,
        const uint32_t saltLength,
        const Constants::ArgonVariant variant);

    virtual void init(std::vector<uint8_t> &initialInput);

    virtual void reinit(const std::vector<uint8_t> &input);

    virtual std::vector<uint8_t> hash(std::vector<uint8_t> &input);

  private:
    Argon2 m_argonInstance;

    const uint32_t m_saltLength;

    std::vector<uint8_t> m_salt;
};
