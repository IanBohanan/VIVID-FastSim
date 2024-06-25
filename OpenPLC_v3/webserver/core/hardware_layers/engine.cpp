#include <iostream>
#include <iterator>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sys/mman.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <utility>
#include <boost/json/src.hpp>
#include "ladder.h"
#include "custom_layer.h"
#include "../../../src/parseConfig.hpp"
#include "../../../src/shmSetup.hpp"

PlcConfig plcConfig;
std::string shmPath;
ShmData* shmData;

void startModbus(std::string ipAddress) {
    std::cout << "Starting Modbus..." << std::endl;

    struct addrinfo hints;
    struct addrinfo *servinfo;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int status = getaddrinfo(ipAddress.c_str(), "43628", &hints, &servinfo);
    if (status == -1) {
        std::cout << "***Error getting address info***" << std::endl;
        return;
    }
    int connectSocket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (connectSocket == -1) {
        std::cout << "***Error creating socket***" << std::endl;
        return;
    }
    int connectOutput = connect(connectSocket, servinfo->ai_addr, servinfo->ai_addrlen);
    if (connectOutput == -1) perror("socket connection error");
    while (connectOutput == -1) {
        connectOutput = connect(connectSocket, servinfo->ai_addr, servinfo->ai_addrlen);
        if (connectOutput == -1) perror("socket connection error");
    }
    std::cout << "Connected to PLC server" << std::endl;
    const char *connectMessage = "start_modbus(502)\n";
    int messageLength = strlen(connectMessage) + 1;
    int bytesSent = send(connectSocket, connectMessage, messageLength, 0);
    if (bytesSent == -1) {
        perror("error sending message");
        return;
    }
    std::cout << "Message sent successfully: " << bytesSent << std::endl;
}


//-----------------------------------------------------------------------------
// This function is called by the main OpenPLC routine when it is initializing.
// Hardware initialization procedures should be here.
//-----------------------------------------------------------------------------
void initializeHardware() {
    std::string plcName = std::getenv("PLC_NAME");
    std::string configPath = std::getenv("CONFIG_PATH");

    // Parse JSON config file
    Config config = parseConfig(configPath);
    for (PlcConfig plc : config.plcs) {
        if (plc.name == plcName) {
            plcConfig = plc;
            break;
        }
    }

    startModbus(plcConfig.ipAddress);

    std::cout << "Connecting to shared memory" << std::endl;
    // Setup shm
    shmPath = "/" + plcName; // actually maps to /dev/shm/<plcName>
    shmData = connectToShm(shmPath);

    // Set any initial plc values
    pthread_mutex_lock(&bufferLock);
    for (auto digitalOutput : plcConfig.digitalOutputs) {
        if (digitalOutput.value.has_value()) 
            *bool_output[digitalOutput.address/8][digitalOutput.address%8] = (uint8_t)digitalOutput.value.value();
    }
    for (auto analogOutput : plcConfig.analogOutputs) {
        if (analogOutput.value.has_value()) *int_output[analogOutput.address] = (uint16_t)analogOutput.value.value();
    }
    pthread_mutex_unlock(&bufferLock);

    std::cout << "Hardware layer initialized successfully" << std::endl;
}

//-----------------------------------------------------------------------------
// This function is called by the main OpenPLC routine when it is finalizing.
// Resource clearing procedures should be here.
//-----------------------------------------------------------------------------
void finalizeHardware() {
    
}

//-----------------------------------------------------------------------------
// This function is called by the OpenPLC in a loop. Here the internal buffers
// must be updated to reflect the actual Input state. The mutex bufferLock
// must be used to protect access to the buffers on a threaded environment.
//-----------------------------------------------------------------------------
void updateBuffersIn() {
    // Wait until the host calls sem_post to indicate there's new data
    if (sem_wait(&shmData->isHostReady) == -1) throw std::runtime_error("sem_wait failed");

    // Get data from simulink
    auto digitalInputs = plcConfig.digitalInputs;
    for (int i = 0; i < digitalInputs.size(); i++) {
        int address = digitalInputs[i].address;
        *bool_input[address/8][address%8] = *(&shmData->digitalInputs[i]);
    }
    auto analogInputs = plcConfig.analogInputs;
    for (int i = 0; i < analogInputs.size(); i++) {
        int address = analogInputs[i].address;
        *int_input[address] = *(&shmData->analogInputs[i]);
    }
}

//-----------------------------------------------------------------------------
// This function is called by the OpenPLC in a loop. Here the internal buffers
// must be updated to reflect the actual Output state. The mutex bufferLock
// must be used to protect access to the buffers on a threaded environment.
//-----------------------------------------------------------------------------
void updateBuffersOut() {
    // Get data from PLC
    auto digitalOutputs = plcConfig.digitalOutputs;
    for (int i = 0; i < digitalOutputs.size(); i++) {
        int address = digitalOutputs[i].address;
        shmData->digitalOutputs[i] = *bool_output[address/8][address%8];
    }
    auto analogOutputs = plcConfig.analogOutputs;
    for (int i = 0; i < analogOutputs.size(); i++) {
        int address = analogOutputs[i].address;
        shmData->analogOutputs[i] = *int_output[address];
    }

    // Signal to host that data is ready
    if (sem_post(&shmData->isContainerReady) == -1) throw std::runtime_error("sem_post failed");

    // if (shmData->lastRun) run_openplc = 0;
}
