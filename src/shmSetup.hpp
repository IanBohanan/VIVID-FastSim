#include <semaphore.h>
#include <string>
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h> 
#include <cstdlib>
#include <stdlib.h>
#include <fcntl.h> 
#include <unistd.h>
#include <string.h>

struct ShmData {
    sem_t isHostReady; // Define a semaphore to act as a mutex
    sem_t isContainerReady; // Define a semaphore to act as a mutex
    bool lastRun = false;
    double simTime = 0.0;
    bool digitalInputs[128];
    int totalDigitalInputs = 0;
    bool digitalOutputs[128];
    int totalDigitalOutputs = 0;
    int analogInputs[128];
    int totalAnalogInputs = 0;
    int analogOutputs[128];
    int totalAnalogOutputs = 0;
    bool reconnectHmi = false;
};

ShmData* createShm(std::string shmPath);
ShmData* connectToShm(std::string shmPath);