***This is the main branch! Make sure to move to the dockerFix branch if you are using a VM!***


# FastSim
### Initial Run Setup
## VM
- Create Linux Oracle VM
  - Keep Original Machine folder
  - Type: Linux
  - Version: Ubuntu (64-bit)
- Allocate as much memory (RAM) as possible
  - used 21392 MB for current setup
- Select 'Create a virtual hard disk now with VirtualBox Disk Image'
  - Download **Ubuntu ISO 20.04** image for operating system
    - Note: Python default version 3.8.2
- Use Dynamically allocated storage
- Limit storage to at least 100GB
  - used 100GB for current setup
- In settings, after setup completion, go to Storage Controller: IDE -> set up optical drive with Ubuntu iso
- Start Machine
  - Download Ubuntu setup
  - Select Language and Keyboard (both English)
  - Normal Installation
  - Select 'Only Download updates while installing Ubuntu'
  - When prompted, Erase Disk and Install Ubuntu
  - 'Install Now'
  - If errors, restart Machine

## Computer: MAKE SURE TO GET MATLAB 2021b
- Install and Setup Matlab:
  - Login to your Mathworks account and download the 2021b Linux installer
  - Unzip in Downloads Folder from terminal using `cd Downloads` then `unzip [MATLAB]`
  - Run `xhost +SI:localuser:root` to gain root priveleges 
  - Start Installation with `sudo ./install`
    - if errors opening setup or adding root to access control list, try `xhost local:root`
  - Select your student license when propmted
  - Set the username to the username you use to login to your VM/computer
    - find in terminal using `whoami`
  - DO NOT CHANGE THE DEFAULT INSTALL PATH, if you do you'll have to update the path to matlab in the start.sh script
  - Install Matlab with toolboxes: Simulink, Simscape, Simscape Fluids, Simscape Electrical, DSP System, Signal Processing Toolbox
  - Select 'create simulink' in automatic bin location
  - Once installed, check usage by going to file home in terminal and using `matlab`

- If Matlab errors
  - Check license activation in usr/local/bin
  - If problems not resolved with license fix, remove and reinstall 
    - `sudo rm -rf /usr/local/MATLAB/R2021b`
    - `sudo rm /usr/local/bin/matlab /usr/local/bin/mcc /usr/local/bin/mex /usr/local/bin/mbuild` to check simulink removed

- If specific Matlab error of libMatlabDataArray.so missing so hmilog isn't created 
  - Currenlty attempts to fix include 
    - `sudo apt-get update`
    - `sudo apt-get upgrade`
    - restart VM
      
- Install OpenPLC_v3:
  - In terminal, from Downloads folder
    - `sudo apt install git`
    - `git clone https://github.com/thiagoralves/OpenPLC_v3.git`
    - `cd OpenPLC_v3`
    - `./install.sh linux`

- Install Docker:
  - Install using repository using following steps:
    - `sudo apt-get update`
    - `sudo apt-get install \
      ca-certificates \
      curl \
      gnupg \
      lsb-release`
    - #might need `sudo apt-get upgrade` but only if errors
    - `sudo mkdir -m 0755 -p /etc/apt/keyrings`
    - `curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg`
    - `echo \
 "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
$(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null`
    - make sure to include spaces properly as errors will occur from file not being read
    - if errors, double check file using `sudoedit /etc/apt/sources.list.d/docker.list`
    - After all above works, try `sudo apt-get update`. If error recieved, use `sudo chmod a+r /etc/apt/keyrings/docker.gpg` to change permissions for docker keyrings
    - Next, run `sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin`
    - `sudo docker run hello world` to test proper installation

  - If Errors, alternative download 
    - `sudo apt-get update`
    - `sudo apt install docker.io`
    - `sudo docker run hello world` to check installation

- Gain Docker Root Priveleges:
  - `sudo groupadd docker` but chances are group will have already been created, just ignore that output
  - `sudo usermod -aG docker [user]` modify user into group
  - `newgrp docker` activate group changes without restarting
  - `docker run hello-world` test without sudo

