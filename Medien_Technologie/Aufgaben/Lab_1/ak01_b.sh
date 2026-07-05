#!/bin/bash

for file in *.wav; do
    ffmpeg -i "$file" -b:a 32k -codec:a libmp3lame "${file%.wav}.mp3"
done
