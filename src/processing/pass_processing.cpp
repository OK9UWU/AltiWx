#include "pass_processing.h"
#include "logger/logger.h"
#include "config/config.h"
#include "downlink_recorder.h"
#include "dsp/dsp_manager.h"
#include <filesystem>
#include "sol/sol.hpp"

void debug(std::string log)
{
    logger->debug("[Lua] " + log);
}

void info(std::string log)
{
    logger->info("[Lua] " + log);
}

void warn(std::string log)
{
    logger->warn("[Lua] " + log);
}

void error(std::string log)
{
    logger->error("[Lua] " + log);
}

void critical(std::string log)
{
    logger->critical("[Lua] " + log);
}

bool os_exists(std::string file)
{
    return std::filesystem::exists(file);
}

std::string generateFilepath(SatellitePass &satellitePass, SatelliteConfig &satelliteConfig, DownlinkConfig &downlinkConfig)
{
    std::string workdDir = configManager->getConfig().dataDirectory + "/" + satelliteConfig.getName() + "/" + downlinkConfig.name;

    std::filesystem::create_directories(workdDir);

    std::tm *timeReadable = gmtime(&satellitePass.aos);

    return workdDir + "/" +
           satelliteConfig.getName() + "_" + downlinkConfig.name + "_" + std::to_string(timeReadable->tm_year + 1900) +
           "-" + std::to_string(timeReadable->tm_mon) + "-" + std::to_string(timeReadable->tm_mday) +
           "--" + std::to_string(timeReadable->tm_hour) + ":" + (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));
}

void processPass(SatellitePass pass)
{
    SatelliteConfig satelliteConfig = configManager->getConfig().getSatelliteConfigFromNORAD(pass.norad);
    logger->info("AOS " + pass.tle.name);

    std::unordered_map<std::string, std::pair<std::string, std::string>> filePaths;

    std::vector<std::shared_ptr<DownlinkRecorder>> downlinkRecoders;
    for (DownlinkConfig &downlinkConfig : satelliteConfig.downlinkConfigs)
    {
        logger->debug("Adding recorder for " + downlinkConfig.name + " downlink on " + std::to_string(downlinkConfig.frequency) + " Hz");

        std::string fileName = generateFilepath(pass, satelliteConfig, downlinkConfig);
        std::string filePath = fileName + +"." + downlinkConfig.outputExtension;
        filePaths.emplace(std::make_pair(downlinkConfig.postProcessingScript, std::make_pair(fileName, filePath)));
        logger->debug("Using file path " + filePath);

        std::shared_ptr<DownlinkRecorder> recorder = std::make_shared<DownlinkRecorder>(rtlDSP, downlinkConfig, satelliteConfig, filePath);
        downlinkRecoders.push_back(recorder);
    }

    for (std::shared_ptr<DownlinkRecorder> &recorder : downlinkRecoders)
        recorder->start();

    while (time(NULL) <= pass.los)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    for (std::shared_ptr<DownlinkRecorder> &recorder : downlinkRecoders)
        recorder->stop();

    logger->info("Processing data for " + satelliteConfig.getName());

    std::vector<std::string> finalFiles;
    for (std::pair<std::string, std::pair<std::string, std::string>> fileToProcess : filePaths)
    {
        logger->debug("Processing " + fileToProcess.second.first + " with " + fileToProcess.first);

        if (!std::filesystem::exists("scripts/" + fileToProcess.first))
        {
            logger->critical("Script " + (std::string) "scripts/" + fileToProcess.first + " does not exist!");
            continue;
        }

        try
        {
            sol::state lua;
            lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::os, sol::lib::io, sol::lib::string);
            lua["filename"] = fileToProcess.second.first;
            lua["input_file"] = fileToProcess.second.second;
            lua.new_usertype<spdlog::logger>("spdlogger", "debug", &debug, "info", &info, "warn", &warn, "error", &error, "critical", &critical);
            lua.set_function("file_exists", &os_exists);
            lua["logger"] = logger;
            lua.script_file("scripts/" + fileToProcess.first);
            finalFiles.push_back((std::string)lua["output_file"]);
        } catch (std::exception& e) {
            logger->error(e.what());
        }
    }

    logger->info("LOS " + pass.tle.name);
}