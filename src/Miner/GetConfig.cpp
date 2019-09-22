// Copyright (c) 2019, Zpalmtree
//
// Please see the included LICENSE file for more information.

////////////////////////////
#include "Miner/GetConfig.h"
////////////////////////////

#include <fstream>

#include "Config/Constants.h"
#include "ExternalLibs/cxxopts.hpp"
#include "ExternalLibs/json.hpp"
#include "Utilities/ColouredMsg.h"
#include "Utilities/Console.h"
#include "Utilities/Input.h"
#include "Utilities/String.h"

#if defined(X86_OPTIMIZATIONS)
#include "cpu_features/include/cpuinfo_x86.h"
#endif

#if defined(NVIDIA_ENABLED)
#include "Backend/Nvidia/NvidiaUtils.h"
#else
std::vector<std::tuple<std::string, bool, int>> getNvidiaDevicesActual() { return {}; }
#endif

#if defined(AMD_ENABLED)
#include "MinerManager/Amd/AmdManager.h"
#else
std::vector<AmdDevice> getAmdDevices() { return {}; }
#endif

std::vector<NvidiaDevice> getNvidiaDevices()
{
    std::vector<NvidiaDevice> devices;

    /* We could probably do some sort of struct/tuple initialization here.. */
    for (const auto &[name, enabled, id] : getNvidiaDevicesActual())
    {
        NvidiaDevice device;

        device.name = name;
        device.enabled = enabled;
        device.id = id;

        devices.push_back(device);
    }

    return devices;
}

void to_json(nlohmann::json &j, const CpuConfig &config)
{
    j = {
        {"enabled", config.enabled},
        {"optimizationMethod", Constants::optimizationMethodToString(config.optimizationMethod)},
        {"threadCount", config.threadCount}
    };
}

void from_json(const nlohmann::json &j, CpuConfig &config)
{
    if (j.find("enabled") != j.end())
    {
        config.enabled = j.at("enabled").get<bool>();
    }
    else
    {
        config.enabled = true;
    }

    if (j.find("threadCount") != j.end())
    {
        config.threadCount = j.at("threadCount").get<uint32_t>();
    }

    if (j.find("optimizationMethod") != j.end())
    {
        const auto optimizations = getAvailableOptimizations();

        config.optimizationMethod = Constants::optimizationMethodFromString(j.at("optimizationMethod").get<std::string>());

        const auto it = std::find(optimizations.begin(), optimizations.end(), config.optimizationMethod);

        if (it == optimizations.end())
        {
            throw std::invalid_argument("Optimization " + Constants::optimizationMethodToString(config.optimizationMethod) + " is unavailable for your hardware.");
        }
    }
    else
    {
        config.optimizationMethod = Constants::AUTO;
    }
}

void to_json(nlohmann::json &j, const NvidiaDevice &device)
{
    j = {
        {"enabled", device.enabled},
        {"name", device.name},
        {"id", device.id},
        {"intensity", device.intensity},
        {"desktopLag", device.desktopLag}
    };
}

void from_json(const nlohmann::json &j, NvidiaDevice &device)
{
    if (j.find("enabled") != j.end())
    {
        device.enabled = j.at("enabled").get<bool>();
    }
    else
    {
        device.enabled = true;
    }

    device.name = j.at("name").get<std::string>();
    device.id = j.at("id").get<uint16_t>();

    if (j.find("intensity") != j.end())
    {
        device.intensity = j.at("intensity").get<float>();

        if (device.intensity < 0.0 || device.intensity > 100.0)
        {
            throw std::invalid_argument("Intensity value of " + std::to_string(device.intensity) + " is invalid. Must be between 0.0 and 100.0");
        }
    }

    if (j.find("desktopLag") != j.end())
    {
        device.desktopLag = j.at("desktopLag").get<float>();

        if (device.desktopLag < 0.0 || device.desktopLag > 100.0)
        {
            throw std::invalid_argument("Desktop lag value of " + std::to_string(device.desktopLag) + " is invalid. Must be between 0.0 and 100.0");
        }
    }
}

void to_json(nlohmann::json &j, const AmdDevice &device)
{
    j = {
        {"enabled", device.enabled},
        {"name", device.name},
        {"id", device.id},
        {"intensity", device.intensity},
        {"desktopLag", device.desktopLag}
    };
}

