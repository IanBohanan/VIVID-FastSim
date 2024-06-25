#include <modbus.h>
#include <iostream>
#include <signal.h>
#include <chrono>
#include <thread>
#include <cmath>
#include <boost/json/src.hpp>
#include "../parseConfig.hpp"
#include "../shmSetup.hpp"

std::string shmPath;
ShmData* shmData;
std::ofstream outputFile;
boost::json::object hmiLog;

void ReadData(std::vector<modbus_t*>& list, Config& config);

std::vector<modbus_t*> initializePlcConnections(Config config) {
    std::cout << "Setting up libmodbus..." << std::endl;
    
    //vector of modbus connection pointers for each detected plc
    std::vector<modbus_t*> PlcList;

    //for each loop using c++ syntax
    //designed to create a tcp connection for each PLC in the config file
    for (PlcConfig plc : config.plcs) {
        modbus_t *ctx;
        ctx = modbus_new_tcp(plc.ipAddress.c_str(), 502);
        if (ctx == NULL) {
            throw std::runtime_error("Unable to allocate libmodbus context for plc " + plc.name);
        }

        bool connected = false;
        while (!connected) {
            if (modbus_connect(ctx) == 0) {
                connected = true;
                std::cout << "Connected to modbus server" << std::endl;
            } else {
                using std::chrono::operator""ms;
                std::this_thread::sleep_until(std::chrono::steady_clock::now() + 1000ms);
            }
        }

        //connection viable and active
        PlcList.push_back(ctx);
    }
    return PlcList;
}

int main(int argc, char *argv[]) {
    // Parse config
    std::string configPath = std::getenv("CONFIG_PATH");
    Config config = parseConfig(configPath);
    std::cout << "Config parsed..." << std::endl;
    double pollingRate = config.hmiConfig.pollingRateSec * 1000.0;

    // Initialize log file
    bool fileCheck = std::filesystem::exists("./hmiLog.json");
    outputFile.open("./hmiLog.json", std::fstream::app);
    if (!fileCheck) {
        outputFile << "[\n";
    }
    int count=0;

    std::vector<modbus_t*> PlcList = initializePlcConnections(config);
    
    std::cout << "Setting up shared memory" << std::endl;
    // Setup shm
    shmPath = "/hmi"; // actually maps to /dev/shm/hmi
    shmData = connectToShm(shmPath);
    
	double lastPollTime = 0.0;
    // Start HMI sim loop
    bool stop = false;
    while (!stop) {
        // Wait until the host calls sem_post to indicate there's new data
        if (sem_wait(&shmData->isHostReady) == -1) throw std::runtime_error("sem_wait failed");
        if (shmData->reconnectHmi) {
            for (auto ctx : PlcList) {
                modbus_close(ctx);
                modbus_free(ctx);
            }
            shmData->reconnectHmi = false;
            PlcList = initializePlcConnections(config);
        }
        double simTime = shmData->simTime * 1000.0;
        // Use ms to avoid floating point issues
        double deltaTime = simTime - lastPollTime;
        if (deltaTime >= pollingRate || (fabs(deltaTime-pollingRate)<0.0001)) {
            hmiLog = {{ "simTime", std::to_string((double) simTime / 1000.0) }};
            // Poll plc for data
            ReadData(PlcList,config);
            lastPollTime=simTime;           
        }
        if (shmData->lastRun) {
            if (hmiLog.size() != 0) {
                outputFile << hmiLog;
            } 
            outputFile << "\n]";
            outputFile.close();
            shm_unlink(shmPath.c_str());
            stop = true;
        
        } else if (count==5) {
            if (hmiLog.size() != 0) {
                outputFile << hmiLog << ",\n";
                hmiLog.clear();
            } 
            outputFile.close();
            outputFile.open("./hmiLog.json", std::fstream::app);
            count=0;
            // Signal to host done reading
            if (sem_post(&shmData->isContainerReady) == -1) throw std::runtime_error("sem_post failed");
        }

         else {
            if (hmiLog.size() != 0) {
                outputFile << hmiLog << ",\n";
                hmiLog.clear();
            }
            count++; 
            // Signal to host done reading
            if (sem_post(&shmData->isContainerReady) == -1) throw std::runtime_error("sem_post failed");
        }
    }
    return 0;
}

