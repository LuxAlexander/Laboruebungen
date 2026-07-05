#!/bin/bash

# Überprüfung, ob genug Argumente übergeben wurden
if [ "$#" -ne 2 ]; then
    echo "Benutzung: $0 <input_file.wav> <profil>"
    echo "Profile: lc, he, hev2"
    exit 1
fi

INPUT=$1
PROFIL=$2
FILENAME=$(basename -- "$INPUT")
EXTENSION="${FILENAME##*.}"
BASENAME="${FILENAME%.*}"

# Profil-Flag bestimmen
case $PROFIL in
    lc)
        FLAG="-lc"
        ;;
    he)
        FLAG="-he"
        ;;
    hev2)
        FLAG="-hev2"
        ;;
    *)
        echo "Fehler: Ungültiges Profil. Nutze lc, he oder hev2."
        exit 1
        ;;
esac

# Array mit den gewünschten Bitraten (in bps für Nero)
BITRATES=(16000 32000 64000)

echo "Starte Enkodierung für: $INPUT mit Profil $PROFIL"
echo "------------------------------------------------"

for BR in "${BITRATES[@]}"; do
    # Umrechnung für den Dateinamen (bps -> kbps)
    KBR=$(($BR / 1000))
    OUTPUT="${BASENAME}_${PROFIL}_${KBR}k.mp4"

    echo "Erstelle: $OUTPUT ..."

    # Ausführung des Encoders (Pfad ggf. anpassen, falls neroAacEnc nicht im Ordner ist)
    ./neroAacEnc $FLAG -br $BR -if "$INPUT" -of "$OUTPUT"

    if [ $? -eq 0 ]; then
        echo "Fertig: $OUTPUT"
    else
        echo "Fehler bei $KBR kbit/s"
    fi
done

echo "------------------------------------------------"
echo "Alle Aufgaben abgeschlossen."
