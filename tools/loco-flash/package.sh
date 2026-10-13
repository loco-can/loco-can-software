#!/bin/sh
# Build standalone LocoFlash archives. Each archive is one program file
# plus the license. Nothing else has to be installed.
set -eu
cd "$(dirname "$0")"
root=$(pwd)
repo=$(CDPATH= cd "$root/../.." && pwd)
out="$root/dist"
rm -rf "$out"
mkdir -p "$out"

build() {
  os=$1
  arch=$2
  name=$3
  ldflags=$4
  dir="$out/LocoFlash-${os}-${arch}"
  mkdir -p "$dir"
  CGO_ENABLED=0 GOOS="$os" GOARCH="$arch" \
    go build -trimpath -ldflags "$ldflags" -o "$dir/$name" .
  cp "$root/LIESMICH.txt" "$dir/LIESMICH.txt"
  cp "$repo/license/LICENSE" "$dir/LICENSE"
}

build windows amd64 LocoFlash.exe "-s -w -H windowsgui"
build linux amd64 LocoFlash "-s -w"
build darwin amd64 LocoFlash "-s -w"
build darwin arm64 LocoFlash "-s -w"

cd "$out"
for dir in LocoFlash-*; do
  if [ -f "$dir/LocoFlash.exe" ]; then
    zip -qr "$dir.zip" "$dir"
  else
    tar -czf "$dir.tar.gz" "$dir"
  fi
done
echo "packages in $out"
