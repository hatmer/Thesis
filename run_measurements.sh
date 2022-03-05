#!/bin/bash

MITI_OFF=0
MITI_ON=1

HARVESTABLE_START=1 #uJ
HARVESTABLE_END=6 #uJ

for ((harvestable=$HARVESTABLE_START;harvestable<=$HARVESTABLE_END;harvestable++))
do
  python simulation.py 0 225 225 $harvestable 36.0 && python simulation.py 1 225 225 $harvestable 36.0
done


