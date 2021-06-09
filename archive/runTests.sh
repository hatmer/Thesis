# usage: python {} <mode> <harvestableEnergyDuringAttack> <attackFrequency> <attackDuration> <demoDuration> <outputFileName>
DEMOTIME=$((60*5-1))

# baseline
#python demo.py nomitigation 0 0 0 $DEMOTIME baseline.csv

# no harvestable energy during attack

# hostile gardener: single 30-minute attack
echo "hostile gardener blackout"
python demo.py mitigation 0 150 60 $DEMOTIME hostileGardenerBlackout.csv
echo "hostile satellite blackout"
# hostile satellite: multiple 5-minute attacks
#python demo.py mitigation 0 30 10 $DEMOTIME hostileSatelliteBlackout.csv
echo "endless attack blackout"
# endless attack: one attack that never ends
#python demo.py mitigation 0 30 $(($DEMOTIME-30)) $DEMOTIME endlessBlackout.csv


# some harvestable energy during attack
echo "hostile gardener trickle"
# hostile gardener: single 30-minute attack
#python demo.py mitigation 2 150 60 $DEMOTIME hostileGardenerTrickle.csv
echo "hostile satellite trickle"
# hostile satellite: multiple 5-minute attacks
#python demo.py mitigation 2 30 10 $DEMOTIME hostileSatelliteTrickle.csv
echo "endless attack trickle"
# endless attack: one attack that never ends
#python demo.py mitigation 2 30 $(($DEMOTIME-30)) $DEMOTIME endlessTrickle.csv


# lots of harvestable energy during attack
echo "hostile gardener surplus"
# hostile gardener: single 30-minute attack
#python demo.py mitigation 8 150 60 $DEMOTIME hostileGardenerSurplus.csv
echo "hostile satellite surplus"
# hostile satellite: multiple 5-minute attacks
#python demo.py mitigation 8 30 10 $DEMOTIME hostileSatelliteSurplus.csv
echo "endless attack surplus"
# endless attack: one attack that never ends
#python demo.py mitigation 8 30 $(($DEMOTIME-30)) $DEMOTIME endlessSurplus.csv