void from_json(const nlohmann::json &j, AmdDevice &device)
{
    if (j.find("enabled") != j.end())
    {
        device.enabled = j.at("enabled").get<bool>();
    }
    else
    {
        device.enabled = true;
    }

    device.name = j.at("name").get<std::string>();
    device.id = j.at("id").get<uint16_t>();

    if (j.find("intensity") != j.end())
    {
        device.intensity = j.at("intensity").get<float>();

        if (device.intensity < 0.0 || device.intensity > 100.0)
        {
            throw std::invalid_argument("Intensity value of " + std::to_string(device.intensity) + " is invalid. Must be between 0.0 and 100.0");
        }
    }

    if (j.find("desktopLag") != j.end())
    {
        device.desktopLag = j.at("desktopLag").get<float>();

        if (device.desktopLag < 0.0 || device.desktopLag > 100.0)
        {
            throw std::invalid_argument("Desktop lag value of " + std::to_string(device.desktopLag) + " is invalid. Must be between 0.0 and 100.0");
        }
    }
}

void to_json(nlohmann::json &j, const NvidiaConfig &config)
{
    j = {
        {"devices", config.devices}
    };
}

void from_json(const nlohmann::json &j, NvidiaConfig &config)
{
    if (j.find("devices") != j.end())
    {
        config.devices = j.at("devices").get<std::vector<NvidiaDevice>>();
    }
    else
    {
        config.devices = getNvidiaDevices();
    }
}

void to_json(nlohmann::json &j, const AmdConfig &config)
{
    j = {
        {"devices", config.devices}
    };
}

void from_json(const nlohmann::json &j, AmdConfig &config)
{
    if (j.find("devices") != j.end())
    {
        config.devices = j.at("devices").get<std::vector<AmdDevice>>();
    }
    else
    {
        config.devices = getAmdDevices();
    }
}

void to_json(nlohmann::json &j, const HardwareConfig &config)
{
    j = {
        {"cpu", config.cpu},
        {"nvidia", config.nvidia},
        /*{"amd", config.amd}*/
    };
}

void from_json(const nlohmann::json &j, HardwareConfig &config)
{
    if (j.find("cpu") != j.end())
    {
        config.cpu = j.at("cpu").get<CpuConfig>();
    }
    else
    {
        /* Default is fine for CPU right now */
    }

    if (j.find("nvidia") != j.end())
    {
        config.nvidia = j.at("nvidia").get<NvidiaConfig>();
    }
    else
    {
        config.nvidia.devices = getNvidiaDevices();
    }

    if (j.find("amd") != j.end())
    {
        config.amd = j.at("amd").get<AmdConfig>();
    }
    else
    {
        config.amd.devices = getAmdDevices();
    }
}


void to_json(nlohmann::json &j, const MinerConfig &config)
{
    j = {
        {"pools", config.pools},
        {"hardwareConfiguration", *(config.hardwareConfiguration)}
    };
}

void from_json(const nlohmann::json &j, MinerConfig &config)
{
    config.pools = j.at("pools").get<std::vector<Pool>>();

    if (j.find("hardwareConfiguration") != j.end())
    {
        *config.hardwareConfiguration = j.at("hardwareConfiguration").get<HardwareConfig>();
    }
    else
    {
        config.hardwareConfiguration->nvidia.devices = getNvidiaDevices();
        config.hardwareConfiguration->amd.devices = getAmdDevices();
    }
}

Constants::OptimizationMethod getAutoChosenOptimization()
{
    auto best = getAvailableOptimizations()[0];

    if (best == Constants::AUTO)
    {
        best = Constants::NONE;
    }

    #if defined(ARMV8_OPTIMIZATIONS)
    /* We don't enable NEON optimizations by default on Armv8: https://github.com/weidai11/cryptopp/issues/367 */
    if (best == Constants::NEON)
    {
        best = Constants::NONE;
    }
    #endif

    return best;
}

