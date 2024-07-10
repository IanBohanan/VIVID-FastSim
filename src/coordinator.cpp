#include "coordinator.hpp"
#include "parseConfig.hpp"
#include "prettyPrintJson.hpp"
#include "shmSetup.hpp"

#include <boost/json/src.hpp>
#include "MatlabEngine.hpp"
#include "MatlabDataArray.hpp"

using namespace matlab::engine;

std::unique_ptr<MATLABEngine> matlabPtr;
matlab::data::ArrayFactory factory;
std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;

std::string outputPath;
std::ofstream outputFile;
boost::json::object hardwareLayerLog;
auto simStartTime = std::chrono::system_clock::now();
auto startTime = std::chrono::system_clock::now();
bool lastRun = false;
double simTime = 0.0;  // sim time of last update in sec

Config config;
std::map<std::string, ShmData*> plcShmDataMap;
ShmData* hmiShmData;
std::map<std::string, bool> plcStatusMap;  // true: plc is responding; false: plc unresponsive

void initialize(std::string projectPath, std::string modelPath) {
    // Setup shared memeory with plc containers 
    for (auto plc : config.plcs) {
        std::string shmPath = "/" + plc.name; // actually maps to /dev/shm/<plcName>
        plcShmDataMap[plc.name] = createShm(shmPath);
        // Now start openplc
        // The following line of code is not needed as long as the new start_openplc.sh file has been pushed to github and you've rebuilt your docker image. 
        //std::string commmand = "docker cp /home/vagrant/IAN/start_openplc.sh " + plc.name + ":/FastSim/start_openplc.sh";
        std::system(commmand.c_str());
        std::string command = "docker exec -w /FastSim " + plc.name + " bash ./start_openplc.sh " + "/FastSim/models/" + config.simName + "/" + config.stFile + " &";
        std::system(command.c_str());
        plcStatusMap[plc.name] = true;
    }

    // Setup shared memory with HMI
    std::string hmiShmPath = "/hmi"; // actually maps to /dev/shm/hmi
    hmiShmData = createShm(hmiShmPath);
    // Start hmi
    std::string command = "docker exec -w /FastSim/src/hmi " + config.hmiConfig.name + " bash ./start_hmi.sh &";
    std::system(command.c_str());

    std::u16string PROJECT_PATH = convert.from_bytes(projectPath);
    std::u16string MODEL_PATH = convert.from_bytes(modelPath);
    // Start Matlab 
    std::cout << "Creating malabPtr..." << std::endl;
    matlabPtr = startMATLAB();
    // Turn off matlab warnings so they don't spam the logs
    matlabPtr->eval(u"warning('off')");
    std::cout << "Loading Simulink model..." << std::endl;   
    // Load simulink sim
    matlabPtr->eval(u"modelHandle = load_system('" + MODEL_PATH + convert.from_bytes((config.modelFile)) + u"');");
    std::cout << "Model loaded" << std::endl;
    const auto modelHandle = matlabPtr->getVariable(u"modelHandle");
    std::cout << "Retrieved modelHandle" << std::endl;

    // Setup model inputs
    std::string ml = "simInput = struct("; 
    for (PlcConfig plcConfig : config.plcs) {
        for (PlcData digitalOutput : plcConfig.digitalOutputs) { 
            ml = ml + "'" + digitalOutput.name + "', " + std::to_string(digitalOutput.value.has_value() ? digitalOutput.value.value() : 0) + ",";
        }
        for (PlcData analogOutput : plcConfig.analogOutputs) {
            ml = ml + "'" + analogOutput.name + "', " + std::to_string(analogOutput.value.has_value() ? analogOutput.value.value() : 0) + ",";
        }
    }
    if (ml.back() == ',') ml.back() = ' ';
    ml = ml + std::string(");");
    matlabPtr->eval(convert.from_bytes(ml));
    std::cout << "MATLAB struct created" << std::endl;
    std::cout << ml << std::endl;

    // Check if sim has PauseSim block
    const auto hasPauseSim = matlabPtr->feval(u"getSimulinkBlockHandle", { factory.createScalar(convert.from_bytes(config.simName) + u"/PauseSim") });
    if (int(hasPauseSim[0]) <= 0) {
        std::cout << "Adding Pause Sim Block" << std::endl;
        // Add Simulink pause
        matlabPtr->eval(u"add_block('simulink/Ports & Subsystems/Subsystem Reference', " + convert.from_bytes(config.simName) + u"'/PauseSim', 'ReferencedSubsystem', 'simulation_pause_subsystem');");
        std::cout << "added Pause Sim Block" << std::endl;
    }
    
    // Set sim timestep in Pause block
    matlabPtr->eval(u"load_system('" + PROJECT_PATH + u"models/simulation_pause_subsystem.slx'); set_param('simulation_pause_subsystem/PauseSim/Timestep_sec', 'Value', '" + convert.from_bytes(std::to_string(config.timestepSizeSec)) + u"');");
    matlabPtr->eval(u"isPaused = false;");

    // Setup matlab logging
    matlabPtr->eval(u"isLogging = " + convert.from_bytes(config.matlabLogging ? "1" : "0") + u";");
    if (config.matlabLogging) {
        matlabPtr->eval(u"outputFile = fopen('" + convert.from_bytes(outputPath) + u"/matlabOutput.json', 'wt'); fprintf(outputFile, '[');");
    }
}

