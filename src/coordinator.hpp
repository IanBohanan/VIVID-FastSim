#pragma once

#include <iostream>
#include <iterator>
#include <string>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>
#include <ctime>
#include <csignal>
#include <optional>

struct PlcData {
    std::string addressVar;
    std::string name;
    int address;
    std::optional<int> value;
};

struct PlcConfig {
    std::string name;
    std::string ipAddress;
    std::vector<PlcData> digitalInputs;
    std::vector<PlcData> digitalOutputs;
    std::vector<PlcData> analogInputs;
    std::vector<PlcData> analogOutputs;
};

struct MacvlanConfig {
    std::string subnet;
    std::string gateway;
    std::string parent;
};

struct HmiConfig {
    std::string name;
    std::string ipAddress;
    double pollingRateSec;
    bool hmiConsoleLogging;
};

struct AttackScenarioConfig {
    bool runAttackScenario;
    std::optional<std::string> path;
    std::optional<double> scenarioStartSimTime;
};

struct Config {
    std::string modelFile;
    std::string simName;
    std::string stFile;
    double timestepSizeSec;
    double runtimeSimSec;
    bool matlabLogging;
    bool consoleLogging;
    int maxSemWaitMS;
    std::vector<PlcConfig> plcs;
    MacvlanConfig macvlanConfig;
    HmiConfig hmiConfig;
    AttackScenarioConfig attackConfig;
};