std::vector<Constants::OptimizationMethod> getAvailableOptimizations()
{
    std::vector<Constants::OptimizationMethod> availableOptimizations;

    #if defined(X86_OPTIMIZATIONS)

    static const cpu_features::X86Features features = cpu_features::GetX86Info().features;

    if (features.avx512f)
    {
        availableOptimizations.push_back(Constants::AVX512);
    }

    if (features.avx2)
    {
        availableOptimizations.push_back(Constants::AVX2);
    }

    if (features.sse4_1)
    {
        availableOptimizations.push_back(Constants::SSE41);
    }

    if (features.ssse3)
    {
        availableOptimizations.push_back(Constants::SSSE3);
    }

    if (features.sse2)
    {
        availableOptimizations.push_back(Constants::SSE2);
    }

    #elif defined(ARMV8_OPTIMIZATIONS)

    availableOptimizations.push_back(Constants::NEON);

    #endif

    availableOptimizations.push_back(Constants::AUTO);
    availableOptimizations.push_back(Constants::NONE);
    
    return availableOptimizations;
}

Pool getPool()
{
    Pool pool;

    while (true)
    {
        std::cout << InformationMsg("Enter the pool address to mine to.") << std::endl
                  << InformationMsg("This should look something like trtl.pool.mine2gether.com:3335: ");

        std::string address;
        std::string host;
        uint16_t port;

        std::getline(std::cin, address);

        Utilities::trim(address);

        if (address == "")
        {
            continue;
        }

        if (!Utilities::parseAddressFromString(host, port, address))
        {
            std::cout << WarningMsg("Invalid pool address! Should be in the form host:port, for example, trtl.pool.mine2gether.com:3335!")
                      << std::endl;

            continue;
        }

        pool.host = host;
        pool.port = port;

        break;
    }

    while (true)
    {
        std::cout << InformationMsg("\nEnter your pool login. This is usually your wallet address: ");

        std::string login;

        std::getline(std::cin, login);

        Utilities::trim(login);

        if (login == "")
        {
            std::cout << WarningMsg("Login cannot be empty! Try again.") << std::endl;
            continue;
        }

        pool.username = login;

        break;
    }

    std::cout << InformationMsg("\nEnter the pool password. You can usually leave this blank, or use 'x': ");

    std::string password;

    std::getline(std::cin, password);

    pool.password = password;

    while (true)
    {
        std::cout << InformationMsg("\nAvailable mining algorithms:") << std::endl;
        int i = 1;
        for (const auto [algorithmName, algoEnum, shouldDisplay] : ArgonVariant::algorithmNameMapping)
        {
            /* We don't print every single alias because it would get a little silly. */
            if (shouldDisplay)
            {
                std::cout << SuccessMsg("(") << SuccessMsg(i) << SuccessMsg (") ") << SuccessMsg(algorithmName) << std::endl;
                i++;
            }
        }

        std::cout << InformationMsg("\nEnter the algorithm you wish to mine with on this pool: ");

        std::string algorithm;

        std::getline(std::cin, algorithm);

        int algorithmNumber;
        bool algorithmIsNumber;

        if (algorithm == "")
        {
            continue;
        }

        try
        {
            algorithmNumber = std::stoi(algorithm);
            algorithmIsNumber = true;
        }
        catch (const std::exception &)
        {
            algorithmIsNumber = false;
        }

        if (algorithmIsNumber) 
        {
            switch(algorithmNumber)
            {
                case 1:
                    algorithm = "chukwa";
                    break;
                case 2:
                    algorithm = "turtlecoin";
                    break;
                case 3:
                    algorithm = "chukwa_wrkz";
                    break;
                case 4:
                    algorithm = "wrkzcoin";
                    break;
                default:
                    algorithm = "turtlecoin";
                    break;
            }
        } 

        try
        {
            ArgonVariant::algorithmNameToCanonical(algorithm);
            pool.algorithm = algorithm;
            break;
        }
        catch (const std::exception &)
        {
            std::cout << WarningMsg("Unknown algorithm \"" + algorithm + "\". Try again.") << std::endl;
        }
    }

    std::cout << InformationMsg("\nEnter the rig ID to use with this pool. This can identify your different computers to the pool.") << std::endl
              << InformationMsg("You can leave this blank if desired: ");

    std::string rigID;

    std::getline(std::cin, rigID);

    pool.rigID = rigID;

    return pool;
}

