from scapy.all import *
from scapy.contrib.modbus import *
import json

def rewriter(outputPath, configJson):
    hmiFilePath = outputPath + "/hmiLog.json"
    f = open(hmiFilePath, 'r+')
    allfile = f.read()
    test = "]" in allfile
    f.close()
    if test != True:
        with open(hmiFilePath, 'r') as f:
            test = f.readlines()
            comma = test[-1:]
            replace = comma[0].replace("},", "}")
        with open(hmiFilePath, "w") as f:
            for thing in test[:-1]:
                f.write(thing)
            f.write(replace)
            f.write("]")
        
    packets = rdpcap(outputPath + "/output.pcap")
    fileHmi = open(outputPath + "/hmiLog.json")
    hmilog = json.load(fileHmi)
    fileHmi.close()
    #also need to import hmi json
    TimeStepIndex=0
    HmiInstanceIndex = 0
    PermaIndex = 0
    beforeFlag = True
    TCPPackets = False
    for plc in configJson["plcs"]:
        if len(plc["digitalInputs"]) != 0:
            PermaIndex = PermaIndex + 1
        if len(plc["digitalOutputs"]) != 0:
            PermaIndex = PermaIndex + 1
        if len(plc["analogInputs"]) != 0:
            PermaIndex = PermaIndex + 1
        if len(plc["analogOutputs"]) != 0:
            PermaIndex = PermaIndex + 1

    timeHolder = 0

    for packetIndex, p in enumerate(packets):
        #if flag is false and packet is not first modbus
        #set time to zero for now, then continue
        if beforeFlag and (p.haslayer('TCP') == False): #deal with nonTCP packets
            p.time = 0
            wrpcap(outputPath + '/filtered.pcap', p, append=True)  
            continue
        elif beforeFlag and ((p['TCP'].flags & 0x16) != 0x10): #skip all packets before connection
            p.time = 0
            wrpcap(outputPath + '/filtered.pcap', p, append=True) 
            continue
        elif beforeFlag and ((p['TCP'].flags & 0x16) == 0x10): #trigger connection
            p.time = 0
            beforeFlag = False
            wrpcap(outputPath + '/filtered.pcap', p, append=True) 
            continue

        #should zero time any packets of any protocol between connection and first modbus req
        if p.haslayer('TCP') == False and TCPPackets == False:
            p.time = 0
            wrpcap(outputPath + '/filtered.pcap', p, append=True) 
            continue
        else:
            TCPPackets = True

        if p.haslayer('TCP'): #should filter for TCP/Modbus packets, otherwise not a request
            tcp_payload_len = len(p[TCP].payload)
            if p.haslayer(Padding):
                tcp_payload_len -= len(p[Padding])
        else:
            tcp_payload_len = 0

        

        if (tcp_payload_len == 12): #if tcp and length ==12 then request packet
            if HmiInstanceIndex == PermaIndex: #reset Hmi request count tracker (num of req * 4)
                HmiInstanceIndex = 0 #index reset point needs to be 3 * # of PLCs
                TimeStepIndex = TimeStepIndex + 1 #go to next polling timestep

        if HmiInstanceIndex == 0: # original request per poll trigger
            timeHolder = p.time
            p.time = float(hmilog[TimeStepIndex]['simTime'])
        else:
            p.time = (p.time - timeHolder) + float(hmilog[TimeStepIndex]['simTime'])

        if (tcp_payload_len == 12): 
            HmiInstanceIndex = HmiInstanceIndex + 1
        
        wrpcap(outputPath + '/filtered.pcap', p, append=True) 