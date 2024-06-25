import os
import subprocess
import json
import sys
import time
import yaml
from yaml.loader import SafeLoader
sys.path.append("./src/scripts")
from PcapRewriteModule import rewriter
#from run_attack import attack
if len(sys.argv) < 4: raise "Input Arguments required: Path to FastSim; Model Name; Git Branch Name"
PROJECT_PATH = sys.argv[1]
MODEL_NAME = sys.argv[2]
BRANCH_NAME = sys.argv[3]

# Some commands require sudo
subprocess.run(["sudo", "-v"], check=True)

# Check if matlab.conf exists and create if it doesn't 
matlabConfFilePath = "/etc/ld.so.conf.d/matlab.conf"
if not os.path.isfile(matlabConfFilePath):
    print("Creating matlab.conf")
    with open(matlabConfFilePath, "w") as matlabConfFile:
        matlabConfFile.write("/usr/local/MATLAB/R2021b/extern/bin/glnxa64")
        subprocess.run("ldconfig", capture_output=True, check=True)

# Build coordinator.cpp first in case there's errors
subprocess.run("export LD_LIBRARY_PATH=/usr/local/MATLAB/R2021b/extern/bin/glnxa64:/usr/local/MATLAB/R2021b/sys/os/glnxa64 && g++ -std=gnu++17 -I /usr/local/boost_1_79_0 -I /usr/local/MATLAB/R2021b/extern/include/ -L /usr/local/MATLAB/R2021b/extern/bin/glnxa64/ -pthread ./src/coordinator.cpp ./src/parseConfig.cpp ./src/prettyPrintJson.cpp ./src/shmSetup.cpp -o coordinator -lMatlabDataArray -lMatlabEngine -lrt", shell=True, check=True)
print("Coordinator compiled successfully!")

# Read config file and create docker container for each PLC
configFilePath = PROJECT_PATH + "/models/" + MODEL_NAME + "/config.json"
with open(configFilePath) as configFile:
    configJson = json.load(configFile)

# Check if docker image exists
dockerImageExists = subprocess.run([ "docker", "image", "inspect", "fastsim" ], text=True, capture_output=True)
if dockerImageExists.returncode != 0:
    # Image doesn't exist so create from Dockerfile
    subprocess.run([ "docker", "build", "--tag", "fastsim", "." ], check=True)

for plc in configJson["plcs"]:
    # Check if plc container exists
    dockerContainerExists = subprocess.run([ "docker", "container", "inspect", plc["name"] ], text=True, capture_output=True)
    if dockerContainerExists.returncode != 0:
        print("Creating docker container for " + plc["name"])
        subprocess.run([ "docker", "run", "--env", "PLC_NAME=" + plc["name"], "--env", "CONFIG_PATH=/FastSim/models/" + MODEL_NAME + "/config.json", "-dit", "--privileged","--name", plc["name"], "-v", "/dev/shm:/dev/shm", "fastsim" ], check=True)
    else:
        # Make sure container is running fresh
        subprocess.run([ "docker", "container", "stop", plc["name"] ], check=True)
        subprocess.run([ "docker", "network", "disconnect", "macvlan", plc["name"] ])
        subprocess.run([ "docker", "network", "connect", "bridge", plc["name"] ])
        subprocess.run([ "docker", "container", "start", plc["name"] ], check=True)
    # Now do a git pull and switch to BRANCH_NAME
    subprocess.run([ "docker", "exec", "-w", "/FastSim", plc["name"], "git", "reset", "--hard" ], check=True)
    subprocess.run([ "docker", "exec", "-w", "/FastSim", plc["name"], "git", "pull" ], check=True)
    subprocess.run([ "docker", "exec", "-w", "/FastSim", plc["name"], "git", "checkout", BRANCH_NAME ], check=True)
    subprocess.run([ "docker", "cp", configFilePath, plc["name"] + ":/FastSim/models/" + MODEL_NAME + "/config.json" ])
    
    # Switch to macvlan docker network
    macvlanConfig = configJson["macvlan"]
    # Check for interface
    interfaceExists = subprocess.run([ "ls", "/sys/class/net/" + macvlanConfig["parent"] + "/" ], text=True, capture_output=True)
    if interfaceExists.returncode != 0:
        subprocess.run([ "sudo", "modprobe", "dummy" ], check=True)
        subprocess.run([ "sudo", "ip", "link", "add", macvlanConfig["parent"], "type", "dummy"], check=True)
        subprocess.run([ "sudo", "ifconfig", macvlanConfig["parent"], macvlanConfig["subnet"] ], check=True)
    # Check for macvlan network
    dockerMacvlanExists = subprocess.run([ "docker", "network", "inspect", "macvlan" ], text=True, capture_output=True )
    if dockerMacvlanExists.returncode != 0:
        # Create macvlan docker network
        subprocess.run([ "docker", "network", "create", "-d", "macvlan", "--subnet", macvlanConfig["subnet"], "--gateway", macvlanConfig["gateway"], "-o parent=" + macvlanConfig["parent"], "macvlan" ], check=True)
    # Switch container to macvlan network
    subprocess.run([ "docker", "network", "disconnect", "bridge", plc["name"] ])
    subprocess.run([ "docker", "network", "connect", "--ip", plc["ipAddress"], "macvlan", plc["name"] ])
    # Start openPLC
    # subprocess.Popen([ "docker", "exec", "-w", "/FastSim", plc["name"], "./start_openplc.sh", "/FastSim/models/" + MODEL_NAME + "/" + configJson["stFile"] ])

