#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <input_file>"
    exit 1
fi

INPUT=$1
BASENAME="${1%.*}"

# Search boundaries in bps (as required by Nero)
LOW=8000
HIGH=128000*2
STEP=2000 # 2 kbit/s precision

echo "Starting Binary Search for: $INPUT"
echo "Target precision: $STEP bps"

while [ $(($HIGH - $LOW)) -gt $STEP ]; do
    # Calculate Midpoint
    MID=$(( ($LOW + $HIGH) / 2 ))
    # Round to nearest 2000 for consistency
    MID=$(( ($MID / $STEP) * $STEP ))
    
    KBR=$(($MID / 1000))
    OUTPUT="${BASENAME}_${KBR}k.mp4"

    echo "--------------------------------------"
    echo "Testing: $KBR kbps (Range: $(($LOW/1000))k - $(($HIGH/1000))k)"
    
    # Calculate the bitrate in kbit/s for FFmpeg
	KBR=$(($MID / 1000))

	# The FFmpeg command
	ffmpeg -y -i "$INPUT" -c:a aac -b:a "${KBR}k" "$OUTPUT" > /dev/null 2>&1

    # Play the file (using ffplay or your preferred player)
    echo "Listen to the file..."
    ffplay -nodisp -vn -autoexit "$OUTPUT" > /dev/null 2>&1

    # Ask for user input
    read -p "Is the quality SUFFICIENT? (y = yes, still okay / n = no, unhearable): " CHOICE

    if [[ "$CHOICE" == "y" || "$CHOICE" == "Y" ]]; then
        # If it's good, try to go lower to find the breaking point
        BEST_FOUND=$KBR
        HIGH=$MID
    else
        # If it's bad, we must go higher
        LOW=$MID
    fi
    		# Clean up temp file
		rm "$OUTPUT"
done

OUTPUT="${BASENAME}_${BEST_FOUND}k.mp4"

ffmpeg -y -i "$INPUT" -c:a aac -b:a "${BEST_FOUND}k" "$OUTPUT"


echo "--------------------------------------"
echo "SEARCH FINISHED!"
echo "The threshold bitrate for '$INPUT' is: ${BEST_FOUND} kbps"