- Docker Installation RESOURCES 
  - Install using the repository: https://docs.docker.com/engine/install/ubuntu/#install-using-the-repository
  - Add yourself to the docker group so you don't have to run docker commands as root: https://docs.docker.com/engine/install/linux-postinstall/

- Install Boost:
  - Download https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.tar.bz2
  - Run `tar --bzip2 -xf boost_1_79_0.tar.bz2 -C /usr/local/` from downloads folder
  - Check if net-tools/ifconfig is installed:
    - `ifconfig -V`
    - If not installed, run `sudo apt install net-tools`

- Install Scapy 
  - `pip3 install scapy`
  - If errors running environment, remove and reinstall

- Clone GitLab Project to VM Desktop
  - `cd /Desktop`
  - `git clone https://github.com/IanBohanan/VIVID-FastSim.git -b [branch]`


- Download Wireshark to open pcap output file 
  - `sudo add-apt-repository ppa:wireshark-dev/stable`
  - `sudo apt update`
  - `sudo apt install wireshark`


### To Run Application
- Running Application within VM Terminal
  -  Navigate to cloned environment on Desktop
  - To run, use python3 start.py [absolute file path to environment folder] [model] [branch]
    - ex. python3 start.py /home/ccre/Desktop/FastSim/ watertank Attack-Files


- To Run Everything Locally:
  - Compile Coordinator: `export LD_LIBRARY_PATH=/usr/local/MATLAB/R2021b/extern/bin/glnxa64:/usr/local/MATLAB/R2021b/sys/os/glnxa64 && g++ -std=gnu++17 -I /usr/local/boost_1_79_0 -I /usr/local/MATLAB/R2021b/extern/include/ -L /usr/local/MATLAB/R2021b/extern/bin/glnxa64/ -pthread ./src/coordinator.cpp ./src/parseConfig.cpp ./src/prettyPrintJson.cpp -o coordinator -lMatlabDataArray -lMatlabEngine -lrt`
  - Set environment variables for OpenPLC: `export PLC_NAME=` and `export CONFIG_PATH=`
    - PLC_NAME should be the name of the plc from the model's config.json that this PLC represents e.g., for watertank it would just be "plc1"
    - CONFIG_PATH should be the path to the model's config file e.g., /home/whitney/Desktop/FastSim/models/watertank/config.json
  - Start OpenPLC first: `./start_openplc.sh [ST_FILE]`
    - ST_FILE is the path to the model ST file you want to run e.g., /home/whitney/Desktop/FastSim/models/watertank/watertank.st
  - Then start Coordinator: `./coordinator [PROJECT_PATH] [MODEL_PATH]`
    - PROJECT_PATH is the path to FastSim e.g., /home/whitney/Desktop/FastSim/
    - MODEL_PATH is the path to the model you're running e.g., /home/whitney/Desktop/FastSim/models/watertank/

- To Run with Docker:
  - Run: `/bin/python3 /path/to/start.py [PROJECT_PATH] [MODEL_NAME] [BRANCH_NAME]`
    - PROJECT_PATH is the path to FastSim e.g., /home/whitney/Desktop/FastSim/
    - MODEL_NAME is the name of the model folder e.g., watertank
    - BRANCH_NAME is the git branch you want to use e.g., main

## Noted Errors:
  - Error regarding unknown branch, double check execution command

  - If errors about missing files, remove and reclone.
  - matlab.conf permissions error 
    - `sudo chmod 757 /etc/ld.so.conf.d/`
    - followed by ldconfig error 
      - `xhost +SI:localuser:root` 
      - rerun python3 simulation command
  - Simulation Run Macvlan Connection Error
    - reset docker images and containers
     - `docker rm -f watertank_hmi watertank_attack plc1`
      - `docker image remove fastsim`
      - reconnect sudo privileges for docker container
      - restart computer/vm
      - `python3 start.py [fastsim directory] [model] [branch]`
        -Archive Ubunutu Error known fix is DNS related
          - `sudo apt-get update`
          - `sudo apt-get upgrade`
  - If fatal: unable to access gitlab: could not resolve host
    - Partnered with macvlan connection error, run above fix.
  -With First run, if error compiling c files, aka "Compilation finished with errors!"
    - Double check proper installation of all parts (specifically local OpenPLC_v3)

  -NEW UNSOLVED ERROR:
    - fatal: Remote branch Attack-Files not found in upstream origin

