#FIRST=$RANDOM
FIRST=1
LAST=$((FIRST+1000))

# first run once to compile if needed
sim.sh
# now run the batch
seq $FIRST 1 $LAST | parallel sim.sh | grep ERROR