# Start HMI
hmi = configJson["hmi"]
# Check if hmi container exists
dockerContainerExists = subprocess.run([ "docker", "container", "inspect", hmi["name"] ], text=True, capture_output=True)
if dockerContainerExists.returncode != 0:
    print("Creating docker container for " + hmi["name"])
    subprocess.run([ "docker", "run", "--env", "CONFIG_PATH=/FastSim/models/" + MODEL_NAME + "/config.json", "-dit", "--privileged","--name", hmi["name"], "-v", "/dev/shm:/dev/shm", "fastsim" ], check=True)
else:
    # Make sure container is running fresh
    subprocess.run([ "docker", "container", "stop", hmi["name"] ], check=True)
    subprocess.run([ "docker", "network", "disconnect", "macvlan", hmi["name"] ])
    subprocess.run([ "docker", "network", "connect", "bridge", hmi["name"] ])
    subprocess.run([ "docker", "container", "start", hmi["name"] ], check=True)
# Now do a git pull and switch to BRANCH_NAME
subprocess.run([ "docker", "exec", "-w", "/FastSim", hmi["name"], "git", "reset", "--hard" ], check=True)
subprocess.run([ "docker", "exec", "-w", "/FastSim", hmi["name"], "git", "pull" ], check=True)
subprocess.run([ "docker", "exec", "-w", "/FastSim", hmi["name"], "git", "checkout", BRANCH_NAME ], check=True)
subprocess.run([ "docker", "cp", configFilePath, hmi["name"] + ":/FastSim/models/" + MODEL_NAME + "/config.json" ])
subprocess.run([ "docker", "exec", "-w", "/FastSim", hmi["name"], "service", "ssh", "start" ], check=True)
# Switch to macvlan docker network
macvlanConfig = configJson["macvlan"]
# Check for interface
interfaceExists = subprocess.run([ "ls", "/sys/class/net/" + macvlanConfig["parent"] + "/" ], text=True, capture_output=True)
if interfaceExists.returncode != 0:
    subprocess.run([ "sudo", "modprobe", "dummy" ], check=True)
    subprocess.run([ "sudo", "ip", "link", "add", macvlanConfig["parent"], "type", "dummy"], check=True)
    subprocess.run([ "sudo", "ifconfig", macvlanConfig["parent"], macvlanConfig["subnet"] ], check=True)
# Check for macvlan network
dockerMacvlanExists = subprocess.run([ "docker", "network", "inspect", "macvlan" ], text=True, capture_output=True )
if dockerMacvlanExists.returncode != 0:
    # Create macvlan docker network
    subprocess.run([ "docker", "network", "create", "-d", "macvlan", "--subnet", macvlanConfig["subnet"], "--gateway", macvlanConfig["gateway"], "-o parent=" + macvlanConfig["parent"], "macvlan" ], check=True)
# Switch container to macvlan network
subprocess.run([ "docker", "network", "disconnect", "bridge", hmi["name"] ])
subprocess.run([ "docker", "network", "connect", "--ip", hmi["ipAddress"], "macvlan", hmi["name"] ])
# Compile and start HMI
# hmiProcess = subprocess.Popen([ "docker", "exec", "-w", "/FastSim/src/hmi", hmi["name"], "./start_hmi.sh" ])

