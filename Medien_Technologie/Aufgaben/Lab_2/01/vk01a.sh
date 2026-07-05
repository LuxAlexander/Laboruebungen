#!/bin/bash

# Bitraten
bitrates=(100 200 500)

# Eingabedatei
input="foreman.yuv"

# Loop über alle Bitraten
for i in "${bitrates[@]}"; do
    echo "Kodierung mit ${i} kbit/s..."

# x264 -o output.264 input.yuv
    x264 "$input" \
        --bitrate "$i" \
        --input-res 352x288 \
        -o "foreman_${i}.264" \
        --dump-yuv "foreman_${i}_decoded.yuv"

done
