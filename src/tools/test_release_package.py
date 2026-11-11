# Python / test_release_package.py / Python 3.10+ / temporary release publication tests
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import release_package


class ReleaseTest(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='hazzure-package-test-')
        self.root=Path(self.temp.name);self.source=self.root/'source.bin';self.source.write_bytes(b'new')
        self.output=self.root/'release'
    def tearDown(self):self.temp.cleanup()
    def test_new_release_has_verified_manifest(self):
        result=release_package.assemble({'module.bin':self.source},self.output,self.root)
        manifest=json.loads(Path(result['manifest']).read_text())
        self.assertEqual(manifest['files'][0]['sha256'],release_package.sha256(self.source))
        self.assertEqual((self.output/'module.bin').read_bytes(),b'new')
    def test_existing_release_and_profile_are_preserved(self):
        self.output.mkdir();(self.output/'old.bin').write_bytes(b'old')
        (self.output/'settings.json').write_text('{"custom":true}')
        defaults=self.root/'default.json';defaults.write_text('{"custom":false}')
        result=release_package.assemble({'module.bin':self.source,'settings.json':defaults},self.output,self.root)
        self.assertTrue(json.loads((self.output/'settings.json').read_text())['custom'])
        self.assertEqual((Path(result['previous'])/'old.bin').read_bytes(),b'old')
    def test_missing_input_does_not_touch_current_release(self):
        self.output.mkdir();old=self.output/'old.bin';old.write_bytes(b'old')
        with self.assertRaises(FileNotFoundError):
            release_package.assemble({'missing.bin':self.root/'missing'},self.output,self.root)
        self.assertEqual(old.read_bytes(),b'old')
    def test_failed_publication_restores_current_release(self):
        self.output.mkdir();(self.output/'old.bin').write_bytes(b'old')
        replace=release_package.os.replace
        def fail_staging(source,target):
            if '.staging-' in Path(source).name:raise OSError('simulated publication failure')
            return replace(source,target)
        with patch('release_package.os.replace',side_effect=fail_staging),self.assertRaises(OSError):
            release_package.assemble({'module.bin':self.source},self.output,self.root)
        self.assertEqual((self.output/'old.bin').read_bytes(),b'old')
    def test_path_escape_and_workspace_output_rejected(self):
        for destination in (self.root,self.root.parent/'outside-release'):
            with self.assertRaises(ValueError):release_package.assemble({'a':self.source},destination,self.root)
        for member in ('../outside','C:/outside','/absolute','release-manifest.json'):
            with self.assertRaises(ValueError):release_package.assemble({member:self.source},self.output,self.root)
    def test_failed_copy_preserves_current_release(self):
        self.output.mkdir();(self.output/'old.bin').write_bytes(b'old')
        with patch('release_package.shutil.copy2',side_effect=OSError('copy failed')),self.assertRaises(OSError):
            release_package.assemble({'module.bin':self.source},self.output,self.root)
        self.assertEqual((self.output/'old.bin').read_bytes(),b'old')


if __name__=='__main__':unittest.main(verbosity=2)