std::vector<Pool> getPools()
{
    std::vector<Pool> pools;

    int i = 0;

    while (true)
    {
        Pool pool = getPool();
        pool.priority = i;

        pools.push_back(pool);

        if (!Utilities::confirm("\nWould you like to add another pool?", false))
        {
            break;
        }

        std::cout << std::endl;

        i++;
    }

    return pools;
}

void writeConfigToDisk(MinerConfig config)
{
    std::ofstream configFile(Constants::CONFIG_FILE_NAME);

    nlohmann::json j = config;

    if (configFile)
    {
        configFile << j.dump(4) << std::endl;
    }
    else
    {
        std::cout << WarningMsg("Failed to write config to disk. Please check that the program can write to the folder you launched it from.")
                  << std::endl << std::endl
                  << "Config:" << std::endl << j.dump(4) << std::endl;
    }
}

MinerConfig getConfigInteractively()
{
    MinerConfig config;

    config.pools = getPools();
    config.hardwareConfiguration->nvidia.devices = getNvidiaDevices();
    config.hardwareConfiguration->amd.devices = getAmdDevices();
    config.hardwareConfiguration->cpu.enabled = true;
    config.hardwareConfiguration->cpu.optimizationMethod = Constants::AUTO;
    config.interactive = true;

    writeConfigToDisk(config);

    return config;
}

MinerConfig getConfigFromJSON(const std::string &configLocation)
{
    std::ifstream configFile(configLocation);

    if (!configFile)
    {
        std::stringstream stream;

        stream << "Failed to open config file \"" << configLocation
               << "\"." << std::endl << "Does the file exist?" << std::endl;

        std::cout << WarningMsg(stream.str());

        Console::exitOrWaitForInput(1);
    }

    try
    {
        std::string fileContents((std::istreambuf_iterator<char>(configFile)),
                                 (std::istreambuf_iterator<char>()));

        const MinerConfig jsonConfig = nlohmann::json::parse(fileContents);

        writeConfigToDisk(jsonConfig);

        return jsonConfig;
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cout << WarningMsg("Failed to parse config file: ")
                  << WarningMsg(e.what()) << std::endl
                  << "Try pasting your config file (" << configLocation << ") into "
                  << InformationMsg("https://jsonformatter.curiousconcept.com/")
                  << " to figure out which line is invalid." << std::endl;

        Console::exitOrWaitForInput(1);
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << WarningMsg("Config file is invalid: ")
                  << WarningMsg(e.what()) << std::endl;

        Console::exitOrWaitForInput(1);
    }
    catch (const std::exception &e)
    {
        std::cout << WarningMsg("Failed to read from config file: ")
                  << WarningMsg(e.what()) << std::endl;

        Console::exitOrWaitForInput(1);
    }

    /* Compiler can't figure out that Console::exitOrWaitForInput() exits the
       program */
    throw std::runtime_error("Programmer error");
}

