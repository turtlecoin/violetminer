// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

#include <iostream>
#include <map>
#include <vector>

#include "Nvidia/Argon2.h"
#include "Utilities/ColouredMsg.h"

#include <thrust/system_error.h>
#include <thrust/system/cuda/error.h>
#include <sstream>

int getDeviceCount()
{
    int numberDevices;

    bool haveDevice = throw_on_cuda_error(cudaGetDeviceCount(&numberDevices), __FILE__, __LINE__);

    if (!haveDevice)
    {
        return 0;
    }

    return numberDevices;
}

std::string getDeviceName(uint16_t deviceId)
{
    cudaDeviceProp prop;
    throw_on_cuda_error(cudaGetDeviceProperties(&prop, deviceId), __FILE__, __LINE__);
    return prop.name;
}

std::vector<std::tuple<std::string, bool, int>> getNvidiaDevicesActual()
{
    std::vector<std::tuple<std::string, bool, int>> devices;

    int numberDevices = getDeviceCount();

    for (int i = 0; i < numberDevices; i++)
    {
        const auto device = getDeviceName(i);

        if (device == "")
        {
            continue;
        }

        devices.push_back(std::make_tuple(device, true, i));
    }

    return devices;
}

void printNvidiaHeader()
{
    std::cout << InformationMsg<std::string>("* ") << WhiteMsg<std::string>("NVIDIA DEVICES", 25);

    int numberDevices = getDeviceCount();

    if (numberDevices == 0)
    {
        std::cout << WarningMsg<std::string>("None found") << std::endl;
        return;
    }

    std::map<std::string, std::vector<cudaDeviceProp>> gpus;

    for (int i = 0; i < numberDevices; i++)
    {
        cudaDeviceProp prop;
        throw_on_cuda_error(cudaGetDeviceProperties(&prop, i), __FILE__, __LINE__);

        std::string deviceName = prop.name;

        gpus[deviceName].push_back(prop);
    }

    int i = 0;

    for (const auto gpu : gpus)
    {
        const auto name = gpu.first;
        const auto properties = gpu.second;

        if (i != 0)
        {
            std::cout << ", ";
        }

        size_t numDevices = properties.size();

        std::cout << SuccessMsg<size_t>(numDevices) << SuccessMsg<std::string>("x ") << SuccessMsg<std::string>(name);

        i++;
    }

    std::cout << std::endl;
}
