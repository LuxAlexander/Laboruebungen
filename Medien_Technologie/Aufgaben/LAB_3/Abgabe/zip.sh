#!/bin/bash

# Hardcoded submission data
STUDIENGANG="ITS-B"
NACHNAME="Lux"
VORNAME="Alexander"
MATRIKEL="2410555044"

# Der Name der ZIP-Datei
ZIPNAME="${STUDIENGANG}_${NACHNAME}_${VORNAME}_${MATRIKEL}.zip"

# Überprüfung, ob der Aufgabenordner (z.B. 03) als Argument übergeben wurde
if [ -z "$1" ]; then
    echo "Usage: $0 <folder_name>"
    exit 1
fi

VIRTUAL_DIR="$1"
TEMP_BUILD=".temp_zip_build"

# 1. Bereinigung alter Durchläufe
rm -rf "$TEMP_BUILD"
rm -f "$ZIPNAME"

# 2. Erstelle das leere Temp-Verzeichnis
mkdir -p "$TEMP_BUILD"

# 3. Kopiere deinen lokalen Aufgabenordner (z.B. 03) direkt in den Temp-Ordner
if [ -d "$VIRTUAL_DIR" ]; then
    cp -r "$VIRTUAL_DIR" "$TEMP_BUILD/"
else
    echo "Fehler: Lokaler Ordner '$VIRTUAL_DIR' existiert nicht!"
    exit 1
fi

# 4. Kopiere den lokalen libs-Ordner ebenfalls direkt in den Temp-Ordner
# 4. Kopiere den lokalen libs-Ordner (NUR include und lib, ignoriere share)
if [ -d "libs" ]; then
    mkdir -p "$TEMP_BUILD/libs"
    cp -r libs/include "$TEMP_BUILD/libs/" 2>/dev/null
    cp -r libs/lib "$TEMP_BUILD/libs/" 2>/dev/null
elif [ -d "../libs" ]; then
    mkdir -p "$TEMP_BUILD/libs"
    cp -r ../libs/include "$TEMP_BUILD/libs/" 2>/dev/null
    cp -r ../libs/lib "$TEMP_BUILD/libs/" 2>/dev/null
fi

# 5. Führe 'make clean' im Temp-Ordner aus, um Binaries vor dem Zippen zu löschen
if [ -f "$TEMP_BUILD/$VIRTUAL_DIR/Makefile" ]; then
    make -C "$TEMP_BUILD/$VIRTUAL_DIR" clean > /dev/null 2>&1
fi

# 6. Erstelle das ZIP aus dem Temp-Verzeichnis heraus
# Der Punkt '.' sorgt dafür, dass der INHALT von $TEMP_BUILD flach gezippt wird
(cd "$TEMP_BUILD" && zip -r "../$ZIPNAME" .) > /dev/null

# 7. Endgültige Bereinigung des temporären Ordners
rm -rf "$TEMP_BUILD"

echo "-------------------------------------------"
echo "ZIP erfolgreich erstellt: $ZIPNAME"
echo "Deine lokalen Ordner wurden NICHT verändert."
echo "Struktur beim Entpacken der ZIP-Datei:"
echo "├── $VIRTUAL_DIR/ (mit .c und bereinigtem Makefile)"
echo "└── libs/"
echo "-------------------------------------------"