MinerConfig getMinerConfig(int argc, char **argv)
{
    MinerConfig config;
    Pool poolConfig;

    std::string poolAddress;

    bool help;
    bool version;
    bool disableCPU;
    bool disableNVIDIA;
    bool disableAMD;

    cxxopts::Options options(argv[0], "");

    options.add_options("Core")
        ("h,help", "Display this help message",
         cxxopts::value<bool>(help)->implicit_value("true"))

        ("v,version", "Display the miner version",
         cxxopts::value<bool>(version)->implicit_value("true"))

        ("config", "The location of the config file to use",
         cxxopts::value<std::string>(config.configLocation), "<file>");

    options.add_options("Pool")
        ("pool", "The pool <host:port> combination to mine to",
         cxxopts::value<std::string>(poolAddress), "<host:port>")

        ("username", "The username to use with the pool, normally your wallet address",
         cxxopts::value<std::string>(poolConfig.username), "<username>")

        ("password", "The password to use with the pool, can be omitted",
         cxxopts::value<std::string>(poolConfig.password), "<password>")

        ("rigid", "The rig ID to use with the pool, can be omitted",
         cxxopts::value<std::string>(poolConfig.rigID), "<rig ID>");

    options.add_options("Miner")
        ("algorithm", "The mining algorithm to use",
         cxxopts::value<std::string>(poolConfig.algorithm), "<algorithm>")

        ("threads", "The number of mining threads to use",
         cxxopts::value<uint32_t>(config.hardwareConfiguration->cpu.threadCount)->default_value(
            std::to_string(config.hardwareConfiguration->cpu.threadCount)), "<threads>")

        ("disableCPU", "Disable CPU mining",
         cxxopts::value<bool>(disableCPU)->implicit_value("true"))

        ("disableNVIDIA", "Disable Nvidia mining",
         cxxopts::value<bool>(disableNVIDIA)->implicit_value("true"))

        ("disableAMD", "Disable AMD mining",
         cxxopts::value<bool>(disableAMD)->implicit_value("true"));

    try
    {
        const auto result = options.parse(argc, argv);

        if (help)
        {
            std::cout << options.help({}) << std::endl;
            exit(0);
        }

        if (version)
        {
            std::cout << "violetminer " << Constants::VERSION << std::endl;
            exit(0);
        }

        const bool configFileExists = static_cast<bool>(std::ifstream(Constants::CONFIG_FILE_NAME));

        /* Use config file if no args given and config file exists on disk */
        if (configFileExists && result.arguments().size() == 0)
        {
            config.configLocation = Constants::CONFIG_FILE_NAME;
        }

        /* If we have been given a config file, or no args are given and config
           file exists on disk */
        if (config.configLocation != "")
        {
            return getConfigFromJSON(config.configLocation);
        }
        /* No command line args given, and no config on disk, create config from
           user input */
        else if (result.arguments().size() == 0)
        {
            return getConfigInteractively();
        }
        else
        {
            const std::vector<std::string> requiredArgs { "pool", "username", "algorithm" };

            for (const auto &arg : requiredArgs)
            {
                if (result.count(arg) == 0)
                {
                    std::cout << WarningMsg("Required argument --" + arg + " has not been supplied!") << std::endl;
                    Console::exitOrWaitForInput(1);
                }
            }

            if (!Utilities::parseAddressFromString(poolConfig.host, poolConfig.port, poolAddress))
            {
                std::cout << WarningMsg("Failed to parse pool address!") << std::endl;
                Console::exitOrWaitForInput(1);
            }

            if (poolConfig.username == "")
            {
                std::cout << WarningMsg("Username cannot be empty!") << std::endl;
                Console::exitOrWaitForInput(1);
            }

            try
            {
                ArgonVariant::algorithmNameToCanonical(poolConfig.algorithm);
            }
            catch (const std::exception &)
            {
                std::cout << WarningMsg("Algorithm \"" + poolConfig.algorithm + "\" is not a known algorithm!") << std::endl;

                std::cout << InformationMsg("Available mining algorithms:") << std::endl;

                for (const auto [algorithmName, algoEnum, shouldDisplay] : ArgonVariant::algorithmNameMapping)
                {
                    /* We don't print every single alias because it would get a little silly. */
                    if (shouldDisplay)
                    {
                        std::cout << SuccessMsg("* ") << SuccessMsg(algorithmName) << std::endl;
                    }
                }

                Console::exitOrWaitForInput(1);
            }

            config.pools.push_back(poolConfig);
            config.hardwareConfiguration->nvidia.devices = getNvidiaDevices();
            config.hardwareConfiguration->amd.devices = getAmdDevices();
            config.hardwareConfiguration->cpu.enabled = true;
            config.hardwareConfiguration->cpu.optimizationMethod = Constants::AUTO;

            if (disableCPU)
            {
                config.hardwareConfiguration->cpu.enabled = false;
            }

            if (disableNVIDIA)
            {
                for (auto &device : config.hardwareConfiguration->nvidia.devices)
                {
                    device.enabled = false;
                }
            }

            if (disableAMD)
            {
                for (auto &device : config.hardwareConfiguration->amd.devices)
                {
                    device.enabled = false;
                }
            }

            return config;
        }
    }
    catch (const cxxopts::OptionException &e)
    {
        std::cout << WarningMsg("Error: Unable to parse command line options: ") << WarningMsg(e.what())
                  << std::endl << std::endl
                  << options.help({}) << std::endl;

        Console::exitOrWaitForInput(1);
    }

    /* Compiler can't figure out that Console::exitOrWaitForInput() exits the
       program */
    throw std::runtime_error("Programmer error");
}