int main(int argc, char *argv[]) {
    // Expects one argument with the path to FastSim e.g., /home/whitney/Desktop/FastSim
    std::string projectPath = std::string(argv[1]);
    std::string modelPath = std::string(argv[2]);
    outputPath = std::string(argv[3]);
    if (projectPath.length() == 0 || modelPath.length() == 0 || outputPath.length() == 0) {
        std::cout << "Project root path and model path required as input!" << std::endl;
        exit(1);
    }

    // Parse JSON config file
    config = parseConfig(modelPath + "/config.json");

    // Initialize log file
    outputFile.open(outputPath + "/output.json");
    outputFile << "[\n";
    
    initialize(projectPath, modelPath);

    // Start simulink
    matlabPtr->eval(u"set_param(modelHandle, 'SimulationCommand', 'start');");
    std::cout << "Simulation running" << std::endl;
    startTime = std::chrono::system_clock::now();

    // Sim Loop
    bool stop = false;
    bool attackComplete = false;
    while (!stop) {
        simStartTime = std::chrono::system_clock::now();
        // Check if sim is paused
        matlab::data::TypedArray<bool> isPaused = matlabPtr->getVariable(u"isPaused");
       
        if (isPaused[0] == 1) {
            // Get sim output
            matlab::data::StructArray simOutput = matlabPtr->getVariable(u"output");
          
            // Get sim time 
            matlab::data::TypedArrayRef<double> time = simOutput[0]["simTime"];
            simTime = (double)time[0];

            // Check for attack scenario
            if (config.attackConfig.runAttackScenario && !attackComplete) {
                int simTimeInt = simTime * 1000;
                int attackTime = config.attackConfig.scenarioStartSimTime.value() * 1000;
                if (attackTime <= simTimeInt) {
                    attackComplete = true;
                    // Start run_attack.py script with project path and attack scenario path
                    std::string command = "/bin/python3 ./src/scripts/run_attack.py " + projectPath + " " + modelPath + config.attackConfig.path.value() + " " + outputPath + " " + modelPath + " &";
                    std::system(command.c_str());
                }
            }

            // Set sim time for HMI
            hmiShmData->simTime = simTime;
            
            // Check if sim time is past runtime
            if (simTime >= config.runtimeSimSec) {
                lastRun = true;
                stop = true;
                hmiShmData->lastRun = true;
            }
            hardwareLayerLog = {{ "simTime", std::to_string(simTime) }};

            // Get matlab data and update shmData for each plc container 
            for (auto plc : config.plcs) {
                if (lastRun) *(&plcShmDataMap[plc.name]->lastRun) = true;
                hardwareLayerLog[plc.name] = {};
               
                auto digitalInputs = plc.digitalInputs;
                for (int i = 0; i < digitalInputs.size(); i++) {
                    matlab::data::TypedArrayRef<double> data = simOutput[0][digitalInputs[i].name];
                    *(&plcShmDataMap[plc.name]->digitalInputs[i]) = (bool)data[0];
                    hardwareLayerLog[plc.name].as_object().insert(
                        std::make_pair(digitalInputs[i].name, (bool)data[0])
                    );
                }
                
                auto analogInputs = plc.analogInputs;
                for (int i = 0; i < analogInputs.size(); i++) {
                    matlab::data::TypedArrayRef<double> data = simOutput[0][analogInputs[i].name];
                    *(&plcShmDataMap[plc.name]->analogInputs[i]) = (uint16_t)data[0];
                    hardwareLayerLog[plc.name].as_object().insert(
                        std::make_pair(analogInputs[i].name, (uint16_t)data[0])
                    );
                }

                // Set host for this plc as ready
                if (sem_post(&plcShmDataMap[plc.name]->isHostReady) == -1) throw std::runtime_error("Host (" + plc.name + ") sem_post failed");
            }
            
            // Get PLCs output data and continue simulink
            const auto modelHandle = matlabPtr->getVariable(u"modelHandle");
            matlab::data::StructArray simInput = matlabPtr->getVariable(u"simInput");
            bool doUpdate = false;
            for (auto plc : config.plcs) {
                // Check if container is ready before waiting
                auto startWaitTime = std::chrono::system_clock::now();
                // -1: sem is locked, 0: we got the lock
                int semLocked = -1;
                if (config.maxSemWaitMS < 0) {
                    semLocked = sem_wait(&plcShmDataMap[plc.name]->isContainerReady);
                    if (semLocked == -1) throw std::runtime_error("Plc " + plc.name + " sem_wait failed");
                } else {
                    while (semLocked == -1) {
                        semLocked = sem_trywait(&plcShmDataMap[plc.name]->isContainerReady);
                        auto currentTime = std::chrono::system_clock::now();
                        auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startWaitTime);
                        if (timeDiff.count() > config.maxSemWaitMS) {
                            plcStatusMap[plc.name] = false;
                            break;
                        }
                    }
                }
                
                if (semLocked == 0) {
                    if (!plcStatusMap[plc.name]) {
                        std::cout << "A plc has been restarted, need to reconnect hmi to modbus" << std::endl;
                        // Reconnect hmi
                        hmiShmData->reconnectHmi = true;
                        plcStatusMap[plc.name] = true;
                    }
                    auto digitalOutputs = plc.digitalOutputs;
                    for (int i = 0; i < digitalOutputs.size(); i++) {
                        matlab::data::TypedArrayRef<double> data = simInput[0][digitalOutputs[i].name];
                        const auto newData = plcShmDataMap[plc.name]->digitalOutputs[i];
                        if (data[0] != newData) {
                            data[0] = newData;
                            doUpdate = true;
                        }
                        hardwareLayerLog[plc.name].as_object().insert(std::make_pair(digitalOutputs[i].name, newData));
                    }

                    auto analogOutputs = plc.analogOutputs;
                    for (int i = 0; i < analogOutputs.size(); i++) {
                        matlab::data::TypedArrayRef<double> data = simInput[0][analogOutputs[i].name];
                        const auto newData = plcShmDataMap[plc.name]->analogOutputs[i];
                        if (data[0] != newData) {
                            data[0] = newData;
                            doUpdate = true;
                        }
                        hardwareLayerLog[plc.name].as_object().insert(std::make_pair(analogOutputs[i].name, newData));
                    }
                }
            }

            if (sem_post(&hmiShmData->isHostReady) == -1) throw std::runtime_error("Host hmi sem_post failed");

            if (doUpdate) {
                matlabPtr->setVariable(u"simInput", simInput);
                matlabPtr->feval(u"set_param", 0, { modelHandle, factory.createScalar("SimulationCommand"), factory.createScalar("update") });
            }

            if (lastRun) {
                auto currentTime = std::chrono::system_clock::now();
                auto totalRuntime = currentTime - startTime;
                auto totalTime = currentTime - simStartTime;
                hardwareLayerLog.insert(std::make_pair("totalTime", std::chrono::duration_cast<std::chrono::milliseconds>(totalTime).count()));
                if (config.consoleLogging) {
                    prettyPrintJson(std::cout, hardwareLayerLog, NULL);
                    std::cout << "---------------------------------\n" << std::endl;
                }

                std::cout << "\nTotal Runtime: " << std::chrono::duration_cast<std::chrono::seconds>(totalRuntime).count() << " seconds\n" << std::endl;
                // Stop sim
                outputFile << hardwareLayerLog << "\n]";
                outputFile.close();
                // Stop matlab
                matlabPtr->feval(u"set_param", 0, { modelHandle, factory.createScalar("SimulationCommand"), factory.createScalar("stop") });
                matlab::engine::terminateEngineClient();
                std::raise(SIGTERM);
                return 0;
            }

            matlabPtr->feval(u"set_param", 0, { modelHandle, factory.createScalar("SimulationCommand"), factory.createScalar("continue") });
            auto currentTime = std::chrono::system_clock::now();
            auto totalTime = currentTime - simStartTime;
            hardwareLayerLog.insert(std::make_pair("totalTime", std::chrono::duration_cast<std::chrono::milliseconds>(totalTime).count()));
            if (config.consoleLogging) {
                prettyPrintJson(std::cout, hardwareLayerLog, NULL);
                std::cout << "---------------------------------\n" << std::endl;
            }

            outputFile << hardwareLayerLog << ",\n";
            hardwareLayerLog.clear();
            // Make sure HMI is done
            // Check if container is ready before waiting
            auto startWaitTime = std::chrono::system_clock::now();
            // -1: sem is locked, 0: we got the lock
            int semLocked = -1;
            if (config.maxSemWaitMS < 0) {
                semLocked = sem_wait(&hmiShmData->isContainerReady);
                if (semLocked == -1) throw std::runtime_error("Host HMI sem_wait failed");
            } else {
                while (semLocked == -1) {
                    semLocked = sem_trywait(&hmiShmData->isContainerReady);
                    auto currentTime = std::chrono::system_clock::now();
                    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startWaitTime);
                    if (timeDiff.count() > config.maxSemWaitMS)
                        break;
                }
            }
        }
    }
    return 0;
}