# NS3 Addition
## NS3 
  - Clone NS3withHardware branch with git
    - `git clone -b NS3withHardware https://gitlab.uah.edu/fastsim/FastSim.git NS3withHardware`
  - `cd NS3withHardware/NS3/`
  - Add bake to path 
    - `export BAKE_HOME=`pwd`/bake` as all one command
    - `export PATH=$PATH:$BAKE_HOME`
    - `export PYTHONPATH=$PYTHONPATH:$BAKE_HOME`
  - `bake.py check`
  - `bake.py configure -e ns-3.35`
  - `cd source/ns-3.35/`
  - `./waf configure --enable-examples --enable-tests --enable-sudo`
  - `./waf build`
    - if dependency errors, visit https://www.nsnam.org/wiki/Installation#Installation
  - testing with tutorial run
    - `./waf --run first`
      - Should output:
      - At time +2s client sent 1024 bytes to 10.1.1.2 port 9
      - At time +2.00369s server received 1024 bytes from 10.1.1.1 port 49153
      - At time +2.00369s server sent 1024 bytes to 10.1.1.1 port 49153
      - At time +2.00737s client received 1024 bytes from 10.1.1.2 port 9

## Tap and Docker
### NOTES
  - ONLY MOVE FORWARD IF PREVIOUS IS ERROR FREE
  - REQUIRED: FastSim properly installed and set up in second directory
  - Next step gathered from https://sites.google.com/thapar.edu/ramansinghtechpages/step-wise-establishing-connection
  - TAP DEVICES DO NOT STAY AFTER RESTART and WILL NEED TO BE REBUILT
_____________________________________________________________
  - `sudo apt install uml-utilities` 
  - Locate tap-setup-script.sh and ModifiedStart.py files in NS3withHardware directory and mv both into the updated, errorless FastSim directory
  - Create docker containers by running FastSim normally till simulation outputs OpenPLC compile
    - Do not let simulation complete
  - `docker container ps`
    - double checks all three containers still active
  - Stop all processes running on containers 
    - `docker attach watertank_hmi`
    - `killall5 15`
    - exit container with escape sequence
      - hold ctrl, press p q sequentially
    - `docker attach plc1`
    - `killall5 15`
    - exit container with escape sequence
      - hold ctrl, press p q sequentially
  -Disconnect containers from macvlan 
    - `docker network disconnect macvlan watertank_hmi`
    - `docker network disconnect macvlan plc1`
    - `docker network connect none watertank_hmi`
    - `docker network connect none plc1`
    - `docker network disconnect macvlan watertank_attacker`
    - `docker network connect none watertank_attacker`
  - Check installations
    - `sudo apt install net-tools`
    - `sudo apt-get upgrade`
    - `sudo apt-get update`
  - If any errors for this next bit, restart vm before reattempting
    - `sudo bash Tap-Setup-Script.sh` 
    - `docker attach watertank_hmi`
    - `ifconfig`
      - should see 2 devices, eth0 and lo
    - exit container

## Custom-tap-csma-VM run
  - Need two terminal windows open or download Terminator
    - `sudo apt install terminator` if you chose
  - Second terminal open to NS3withHardware/NS3/ns-3.35/
    - `sudo ./waf --run Goal2`
    - Let "the correct simulator imp is being called" and "null event" continue running through next bit
  - Current terminal open to FastSim
    - `python3 ModifiedStart.py [path to FastSim directory] watertank main`
      - Let run through simulation completion
  - Kill second terminal processes

## Successful Output
  - custom-tap-csma-0-0.pcap and custom-tap-csma-1.0.pcap holding packets with ~1sec interval
  - Inveral Adjustable in customTap-bridge.cc line 723, capable of 0.1<x< unknown
