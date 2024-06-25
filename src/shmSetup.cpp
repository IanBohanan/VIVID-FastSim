#include "shmSetup.hpp"

ShmData* createShm(std::string shmPath) {
    std::cout << "Setting up shared memory file: " << shmPath << std::endl;
    int fd = -1;
    do {
        fd = shm_open(shmPath.c_str(), O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
        // If the file still exists, unlink it
        if (fd == -1) {
            std::cout << strerror(errno) << std::endl;
            shm_unlink(shmPath.c_str());
        }
    } while (fd == -1);

    // ftruncate is a misnomer, it changes the size of the specified file to whatever value we pass in.  We are using it here to 
    // expand the file to fit our struct (in bytes).  Technically, the file size will be equal to max(size of struct, page size)
    if (ftruncate(fd, sizeof(ShmData)) == -1) throw std::runtime_error("Failed to extend shared memory file to fit our struct");

    // Perform the mmap so that we get a pointer to memory that represents the contents of the file.  Our file will contain
    // one instance of our struct so we can treat the pointer as if it were a pointer to an instance of the struct.
    ShmData* shmData = (ShmData*)mmap(NULL, sizeof(*shmData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmData == MAP_FAILED) throw std::runtime_error("Failed to map shared memory");

    // Add permissions since docker runs as root
    system(("chmod a=rw /dev/shm" + shmPath).c_str());

    // Initialize our semaphore, pshared == 1 => it isn't shared across threads in this process (i.e.: every thread will need to do this)
    // initial value = how many resources are available (i.e.: will an immediate call to sem_wait work or sleep)
    if (sem_init(&shmData->isHostReady, /* pshared */ 1, /* initial value */ 0) == -1) throw std::runtime_error("sem_init isHostReady failed");
    if (sem_init(&shmData->isContainerReady, 1, 0) == -1) throw std::runtime_error("sem_init isClientReady failed");
    return shmData;
}

ShmData* connectToShm(std::string shmPath) {
    int fd = shm_open(shmPath.c_str(), O_RDWR, 0);
    if (fd == -1) {
        std::cout << strerror(errno) << std::endl;
        throw std::runtime_error("Could not open host (" + shmPath + ") shm file");
    }

    ShmData* shmData = (ShmData*)mmap(NULL, sizeof(*shmData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmData == MAP_FAILED) throw std::runtime_error("Failed to map shared memory");

    std::cout << "Connected to (" << shmPath << ") successfully" << std::endl;
    return shmData;
}