# Start Attacker
attacker = configJson["attacker"]
# Checks the config file to see if there is an attack
if attacker != None:
    attackerContainerExists = subprocess.run(["docker", "container", "inspect", attacker["name"]], text=True, capture_output=True)
    if attackerContainerExists.returncode != 0:
        print("Creating docker container for " + attacker["name"])
        subprocess.run([ "docker", "run", "--env", "CONFIG_PATH=/FastSim/models/" + MODEL_NAME + "/config.json", "-dit", "--privileged", "--name", attacker["name"], "-v", "/dev/shm:/dev/shm", "fastsim" ], check=True)
    else:
        # Make sure container is running fresh
        subprocess.run([ "docker", "container", "stop", attacker["name"] ], check=True)
        subprocess.run([ "docker", "network", "disconnect", "macvlan", attacker["name"] ])
        subprocess.run([ "docker", "network", "connect", "bridge", attacker["name"] ])
        subprocess.run([ "docker", "container", "start", attacker["name"] ], check=True)
        # Do git pull
    subprocess.run([ "docker", "exec", "-w", "/FastSim", attacker["name"], "git", "reset", "--hard" ], check=True)
    subprocess.run([ "docker", "exec", "-w", "/FastSim", attacker["name"], "git", "pull" ], check=True)
    subprocess.run([ "docker", "exec", "-w", "/FastSim", attacker["name"], "git", "checkout", BRANCH_NAME ], check=True)
    subprocess.run([ "docker", "cp", configFilePath, attacker["name"] + ":/FastSim/models/" + MODEL_NAME + "/config.json" ])
    subprocess.run([ "docker", "network", "disconnect", "bridge", attacker["name"] ])
    subprocess.run([ "docker", "network", "connect", "--ip", attacker["ipAddress"], "macvlan", attacker["name"] ])

# Create output directory
outputPath = PROJECT_PATH + "output/" + str(time.time_ns())
os.makedirs(outputPath)
# Start coordinator 
coordinatorProcess = subprocess.Popen([ "./coordinator", PROJECT_PATH, PROJECT_PATH + "/models/" + MODEL_NAME + "/", outputPath ])
# Compile and start Sniffer
# Find interface for sniffer
SnifferSearch = subprocess.Popen(("ls", "/sys/class/net/"), stdout=subprocess.PIPE)
SnifferInterfaceExists = subprocess.check_output(("grep", "dm-"), stdin=SnifferSearch.stdout)
# Gets Rid of newline character in the inteface name
SnifferInterfaceExists = SnifferInterfaceExists[:-1]
#Turns the interface into a string
SnifferInterfaceExists=str(SnifferInterfaceExists)
#Replaces b' with nothing
SnifferInterface = SnifferInterfaceExists.replace("b'", "")
#strips the last ' from the string
SnifferInterface = SnifferInterface.rstrip("'")
#subprocess.run(["g++", "--std=c++17", "-osniffer", "/home/ccre/Attack-Files/src/sniffer/sniffer.cpp", "-lpcap"])
#Starts the sniffing process
snifferProcess=subprocess.Popen(["sudo", "tcpdump", "-B", "1048576", "-i", SnifferInterface, "-w", outputPath + "/output.pcap"])
#snifferProcess = subprocess.Popen(["docker", "exec", "-w", "/FastSim/src/sniffer", hmi["name"], "./start_sniffer.sh"])
# This is a busy wait until coordinator sends the SIGTERM signal, anything that comes after wait() will not get executed until coordinator completes 
coordinatorProcess.wait()
coordinatorProcess.kill()
#subprocess.run(["docker", "cp", hmi["name"]+":/FastSim/src/sniffer/output.pcap", outputPath + "/output.pcap"])
subprocess.run(["docker", "cp", hmi["name"]+":/FastSim/src/hmi/hmiLog.json", outputPath + "/hmiLog.json"])
retrieveLogPath = configJson["attackScenario"]
if retrieveLogPath["enableRetrieveLog"] == True:
    for plcs in configJson["plcs"]:
        subprocess.run(["docker", "cp", plcs["name"]+":/./FastSim/retrieveLog/", outputPath,], text=True, capture_output=True)
    subprocess.run(["docker", "cp", hmi["name"]+":/./FastSim/retrieveLog/", outputPath], text=True, capture_output=True)
    subprocess.run(["docker", "cp", attacker["name"]+":/./FastSim/retrieveLog/", outputPath], text=True, capture_output=True)
subprocess.run([ "docker", "container", "stop", hmi["name"] ], check=True)
subprocess.run([ "docker", "container", "rm", hmi["name"] ], check=True)
for plc in configJson["plcs"]:
    subprocess.run([ "docker", "container", "stop", plc["name"] ], check=True)
    subprocess.run([ "docker", "container", "rm", plc["name"] ], check=True)
subprocess.run(["docker", "container", "stop", attacker["name"]],check=True)
#Kills the sniffer
snifferkill = subprocess.run(("sudo", "pkill", "tcpdump", "-15"))
rewriter(outputPath, configJson)