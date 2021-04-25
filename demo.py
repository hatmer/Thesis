"""
Simulation: show impact of solution on data yield

"""
from random import randint
from time import sleep, time

DesiredQoS = .1 # 10% minimum required data yield
OneRoundOfOpsTime = 1 # assume 1 seconds needed to sense and relay data for demo app

""" Demo Appliction: makes API calls to collect and send data """
class Application:
    def __init__(self, mitigator):
        self.mitigator = mitigator 

    def normalOps(self):
        # run the simulated application
        data = self.mitigator.sense()
        sleep(.25) # process the data
        self.mitigator.send(data)
        sleep(.1) # reset stuff

    def run(self):
        self.normalOps()

        if self.mitigator.power == "run" and randint(1,5) == 1: # simulate random attack occuring
            print("Attack detected!")
            simulatedAttackEnd = time() + randint(1, 30)
            while time() < simulatedAttackEnd:
                self.mitigator.doze()
                print("waking for {} seconds".format(OneRoundOfOpsTime))
                begin = time()
                while time() < begin + OneRoundOfOpsTime:
                    self.normalOps()
                
            print("Attack is over, resuming normal operations")


""" Attack Mitigation system """
class Mitigator:
    def __init__(self, drivers):
        self.wakelocks = [0]*len(drivers) # one wakelock per corruptible driver
        self.network = drivers[0]
        self.sensor = drivers[1]
        self.power = "run" # run / suspend
        self.sleeptime = ((1-DesiredQoS) * 10) * OneRoundOfOpsTime  # how long to sleep before waking up
    
    def send(self, data):
        if self.power == "run":
            self.wakelocks[0] = 1
            ret = self.network.send(data)
            self.wakelocks[0] = 0
            return ret

    def sense(self):
        if self.power == "run":
            self.wakelocks[1] = 1
            data = self.sensor.sense()
            self.wakelocks[1] = 0
            return data

    def doze(self):
        print("dozing for {} seconds".format(self.sleeptime))
        self.power = "suspend"
        # wait until no peripherals hold a wakelock
        while sum(self.wakelocks) != 0:
            print("waiting for peripheral operations to complete. Wakelocks: [{}, {}]".format(self.wakelocks[0], self.wakelocks[1]))

        # sleep for 90% of the time
        sleep(self.sleeptime)
        self.power = "run"





""" Peripheral drivers called via API """ 
class Network:
    def send(self, data):
        print("Network is sending...")
        sleep(.5) # assume peripherals block
        return 0

class Sensor:
    def sense(self):
        print("Sensor is sensing...")
        sleep(.5)
        return "sensor data"

""" Run with attack mitigation system and without """

# Assumption: the application doesn't make more API requests while waiting for others to complete

Network = Network()
Sensor = Sensor()
Mitigator = Mitigator([Network,Sensor])
App = Application(Mitigator)

while True:
    App.run()



