"""
usage: ./simulation.py <mitigate> <capacitor_max> <starting energy> <harvestable energy>
"""

send_interval = 10

mitigate = bool(sys.argv[1])

energy = float(sys.argv[3])  # uJ
capacitor_max = float(sys.argv[2])
critical_energy_level = 36.0

harvestable = float(sys.argv[4]) # uJ energy per second

off_seconds = 90
delta = 1

cpu_energy = 1.2 # amount of energy (mu amps) used by 1 cpu millisecond
radio_transmit_energy = 27.5 # amount of energy used by a radio per millisecond
sensor_reading_energy = 198.0 # uJoules per sensor reading

time = 0
packets = 0

fh = open("measurements.dat", 'a')

def check_energy():
    global energy
    energy = min(capacitor_max, energy)
    print("Time: {}\nPackets: {}".format(time, packets))
    # check if ran out of energy
    if energy <= 0:
        print("out of energy")
        # TODO output to data file
        fh.write("{}\t{}\n".format(time,packets))
        fh.close()
        exit(0)
    if time > 600: # more than 10 minutes have passed
        print("Exiting: runs for at least 10 minutes")
        fh.write("{}\t{}\n".format(600,packets))
        fh.close()
        exit(0)
        

def LPM(seconds):
    # sleep for a second, update energy, check if out of energy
    global time
    global energy
    for i in range(0, seconds):
        time += 1
        energy -= 2 * cpu_energy # 2 ms cpu time per second in LPM
        energy += harvestable
        check_energy()

def AIMD():
    global off_seconds
    if energy <= critical_energy_level:
        off_seconds *= 2
    elif off_seconds - delta > 0:
        off_seconds -= delta
    

def trial(mitigate, capacitor_max, energy, harvestable): 
    # append parameters to data file
    fh.write("{}\t{}\t{}\t{}\t".format(mitigate, capacitor_max, energy, harvestable))
    global energy
    global packets
    while True:
        ##### SENSOR #####

        # if attack ongoing, sleep
        if mitigate:
            LPM(off_seconds)
            AIMD()

        # normal application sleep
        LPM(send_interval)

        # sensor
        energy -= sensor_reading_energy
        check_energy()

        ##### RADIO #####

        # if attack ongoing, sleep
        if mitigate:
            LPM(off_seconds)
            AIMD()

        # normal application sleep
        LPM(send_interval)

        # send packet
        energy -= radio_transmit_energy * 3
        packets += 1
        check_energy()

trial(mitigate, capacitor_max, energy, harvestable)
