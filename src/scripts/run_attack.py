import json
import subprocess
import sys
import yaml

if len(sys.argv) < 4: raise "Input Arguments required: Path to FastSim; Attack Scenario Path; Attack Dir Path; Output Dir"
PROJECT_PATH = sys.argv[1]
ATTACK_SCENARIO_PATH = sys.argv[2]
OUTPUT_DIR = sys.argv[3]
MODEL_PATH = sys.argv[4]


modelConfig = MODEL_PATH + "config.json"
with open(modelConfig) as configfile:
        configJson = json.load(configfile)
attackEnabled = configJson["attackScenario"]        
if attackEnabled["enableScenario"] == True:
    attackList = []
    attackScenarioConfig = yaml.load(open(ATTACK_SCENARIO_PATH, "r").read(), Loader=yaml.CLoader)
    scenarioActions = attackScenarioConfig["scenario_actions"]
    log = []

    scenarioAsynchronous = attackScenarioConfig["run_asynchronous"]
    done = False


    if scenarioAsynchronous == 'false': 
        for attackIndex in range(1, len(scenarioActions) + 1):
            actionLog = { "scenario_action": attackIndex, "commands": [] }
            attack = scenarioActions[attackIndex]
                
            attackConfig = yaml.load(open(PROJECT_PATH + "/" + attack["path"], 'r').read(), Loader=yaml.CLoader)
            attackActions = attackConfig["actions"]
            for index in range(1, len(attackActions) + 1):
                command = attackActions[index]
                output = subprocess.run("docker exec -w /FastSim " + attack["container_name"] + " " + command, shell=True, check=True, text=True, capture_output=True)
                actionLog["commands"].append({ "command": command, "output": output, "done": False })
                
            log.append(actionLog)
        with open(OUTPUT_DIR + "/attackLog.json", "w") as outfile:
            outputLog = str(log)
            outfile.write(json.dumps(outputLog))



    if scenarioAsynchronous == 'true': 
        for attackIndex in range(1, len(scenarioActions) + 1):
            actionLog = { "scenario_action": attackIndex, "commands": [] }
            attack = scenarioActions[attackIndex]
        
            attackConfig = yaml.load(open(PROJECT_PATH + "/" + attack["path"], 'r').read(), Loader=yaml.CLoader)
            attackActions = attackConfig["actions"]
            for index in range(1, len(attackActions) + 1):
                command = attackActions[index]
                output = subprocess.Popen("docker exec -w /FastSim " + attack["container_name"] + " " + command, shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                actionLog["commands"].append({ "command": command, "output": output, "done": False })
        
            log.append(actionLog)

        while not done:
            doneList = []
            for actionLog in log:
                for command in actionLog["commands"]:
                    poll = command["output"].poll()
                    if poll is not None and not command["done"]:
                        output = command["output"].communicate()
                        command["stdout"] = output[0]
                        command["stderror"] = output[1]
                        command["done"] = True
                        command.pop("output", None)
                    doneList.append(command["done"])
            done = all(doneList)

        with open(OUTPUT_DIR + "/attackLog.json", "w") as outfile:
            outfile.write(json.dumps(log))
