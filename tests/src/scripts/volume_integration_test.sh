#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."
if [ "$#" -gt 1 ] || { [ "$#" -eq 1 ] && [ "$1" != --skip-exfat ]; }; then
    echo "Usage: $0 [--skip-exfat]" >&2
    exit 2
fi
test_directory=$(mktemp -d /private/tmp/karabiner-volume-test.XXXXXX)
mounted_paths=()

cleanup() {
    local status=$?
    local failed=0
    trap - EXIT
    for ((i=${#mounted_paths[@]}-1; i>=0; i--)); do
        if ! hdiutil detach "${mounted_paths[$i]}"; then
            failed=1
        fi
    done
    if [ "$failed" -eq 0 ]; then
        # Only remove the mktemp directory after all test volumes are unmounted.
        case "$test_directory" in
            /private/tmp/karabiner-volume-test.*) rm -rf -- "$test_directory" ;;
        esac
    else
        echo "Unmount failed; retained test images at $test_directory" >&2
        status=1
    fi
    exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

create_volume() {
    local name=$1
    local filesystem=$2
    local owners=$3
    local image="$test_directory/$name.sparseimage"
    local mountpoint="$test_directory/$name"
    mkdir "$mountpoint"
    local layout=GPTSPUD
    if [ "$filesystem" = 'MS-DOS FAT32' ] || [ "$filesystem" = ExFAT ]; then
        layout=MBRSPUD
    fi
    local volume_name="KE-Test-$name"
    if [ "$filesystem" = ExFAT ]; then
        # Use a short alphanumeric name accepted by macOS's exFAT formatter.
        volume_name=KETESTEXFAT
    fi
    hdiutil create -size 128m -type SPARSE -layout "$layout" -fs "$filesystem" -volname "$volume_name" "$image"
    hdiutil attach "$image" -mountpoint "$mountpoint" -nobrowse -owners "$owners"
    mounted_paths+=("$mountpoint")
}

# Mount options affect only these disposable images; no diskutil ownership
# database changes or operations on existing user volumes are performed.
create_volume apfs-owners APFS on
create_volume apfs-noowners APFS off
create_volume fat32 'MS-DOS FAT32' off
if [ "${1:-}" = --skip-exfat ]; then
    echo 'SKIP: exFAT (explicitly requested)'
else
    create_volume exfat ExFAT off
fi

./build/volume_integration_test "$test_directory" "$@"
