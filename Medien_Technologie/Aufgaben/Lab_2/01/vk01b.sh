#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: $0 <input.yuv>"
    exit 1
fi

input="$1"
basename=$(basename "$input" .yuv)

# QP-Werte (decken die geforderten Schrittweiten ab)
qps=(2 8 14 20 26 32 38 44 51)

# Output-Datei für RD-Daten
echo "QP Size(bytes) PSNR_Y" > rd_results.txt

for qp in "${qps[@]}"; do
    echo "Kodierung mit QP=$qp"

    logfile="log_qp_${qp}.txt"
    output="${basename}_qp${qp}.264"

    x264 "$input" \
        --qp "$qp" \
        --input-res 352x288 \
        --psnr \
        -o "$output" 2> "$logfile"

    # Dateigröße holen
    size=$(stat -c%s "$output")

    # PSNR Y extrahieren
    psnr=$(grep "PSNR Mean Y" "$logfile" | awk '{print $5}')

    echo "$qp $size $psnr" >> rd_results.txt

done

echo "Fertig! Ergebnisse in rd_results.txt"
