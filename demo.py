"""
Simulation: show impact of solution on data yield

"""
from random import randint
from time import sleep, time
import sys

DesiredQoS = .1 # 10% minimum required data yield
OneRoundOfOpsTime = 1 # assume 1 seconds needed to sense and relay data for demo app
OneRoundOfOpsEnergy = 1 # units of energy required per normal operation sequence
HarvestableEnergyDuringAttack = 2 # units of energy per second harvestable during the attack
mode = "mitigation"
demoDuration = 60 # seconds
delta = 2.0 # rate of fixed sleep time decrease
attackDuration = 20
debug = False
fname = "results-baseline.csv"
fh = 1
starttime = time()
attackFrequency = 10
normalEnergyCollectedPerRound = 5

def dprint(msg):
    if debug == True:
        print(msg)

""" Demo Appliction: makes API calls to collect and send data """
class Application:
    def __init__(self, mitigator):
        self.mitigator = mitigator 
        self.starttime = time()
        self.numAttacks = 0

    def normalOps(self):
        # run the simulated application
        dprint("energy is {}".format(self.mitigator.energy))
        data = self.mitigator.sense()
        sleep(.25) # process the data
        self.mitigator.send(data)
        sleep(.1) # reset stuff
        self.mitigator.updateEnergy(-1 * OneRoundOfOpsEnergy)

    def run(self):
        self.normalOps()

        # simulate periodic attack occuring
        if mode == "mitigation":
            if self.mitigator.power == "run" and int(time()-self.starttime) % attackFrequency == 0: # randint(1,5) == 1:
                print("Attack detected!")
                self.numAttacks += 1
                simulatedAttackEnd = time() + attackDuration # randint(1, 30)
                self.mitigator.sleeptime = self.mitigator.baselineSleepTime # reset at beginning of each attack
                while time() < simulatedAttackEnd:
                    # enter low power state
                    self.mitigator.doze()

                    dprint("waking for {} seconds".format(self.mitigator.waketime))
                    begin = time()
                    while time() < begin + self.mitigator.waketime:
                        self.normalOps()
                    
                print("Attack is over, resuming normal operations")

        # normal energy supply
        self.mitigator.updateEnergy(normalEnergyCollectedPerRound)
        print("number of attacks: {}".format(self.numAttacks))



""" Attack Mitigation system """
class Mitigator:
    def __init__(self, drivers):
        self.network = drivers[0]
        self.sensor = drivers[1]
        self.power = "run" # run / suspend
        self.buf = []  # buffer for unsent messages
        self.energy = 100.0 # start with full energy
        self.energyLastRound = 100.0 # placeholder value

        # calculate sleep and wake time. Assuming that waking ops in one chunk is same QoS as multiple smaller wakeups
        self.baselineSleepTime = ((1-DesiredQoS) * 10) * OneRoundOfOpsTime # pessimistic starting assumption
        self.sleeptime = self.baselineSleepTime
        self.waketime = DesiredQoS * 10 * OneRoundOfOpsTime

    # wrapper for network driver
    def send(self, data):

        if self.power == "run":
            if len(self.buf):
                for entry in self.buf:
                    self.network.send(buf)
            self.network.send(data)
        else:
            self.buf.append(data)

        return "send ok"
        

    # wrapper for sensor driver
    def sense(self):
        if self.power == "run":
            data = self.sensor.sense()
            return data
        else:
            return None

    def updateEnergy(self, amt):
        tmp = self.energy + amt
        if tmp > 100:
            self.energy = 100
        elif tmp < 0:
            self.energy = 0
        else:
            self.energy = tmp
        if self.energy == 0:
            print("ran out of energy! shutting down...")
            exit()

    def doze(self):
        self.power = "suspend"
        dprint("energy is {}".format(self.energy))
        dprint("sleeptime is {}".format(self.sleeptime))

        # determine sleeptime based on power
        # 1. determine if new power has arrived
        #    -> if so, be more optimistic about estimate
        # if we have 10 units of energy remaining, of which two arrived during sleep, and 1 second of ops on average takes 1 unit of energy
        #  -> if rate of new energy is less than use per second, then we still ration energy usage, depends only on amt stored, since attack could theoretically continue forever
        # aim for new energy being equal to amount used: try to keep stored energy at starting level or higher
        # if energy is dropping, provide minimal required QoS
        # otherwise, provide slightly higher QoS each round until starts dropping
        if self.energy < self.energyLastRound and self.sleeptime < self.baselineSleepTime:
            print("increasing sleep time to conserve energy")
            self.sleeptime += ((self.sleeptime - self.baselineSleepTime) / 2) # almost exponential back-off
        elif self.energy > self.energyLastRound: # energy spent was less than energy harvested
            print("decreasing sleep time to improve QoS")
            self.sleeptime -= delta # fixed decrease
       
        print("dozing for {} seconds".format(self.sleeptime))
        sleep(self.sleeptime)
        self.energyLastRound = self.energy
        # simulated harvesting of energy during sleep
        self.updateEnergy(self.sleeptime * HarvestableEnergyDuringAttack)
        self.power = "run"



""" Peripheral drivers called via API """ 
class Network:
    def __init__(self):
        self.msgCount = 0 # amount of data sent
        self.beginTime = time()

    def send(self, data):
        dprint("Network is sending...")
        sleep(.5) # assume peripherals block
        self.msgCount += 1
        
        rate = 45 / 60.7
        duration = time() - self.beginTime
        dataYield = self.msgCount / (duration * rate) * 100

        #print("Message count is {} messages after {} seconds ({}%)".format(self.msgCount, round(duration,1), round(dataYield,1)))
        fh.write("{}\n".format(time()-starttime))
        return 0

class Sensor:
    def sense(self):
        dprint("Sensor is sensing...")
        sleep(.5)
        return "sensor data"

""" Run with attack mitigation system and without """

# Assumption: the application doesn't make more API requests while waiting for others to complete


if len(sys.argv) != 7:
    print("usage: python {} <mode> <harvestableEnergyDuringAttack> <attackFrequency> <attackDuration> <demoDuration> <outputFileName>".format(sys.argv[0]))
    fh = open(fname, 'w')
else:
    mode = sys.argv[1]
    HarvestableEnergyDuringAttack = float(sys.argv[2])
    attackFrequency = float(sys.argv[3])
    attackDuration = float(sys.argv[4])
    demoDuration = float(sys.argv[5])
    fh = open(sys.argv[6], 'w')

Network = Network()
Sensor = Sensor()
Mitigator = Mitigator([Network,Sensor])
App = Application(Mitigator)


startTime = time()
while time() < startTime + demoDuration:
    App.run()


