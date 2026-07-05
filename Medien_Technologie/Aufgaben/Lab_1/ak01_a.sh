#!/bin/bash
echo "Create Audio Files AAC from $1.wav"

for i in 16 32 64; do
    ffmpeg -i "$1.wav" -b:a ${i}k -acodec aac "${1}_output_${i}.aac"
    echo "Created ${i}k bitrate"
done

