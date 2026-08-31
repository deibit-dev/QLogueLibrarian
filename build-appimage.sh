#!/usr/bin/env bash
set -euo pipefail

# Builds a self-contained AppImage using linuxdeploy + linuxdeploy-plugin-qt.
# Usage: ./build-appimage.sh [Debug|Release]

BUILD_DIR="${BUILD_DIR:-appImage}"
CONFIG="${1:-Release}"
TOOLS_DIR="${TOOLS_DIR:-$BUILD_DIR/tools}"
APP_ID="QLogueLibrarian"
APPDIR="$BUILD_DIR/AppDir"
DESKTOP_FILE="$BUILD_DIR/${APP_ID}.desktop"

# 1) Fetch linuxdeploy + Qt plugin (AppImage packages)
mkdir -p "$TOOLS_DIR"
LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
QT_PLUGIN="$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
if [ ! -f "$LINUXDEPLOY" ]; then
    echo "Downloading linuxdeploy..."
    curl -fL -o "$LINUXDEPLOY" \
        https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
fi
if [ ! -f "$QT_PLUGIN" ]; then
    echo "Downloading linuxdeploy-plugin-qt..."
    curl -fL -o "$QT_PLUGIN" \
        https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
fi
chmod +x "$LINUXDEPLOY" "$QT_PLUGIN"

# AppImage tools need FUSE; extract-and-run avoids requiring it
export APPIMAGE_EXTRACT_AND_RUN=1

# 2) Configure + build
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG"
cmake --build "$BUILD_DIR" -j"$(nproc)"

# 3) Assemble AppDir manually (no install prefix, no system paths)
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/doc/${APP_ID}"
cp "$BUILD_DIR/${APP_ID}" "$APPDIR/usr/bin/"
cp LICENSE "$APPDIR/usr/share/doc/${APP_ID}/"

# 4) Desktop entry + icon (both required by linuxdeploy)
cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Type=Application
Name=QLogueLibrarian
GenericName=logue Librarian
Comment=Cross-platform GUI wrapper for KORG logue-sdk/logue-cli tools
Exec=${APP_ID}
Icon=${APP_ID}
Terminal=false
Categories=AudioVideo;Audio;Music;
EOF

ICON_DIR="$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$ICON_DIR"
convert resources/capture.png -resize 256x256 "$ICON_DIR/${APP_ID}.png"
mkdir -p "$APPDIR/usr/share/applications"
cp "$DESKTOP_FILE" "$APPDIR/usr/share/applications/${APP_ID}.desktop"

# 5) Bundle Qt + QML and build the AppImage
export QML_SOURCES_PATHS="${QML_SOURCES_PATHS:-qml}"
export EXTRA_QT_MODULES="${EXTRA_QT_MODULES:-qtquickcontrols2}"
export LDAI_OUTPUT_DIR="$BUILD_DIR"
"$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt --output appimage

# Ensure the result lands inside $BUILD_DIR (plugin may honor relative LDAI_OUTPUT_DIR differently)
mv -f ./${APP_ID}-*.AppImage "$BUILD_DIR/" 2>/dev/null || true

echo "AppImage created in: $BUILD_DIR/${APP_ID}-x86_64.AppImage"