void ReadData(std::vector<modbus_t*>& list, Config& config){
    int index = 0;
    std::cout << std::boolalpha;
    bool logging = config.hmiConfig.hmiConsoleLogging;

    //loop through each plc to get plc information and print
    for (PlcConfig plc : config.plcs) {
        hmiLog[plc.name] = {};

        //variable that determine nb for function calls
        int digSizeOUT, digSizeIN, analogSizeIN, analogSizeOUT;

        //get nb for each type in the plc
        digSizeIN = plc.digitalInputs.size();
        digSizeOUT = plc.digitalOutputs.size();
        analogSizeIN = plc.analogInputs.size();
        analogSizeOUT = plc.analogOutputs.size();

        //create return arrays the size of nb per libmodbus documentation
        uint16_t tab_regIN[analogSizeIN], tab_regOUT[analogSizeOUT];
        uint8_t tab_bitIN[digSizeIN] , tab_bitOUT[digSizeOUT];

        modbus_t* activeConnection = list[index];

        //read present values then go to print loops. WARNING: If force read non present value type: crash (seg fault)
        //using bool(non-zero) = true; prevents issues with bad modbus
        if(digSizeIN){
            modbus_read_input_bits(activeConnection,plc.digitalInputs[0].address,digSizeIN, tab_bitIN);
        }
        if(digSizeOUT){
            modbus_read_bits(activeConnection,plc.digitalOutputs[0].address,digSizeOUT, tab_bitOUT);
        }
        if(analogSizeIN){
            modbus_read_input_registers(activeConnection,plc.analogInputs[0].address,analogSizeIN, tab_regIN);
        }
        if(analogSizeOUT){
            modbus_read_registers(activeConnection,plc.analogOutputs[0].address,analogSizeOUT, tab_regOUT);
        }

        int nameIndex = 0;
        for (PlcData digitalInput: plc.digitalInputs) {
            hmiLog[plc.name].as_object().insert(std::make_pair(digitalInput.name, bool(tab_bitIN[nameIndex])));
            nameIndex++;
        }

        nameIndex = 0;
        for (PlcData digitalOutput: plc.digitalOutputs) {
            hmiLog[plc.name].as_object().insert(std::make_pair(digitalOutput.name, bool(tab_bitOUT[nameIndex])));
            nameIndex++;
        }

        nameIndex = 0;
        for (PlcData analogInput: plc.analogInputs) {
            hmiLog[plc.name].as_object().insert(std::make_pair(analogInput.name, tab_regIN[nameIndex]));
            nameIndex++;
        }

        nameIndex = 0;
        for (PlcData analogOutput: plc.analogOutputs) {
            hmiLog[plc.name].as_object().insert(std::make_pair(analogOutput.name, tab_regOUT[nameIndex]));
            nameIndex++;
        }
        
        if(logging){
            std::cout << "Reading PLC: " << plc.name << std::endl;
            int nameIndex = 0;
            for (PlcData digitalInput: plc.digitalInputs) {
                std::cout << digitalInput.name<< ": " << bool(tab_bitIN[nameIndex]) << std::endl;
                nameIndex++;
            }

            nameIndex = 0;
            for (PlcData digitalOutput: plc.digitalOutputs) {   
                std::cout << digitalOutput.name<< ": " << bool(tab_bitOUT[nameIndex]) << std::endl;
                nameIndex++;
            }

            nameIndex = 0;
            for (PlcData analogInput: plc.analogInputs) {
                std::cout << analogInput.name<< ": " << tab_regIN[nameIndex]<< std::endl;
                nameIndex++;
            }

            nameIndex = 0;
            for (PlcData analogOutput: plc.analogOutputs) {
                std::cout << analogOutput.name<< ": " << tab_regOUT[nameIndex] << std::endl;
                nameIndex++;
            }
            std::cout << std::endl;
        }
        index++; 
    }
}