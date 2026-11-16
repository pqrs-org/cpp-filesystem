"""Detach only images belonging to one integration test, including partial attaches."""

import pathlib
import plistlib
import re
import subprocess
import sys


def attached_test_images(directory):
    result = subprocess.run(
        ["hdiutil", "info", "-plist"], check=True, stdout=subprocess.PIPE
    )
    images = plistlib.loads(result.stdout)["images"]
    return [
        image
        for image in images
        if pathlib.Path(image["image-path"]).resolve().parent == directory
    ]


def detach_test_images(directory):
    failed = False
    for image in reversed(attached_test_images(directory)):
        # hdiutil lists the whole image device before its partitions. An image
        # that failed to mount may have dev-entry but no mount-point at all.
        device = next(
            (
                entity["dev-entry"]
                for entity in image.get("system-entities", [])
                if re.fullmatch(r"/dev/disk[0-9]+", entity.get("dev-entry", ""))
            ),
            None,
        )
        if device is None:
            print("Cannot identify test image device: " + image["image-path"], file=sys.stderr)
            failed = True
        elif subprocess.run(["hdiutil", "detach", device]).returncode != 0:
            failed = True
    # Fail closed: retain images if anything remains or querying state fails.
    return not failed and not attached_test_images(directory)


def main():
    directory = pathlib.Path(sys.argv[1]).resolve()
    if directory.parent != pathlib.Path("/private/tmp") or not directory.name.startswith(
        "karabiner-volume-test."
    ):
        raise ValueError("Unexpected test directory: " + str(directory))
    return 0 if detach_test_images(directory) else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print("Cannot safely clean up test volumes: " + str(error), file=sys.stderr)
        sys.exit(1)
