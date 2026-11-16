#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."
if [ "$#" -gt 1 ] || { [ "$#" -eq 1 ] && [ "$1" != --skip-exfat ]; }; then
    echo "Usage: $0 [--skip-exfat]" >&2
    exit 2
fi
test_directory=$(mktemp -d /private/tmp/karabiner-volume-test.XXXXXX)

cleanup() {
    local status=$?
    trap - EXIT
    # Inspect actual attachments, not a list updated after attach returns: attach
    # can fail or be interrupted after creating a device or mounting a volume.
    if /usr/bin/python3 scripts/osx_volume_cleanup.py "$test_directory"; then
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

./build/osx_volume_integration_test "$test_directory" "$@"

# Reuse the populated image so read-only mounting does not require fixture writes.
hdiutil detach "$test_directory/apfs-owners"
hdiutil attach "$test_directory/apfs-owners.sparseimage" \
    -mountpoint "$test_directory/apfs-owners" -nobrowse -owners on \
    -readonly
diskutil info -plist "$test_directory/apfs-owners" > "$test_directory/restricted-volume.plist"
volume_device=$(/usr/libexec/PlistBuddy -c 'Print :DeviceIdentifier' "$test_directory/restricted-volume.plist")
if [[ ! "$volume_device" =~ ^disk[0-9]+s[0-9]+$ ]]; then
    echo "Unexpected test volume device: $volume_device" >&2
    exit 1
fi
# The device is resolved from this test's mounted image, never caller supplied.
diskutil unmount "$test_directory/apfs-owners"
diskutil mount readOnly -mountOptions noexec,nosuid,nobrowse \
    -mountPoint "$test_directory/apfs-owners" "/dev/$volume_device"
./build/osx_volume_integration_test "$test_directory" --restricted
