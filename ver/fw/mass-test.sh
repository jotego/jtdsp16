#FIRST=$RANDOM
FIRST=1
LAST=$((FIRST+1000))

# first run once to compile if needed
go.sh
# now run the batch
seq $FIRST 1 $LAST | parallel go.sh | grep ERROR