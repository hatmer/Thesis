# usage: python {} <mode> <harvestableEnergyDuringAttack> <attackFrequency> <attackDuration> <demoDuration> <outputFileName>
DEMOTIME=(24*60)-1

# baseline
python demo.py nomitigation 0 0 0 $DEMOTIME baseline.csv

# no harvestable energy during attack

# hostile gardener: single 30-minute attack
python demo.py mitigation 0 100 30 $DEMOTIME hostileGardenerBlackout.csv

# hostile satellite: multiple 5-minute attacks
python demo.py mitigation 0 60 5 $DEMOTIME hostileSatelliteBlackout.csv

# endless attack: one attack that never ends
python demo.py mitigation 0 100 $DEMOTIME-100 $DEMOTIME endlessBlackout.csv


# some harvestable energy during attack

# hostile gardener: single 30-minute attack
python demo.py mitigation 2 100 30 $DEMOTIME hostileGardenerTrickle.csv

# hostile satellite: multiple 5-minute attacks
python demo.py mitigation 2 60 5 $DEMOTIME hostileSatelliteTrickle.csv

# endless attack: one attack that never ends
python demo.py mitigation 2 100 $DEMOTIME-100 $DEMOTIME endlessTrickle.csv


# lots of harvestable energy during attack

# hostile gardener: single 30-minute attack
python demo.py mitigation 4 100 30 $DEMOTIME hostileGardenerSurplus.csv

# hostile satellite: multiple 5-minute attacks
python demo.py mitigation 4 60 5 $DEMOTIME hostileSatelliteSurplus.csv

# endless attack: one attack that never ends
python demo.py mitigation 4 100 $DEMOTIME-100 $DEMOTIME endlessSurplus.csv
