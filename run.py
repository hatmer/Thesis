
import os

MITI_OFF=0
MITI_ON=1

HARVESTABLE_START=0.0 #uJ
HARVESTABLE_END=20.0 #uJ


harvestable = HARVESTABLE_START
while harvestable <= HARVESTABLE_END:
    os.system("python simulation.py 0 225.0 56.25 {} 36.0".format(harvestable))
    harvestable += .01

