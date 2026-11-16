import pathlib
import plistlib
import subprocess
import unittest
from unittest.mock import patch

import osx_volume_cleanup as cleanup


class CleanupTests(unittest.TestCase):
    directory = pathlib.Path("/private/tmp/karabiner-volume-test.mock")

    def image(self, directory, device, mounted=False):
        entity = {"dev-entry": device}
        if mounted:
            entity["mount-point"] = str(directory / "apfs-owners")
        return {"image-path": str(directory / "image.sparseimage"), "system-entities": [entity]}

    def test_partial_attach_without_mountpoint(self):
        owned = self.image(self.directory, "/dev/disk100")
        unrelated = self.image(pathlib.Path("/private/tmp/another-test"), "/dev/disk101", True)
        before = plistlib.dumps({"images": [owned, unrelated]})
        after = plistlib.dumps({"images": [unrelated]})
        with patch.object(cleanup.subprocess, "run") as run:
            run.side_effect = [subprocess.CompletedProcess([], 0, before),
                               subprocess.CompletedProcess([], 0),
                               subprocess.CompletedProcess([], 0, after)]
            self.assertTrue(cleanup.detach_test_images(self.directory))
            self.assertEqual(run.call_args_list[1].args[0], ["hdiutil", "detach", "/dev/disk100"])
            self.assertEqual(run.call_count, 3)

    def test_failed_detach_retains_images(self):
        with patch.object(cleanup, "attached_test_images", return_value=[self.image(self.directory, "/dev/disk100", True)]), \
             patch.object(cleanup.subprocess, "run", return_value=subprocess.CompletedProcess([], 1)):
            self.assertFalse(cleanup.detach_test_images(self.directory))

    def test_successful_detach_with_remaining_attachment_is_not_safe(self):
        with patch.object(cleanup, "attached_test_images", return_value=[self.image(self.directory, "/dev/disk100")]), \
             patch.object(cleanup.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)):
            self.assertFalse(cleanup.detach_test_images(self.directory))

    def test_query_failure_does_not_allow_removal(self):
        with patch.object(cleanup.subprocess, "run", side_effect=subprocess.CalledProcessError(1, "hdiutil")):
            with self.assertRaises(subprocess.CalledProcessError):
                cleanup.detach_test_images(self.directory)


if __name__ == "__main__":
    unittest.main()
