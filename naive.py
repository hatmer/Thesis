"""
Simulation: show impact of solution on data yield

"""
from random import randint
from time import sleep, time
import sys

DesiredQoS = .2 # 20% minimum required data yield
OneRoundOfOpsTime = 1 # assume 1 seconds needed to sense and relay data for demo app
mode = "mitigation"
demoDuration = 60 # seconds

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
"""
        # simulate periodic attack occuring
        if mode == "mitigation":
            if self.mitigator.power == "run" and int(time()) % 10 == 0: # randint(1,5) == 1:
                print("Attack detected!")
                simulatedAttackEnd = time() + 12 # randint(1, 30)
                while time() < simulatedAttackEnd:
                    self.mitigator.doze()
                    print("waking for {} seconds".format(self.mitigator.waketime))
                    begin = time()
                    while time() < begin + self.mitigator.waketime:
                        self.normalOps()
                    
                print("Attack is over, resuming normal operations")
"""

""" Attack Mitigation system """
class Mitigator:
    def __init__(self, drivers):
        self.network = drivers[0]
        self.sensor = drivers[1]
        self.power = "run" # run / suspend
        self.buf = []

        # calculate sleep and wake time. Assuming that waking ops in one chunk is same QoS as multiple smaller wakeups
        self.sleeptime = ((1-DesiredQoS) * 10) * OneRoundOfOpsTime  # how long to sleep before waking up
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

    def doze(self):
        print("dozing for {} seconds".format(self.sleeptime))
        self.power = "suspend"

        # sleep for 90% of the time
        sleep(self.sleeptime)
        self.power = "run"



""" Peripheral drivers called via API """ 
class Network:
    def __init__(self):
        self.msgCount = 0 # amount of data sent
        self.beginTime = time()

    def send(self, data):
        print("Network is sending...")
        sleep(.5) # assume peripherals block
        self.msgCount += 1
        
        rate = 8 / 10.715
        duration = time() - self.beginTime
        dataYield = self.msgCount / (duration * rate) * 100

        print("Message count is {} messages after {} seconds ({}%)".format(self.msgCount, round(duration,1), round(dataYield,1)))
        return 0

class Sensor:
    def sense(self):
        print("Sensor is sensing...")
        sleep(.5)
        return "sensor data"

""" Run with attack mitigation system and without """

# Assumption: the application doesn't make more API requests while waiting for others to complete

if __name__ == "__main__":
    Network = Network()
    Sensor = Sensor()
    Mitigator = Mitigator([Network,Sensor])
    App = Application(Mitigator)


    startTime = time()
    while time() < startTime + demoDuration:
        App.run()


