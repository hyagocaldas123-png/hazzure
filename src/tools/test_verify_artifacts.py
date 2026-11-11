# Python / test_verify_artifacts.py / Python 3.10+ / temporary artifact checks
import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest

from verify_artifacts import collect, compare_group, inspect_json, main


class ArtifactTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix='hazzure-provenance-')
        self.root = Path(self.directory.name)

    def tearDown(self):
        self.directory.cleanup()

    def test_identical_copies_have_matching_hashes(self):
        first, second = self.root / 'a.bin', self.root / 'b.bin'
        first.write_bytes(b'same'); second.write_bytes(b'same')
        group = compare_group([first, second])
        self.assertEqual(group['status'], 'identical')
        self.assertEqual(group['files'][0]['sha256'], group['files'][1]['sha256'])

    def test_different_copies_fail_without_synchronizing(self):
        first, second = self.root / 'a.bin', self.root / 'b.bin'
        first.write_bytes(b'one'); second.write_bytes(b'two')
        self.assertEqual(compare_group([first, second])['status'], 'different')
        self.assertEqual(first.read_bytes(), b'one')
        self.assertEqual(second.read_bytes(), b'two')

    def test_missing_is_not_an_identical_group(self):
        self.assertEqual(compare_group([self.root / 'missing'])['status'], 'incomplete')

    def test_json_values_are_not_returned(self):
        path = self.root / 'settings.json'
        path.write_text('{"password": "example-private-value"}', encoding='utf-8')
        result = inspect_json(path)
        self.assertTrue(result['json_valid'])
        self.assertNotIn('example-private-value', json.dumps(result))

    def test_malformed_and_nonfinite_json_rejected(self):
        path = self.root / 'settings.json'
        for content in ['{broken}', '{"x":NaN}', '[]']:
            path.write_text(content, encoding='utf-8')
            self.assertFalse(inspect_json(path)['json_valid'])

    def test_collect_does_not_create_missing_directories(self):
        root, deploy = self.root / 'workspace', self.root / 'deploy'
        result = collect(root, deploy)
        self.assertTrue(result['issues'])
        self.assertFalse(root.exists() or deploy.exists())

    def test_cli_writes_manifest_and_returns_nonzero_for_missing_files(self):
        output = self.root / 'manifest.json'
        with contextlib.redirect_stdout(io.StringIO()):
            code = main(['--root', str(self.root), '--deploy', str(self.root / 'deploy'),
                         '--output', str(output)])
        self.assertEqual(code, 2)
        self.assertEqual(json.loads(output.read_text())['schema_version'], 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
