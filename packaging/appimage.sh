#!/usr/bin/env bash
set -euo pipefail
build_dir=${1:-build}
output_dir=${2:-artifacts}
repo_dir=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$output_dir" "$build_dir/appimage-tools"
build_dir=$(cd "$build_dir" && pwd)
output_dir=$(cd "$output_dir" && pwd)
app_dir="$build_dir/AppDir"
# Use DESTDIR so the build never installs files into the host /usr.
DESTDIR="$app_dir" cmake --install "$build_dir" --prefix /usr --strip --config Release
for name in linuxdeploy linuxdeploy-plugin-qt; do
    tool="$build_dir/appimage-tools/$name-x86_64.AppImage"
    if [[ ! -f "$tool" ]]; then
        curl --fail --location --retry 3 "https://github.com/linuxdeploy/$name/releases/download/continuous/$name-x86_64.AppImage" -o "$tool"
        chmod +x "$tool"
    fi
done
export PATH="$build_dir/appimage-tools:$PATH"
export QMAKE=${QMAKE:-$(command -v qmake6 || command -v qmake)}
export APPIMAGE_EXTRACT_AND_RUN=1
export EXTRA_QT_MODULES="svg;waylandclient"
export EXTRA_PLATFORM_PLUGINS="libqoffscreen.so;libqwayland-egl.so;libqwayland-generic.so"
export LDAI_OUTPUT="FileSharing-2.0.0-linux-x86_64.AppImage"
cd "$output_dir"
"$build_dir/appimage-tools/linuxdeploy-x86_64.AppImage" --appdir "$app_dir" \
    --desktop-file "$repo_dir/packaging/filesharing.desktop" --icon-file "$app_dir/usr/share/icons/hicolor/scalable/apps/filesharing.svg" \
    --plugin qt --output appimage
