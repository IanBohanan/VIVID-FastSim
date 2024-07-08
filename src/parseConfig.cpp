#include "parseConfig.hpp"
#include "coordinator.hpp"

#include <fstream>
#include <iostream>

Config parseConfig(std::string configPath) 
{	
    std::ifstream configFile(configPath);
    std::string const configString = std::string((std::istreambuf_iterator<char>(configFile)), std::istreambuf_iterator<char>());
    auto jsonConfig = boost::json::parse(configString).get_object();

    Config config;
    config.modelFile = jsonConfig["modelFile"].as_string();
    config.simName = jsonConfig["simName"].as_string();
    config.stFile = jsonConfig["stFile"].as_string();
    config.timestepSizeSec = jsonConfig["timestepSizeSec"].as_double();
    config.runtimeSimSec = jsonConfig["runtimeSimSec"].as_double();
    config.matlabLogging = jsonConfig["matlabLogging"].as_bool();
    config.consoleLogging = jsonConfig["consoleLogging"].as_bool();
    config.maxSemWaitMS = jsonConfig["maxSemWaitMS"].is_int64() ? jsonConfig["maxSemWaitMS"].as_int64() : 0;
    boost::json::object macvlanConfig = jsonConfig["macvlan"].as_object();
    config.macvlanConfig.gateway = macvlanConfig["gateway"].as_string();
    config.macvlanConfig.subnet = macvlanConfig["subnet"].as_string();
    config.macvlanConfig.parent = macvlanConfig["parent"].as_string();
    boost::json::object hmiConfig = jsonConfig["hmi"].as_object();
    config.hmiConfig.name = hmiConfig["name"].as_string();
    config.hmiConfig.ipAddress = hmiConfig["ipAddress"].as_string();
    config.hmiConfig.pollingRateSec = hmiConfig["pollingRateSec"].as_double();
    config.hmiConfig.hmiConsoleLogging = hmiConfig["hmiConsoleLogging"].as_bool();
    if (!jsonConfig["attackScenario"].is_null()) {
        config.attackConfig.runAttackScenario = true;
        boost::json::object attackConfig = jsonConfig["attackScenario"].as_object();
        config.attackConfig.path = attackConfig["path"].as_string();
        config.attackConfig.scenarioStartSimTime = attackConfig["scenarioStartSimTime"].as_double();
    } else config.attackConfig.runAttackScenario = false;
    
    for (auto plc : jsonConfig["plcs"].as_array()) {
        PlcConfig plcConfig;
        plcConfig.name = plc.as_object()["name"].as_string();
        plcConfig.ipAddress = plc.as_object()["ipAddress"].as_string();
        for (auto digitalInput : plc.as_object()["digitalInputs"].as_array()) {
            PlcData data;
            data.address = digitalInput.as_object()["address"].as_int64();
            data.addressVar = digitalInput.as_object()["addressVar"].as_string();
            data.name = digitalInput.as_object()["name"].as_string();
            if (!digitalInput.as_object()["value"].is_null()) data.value = digitalInput.as_object()["value"].as_int64();
            plcConfig.digitalInputs.push_back(data);
        }
        for (auto digitalOutput : plc.as_object()["digitalOutputs"].as_array()) {
            PlcData data;
            data.address = digitalOutput.as_object()["address"].as_int64();
            data.addressVar = digitalOutput.as_object()["addressVar"].as_string();
            data.name = digitalOutput.as_object()["name"].as_string();
            if (!digitalOutput.as_object()["value"].is_null()) data.value = digitalOutput.as_object()["value"].as_int64();
            plcConfig.digitalOutputs.push_back(data);
        }
        for (auto analogInput : plc.as_object()["analogInputs"].as_array()) {
            PlcData data;
            data.address = analogInput.as_object()["address"].as_int64();
            data.addressVar = analogInput.as_object()["addressVar"].as_string();
            data.name = analogInput.as_object()["name"].as_string();
            if (!analogInput.as_object()["value"].is_null()) data.value = analogInput.as_object()["value"].as_int64();
            plcConfig.analogInputs.push_back(data);
        }
        for (auto analogOutput : plc.as_object()["analogOutputs"].as_array()) {
            PlcData data;
            data.address = analogOutput.as_object()["address"].as_int64();
            data.addressVar = analogOutput.as_object()["addressVar"].as_string();
            data.name = analogOutput.as_object()["name"].as_string();
            if (!analogOutput.as_object()["value"].is_null()) data.value = analogOutput.as_object()["value"].as_int64();
            plcConfig.analogOutputs.push_back(data);
        }
        config.plcs.push_back(plcConfig);
    }
    return config;
}
