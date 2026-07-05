#!/bin/bash

# Überprüfung der Argumente
if [ "$#" -ne 1 ]; then
    echo "Benutzung: $0 <input_file.mp3>"
    exit 1
fi

INPUT="$1"
BASENAME="${INPUT%.*}"

# 1. Original-Bitrate ermitteln (für Aufgabe ii)
# Wir nutzen ffprobe, um die Bitrate der Quelldatei zu lesen
ORIG_BR_BPS=$(ffprobe -v error -select_streams a:0 -show_entries stream=bit_rate -of default=noprint_wrappers=1:nokey=1 "$INPUT")
ORIG_BR_KBIT=$(($ORIG_BR_BPS / 1000))

echo "--- Starte Kodierung für: $INPUT ---"
echo "Originale Bitrate erkannt: ${ORIG_BR_KBIT} kbps"

# --- Aufgabe i: AAC mit 256 kbps ---
OUTPUT_AAC="${BASENAME}_256k.m4a"
ffmpeg -y -i "$INPUT" -c:a aac -b:a 256k -vn "$OUTPUT_AAC" > /dev/null 2>&1

# --- Aufgabe ii: MP3 mit Originalbitrate ---
OUTPUT_MP3="${BASENAME}_original.mp3"
ffmpeg -y -i "$INPUT" -c:a libmp3lame -b:a "${ORIG_BR_KBIT}k" -vn "$OUTPUT_MP3" > /dev/null 2>&1

echo "--- Analyse der Ergebnisse (ffprobe) ---"

# Funktion zur Bitraten-Extraktion
get_bitrate() {
    ffprobe -v error -select_streams a:0 -show_entries stream=bit_rate -of default=noprint_wrappers=1:nokey=1 "$1"
}

# Tatsächliche Werte auslesen
ACTUAL_AAC=$(get_bitrate "$OUTPUT_AAC")
ACTUAL_MP3=$(get_bitrate "$OUTPUT_MP3")

# Ergebnisse ausgeben
printf "%-20s | %-15s | %-15s\n" "Datei" "Soll-Bitrate" "Ist-Bitrate (bps)"
printf -- "-------------------------------------------------------------\n"
printf "%-20s | %-15s | %-15s\n" "$OUTPUT_AAC" "256 kbps" "$ACTUAL_AAC"
printf "%-20s | %-15s | %-15s\n" "$OUTPUT_MP3" "${ORIG_BR_KBIT} kbps" "$ACTUAL_MP3"

echo "-------------------------------------------------------------"
echo "Aufgabe abgeschlossen."
