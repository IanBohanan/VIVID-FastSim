import json
import sys

if len(sys.argv) < 3: raise "Input Arguments required: Path to FastSim; Output dir name"
PROJECT_PATH = sys.argv[1]
OUTPUT_DIR_NAME = sys.argv[2]

with open(PROJECT_PATH + "/output/" + OUTPUT_DIR_NAME + "/output.json") as outputFile:
    outputJson = json.load(outputFile)

with open(PROJECT_PATH + "/models/watertank/config.json") as configFile:
    configJson = json.load(configFile)

with open(PROJECT_PATH + "/validation/baselineData/matlabOutput.json") as matlabFile:
    matlabLogJson = json.load(matlabFile)

# for each plc (in order from config): digitalInputs, analogInputs, digitalOutputs, analogOutputs
outputArray = []
for index, object in enumerate(outputJson):
    objectArray = []
    objectArray.append(object["simTime"])
    for plcConfig in configJson["plcs"]:
        plc = object[plcConfig["name"]]
        for digitalInput in plcConfig["digitalInputs"]:
            if (digitalInput["name"] in plc):
                objectArray.append(plc[digitalInput["name"]])
        for analogInput in plcConfig["analogInputs"]:
            if (analogInput["name"] in plc):
                objectArray.append(plc[analogInput["name"]])
        for digitalOutput in plcConfig["digitalOutputs"]:
            if (digitalOutput["name"] in plc):
                objectArray.append(plc[digitalOutput["name"]])
        for analogOutput in plcConfig["analogOutputs"]:
            if (analogOutput["name"] in plc):
                objectArray.append(plc[analogOutput["name"]])
    outputArray.append(objectArray)
timestepSize = int(configJson["timestepSizeSec"] * 1000)
matlabOutputArray = []
for data in matlabLogJson:
    simTime = int(data["simTime"] * 1000)
    if simTime % timestepSize == 0:
        matlabOutputArray.append([data["simTime"], data["percentFull"], data["pump"], data["valve"]])
diffGreater = False
for index, output in enumerate(outputArray):
    if index >= 1199: break
    print("PLC Output: ")
    print(output)
    print("Matlab Output: ")
    print(matlabOutputArray[index])
    print("Timestep: " + str(matlabOutputArray[index][0]) + ", Pumps Match: " + str(output[2] == matlabOutputArray[index][2]) + ", Valves Match: " + 
        str(output[3] == matlabOutputArray[index][3]) + 
        ", Percent Full Diff: " + str(output[1] - float(matlabOutputArray[index][1])))
    if abs(output[1] - float(matlabOutputArray[index][1])) > 50: diffGreater = True
    print("*-----------------------------------------------------------*")
print("***Some diffs > 0.5%***" if diffGreater else "***All diffs <= 0.5%***")