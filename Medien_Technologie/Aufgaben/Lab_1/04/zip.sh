#!/bin/bash

# Hardcoded submission data
STUDIENGANG="ITS-B"
NACHNAME="Lux"
VORNAME="Alexander"
MATRIKEL="2410555044"

ZIPNAME="${STUDIENGANG}_${NACHNAME}_${VORNAME}_${MATRIKEL}.zip"

# Check for folder name argument (e.g., 04)
if [ -z "$1" ]; then
    echo "Usage: $0 <folder_name>"
    exit 1
fi

VIRTUAL_DIR="$1"
TEMP_BUILD=".temp_zip_build"

# 1. Clean up any previous runs
rm -rf "$TEMP_BUILD"
rm -f "$ZIPNAME"

# 2. Create a temporary structure
mkdir -p "$TEMP_BUILD/$VIRTUAL_DIR"

# 3. Copy files to the temporary structure (instead of moving them)
cp *.c Makefile "$TEMP_BUILD/$VIRTUAL_DIR/" 2>/dev/null
# Optional: include headers if you have them
cp *.h "$TEMP_BUILD/$VIRTUAL_DIR/" 2>/dev/null

# 4. Handle the libs folder logic
if [ -d "libs/include" ] || [ -d "libs/lib" ]; then
    cp -r libs "$TEMP_BUILD/"
fi

# 5. Run make clean inside the temp folder to ensure no binaries are zipped
if [ -f "$TEMP_BUILD/$VIRTUAL_DIR/Makefile" ]; then
    make -C "$TEMP_BUILD/$VIRTUAL_DIR" clean > /dev/null 2>&1
fi

# 6. Create the ZIP from the temp directory
# -j (junk paths) isn't used here because we WANT the structure
(cd "$TEMP_BUILD" && zip -r "../$ZIPNAME" .) > /dev/null

# 7. Final Cleanup
rm -rf "$TEMP_BUILD"

echo "-------------------------------------------"
echo "ZIP created: $ZIPNAME"
echo "Structure inside: $VIRTUAL_DIR/ and libs/"
echo "Local files remain unmoved."
echo "-------------------------------------------"
