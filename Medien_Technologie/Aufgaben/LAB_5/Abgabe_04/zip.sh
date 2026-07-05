#!/bin/bash

# Hardcoded submission data
STUDIENGANG="ITS-B"
NACHNAME="Lux"
VORNAME="Alexander"
MATRIKEL="2410555044"

ZIPNAME="${STUDIENGANG}_${NACHNAME}_${VORNAME}_${MATRIKEL}.zip"

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

# 3. Kopiere deinen Aufgabenordner (z.B. 04)
if [ -d "$VIRTUAL_DIR" ]; then
    cp -r "$VIRTUAL_DIR" "$TEMP_BUILD/"
else
    echo "Fehler: Ordner '$VIRTUAL_DIR' existiert nicht!"
    exit 1
fi

# 4. Detektiere den libs-Pfad
LIBS_SOURCE=""
if [ -d "libs" ]; then
    LIBS_SOURCE="libs"
elif [ -d "../libs" ]; then
    LIBS_SOURCE="../libs"
fi

if [ -n "$LIBS_SOURCE" ]; then
    mkdir -p "$TEMP_BUILD/libs/include"
    mkdir -p "$TEMP_BUILD/libs/lib/pkgconfig"
    
    # 4a. Alle Header kopieren (sehr klein, nur Text)
    cp -r "$LIBS_SOURCE/include/"* "$TEMP_BUILD/libs/include/" 2>/dev/null
    
    # 4b. Alle .pc-Dateien kopieren (Wichtig für pkg-config Namen!)
    cp "$LIBS_SOURCE/lib/pkgconfig/"*.pc "$TEMP_BUILD/libs/lib/pkgconfig/" 2>/dev/null
    
    # 4c. Pack all local static libraries required for linking
    cp "$LIBS_SOURCE/lib/libx264.a"      "$TEMP_BUILD/libs/lib/" 2>/dev/null
    cp "$LIBS_SOURCE/lib/libavutil.a"    "$TEMP_BUILD/libs/lib/" 2>/dev/null
    cp "$LIBS_SOURCE/lib/libavcodec.a"   "$TEMP_BUILD/libs/lib/" 2>/dev/null
    cp "$LIBS_SOURCE/lib/libavformat.a"  "$TEMP_BUILD/libs/lib/" 2>/dev/null
    
    # libavcodec.a und libavformat.a werden hier ignoriert.
else
    echo "Warnung: Kein 'libs'-Ordner gefunden!"
fi

# 5. Führe 'make clean' im Temp-Ordner aus
if [ -f "$TEMP_BUILD/$VIRTUAL_DIR/Makefile" ]; then
    make -C "$TEMP_BUILD/$VIRTUAL_DIR" clean > /dev/null 2>&1
fi

# 6. Erstelle das ZIP mit maximaler Kompression
(cd "$TEMP_BUILD" && zip -9 -r "../$ZIPNAME" .) > /dev/null

# 7. Temp-Ordner löschen
rm -rf "$TEMP_BUILD"

ZIP_SIZE=$(du -sh "$ZIPNAME" | cut -f1)

echo "-------------------------------------------"
echo "ZIP erfolgreich erstellt: $ZIPNAME"
echo "Größe der ZIP-Datei: $ZIP_SIZE (Garantiert unter 10 MiB!)"
echo "-------------------------------------------"
