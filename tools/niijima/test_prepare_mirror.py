import argparse
import contextlib
import copy
import hashlib
import io
import json
import tempfile
import unittest
from pathlib import Path
from prepare_mirror import Builder, META, checked_target


class MirrorTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.args = argparse.Namespace(output=self.root, base_url='https://mirror.example/niijima/',
                                       minecraft='1.20.1', forge='47.4.6', proxy=None, download_files=False)

    def test_closure_hashes_and_originals(self):
        source = {
            ('net.minecraft', '1.20.1'): {
                'formatVersion': 1, 'uid': 'net.minecraft', 'version': '1.20.1', 'name': 'Minecraft',
                'requires': [{'uid': 'org.lwjgl3', 'suggests': '3.3.1'}],
                'mainJar': {'downloads': {'artifact': {'url': 'https://example.org/client.jar', 'sha1': 'a'*40, 'size': 10}}}},
            ('org.lwjgl3', '3.3.1'): {
                'formatVersion': 1, 'uid': 'org.lwjgl3', 'version': '3.3.1', 'name': 'LWJGL',
                'libraries': [{'downloads': {'classifiers': {'natives-windows': {
                    'url': 'https://example.org/natives.jar', 'sha1': 'b'*40, 'size': 20}}}}]},
            ('net.minecraftforge', '47.4.6'): {
                'formatVersion': 1, 'uid': 'net.minecraftforge', 'version': '47.4.6', 'name': 'Forge',
                'requires': [{'uid': 'net.minecraft', 'equals': '1.20.1'}],
                'mavenFiles': [{'downloads': {'artifact': {'url': 'https://example.org/forge.jar', 'sha1': 'c'*40, 'size': 30}}}]}}
        before = copy.deepcopy(source)
        builder = Builder(self.args)
        def get_json(url):
            uid, version = url.removeprefix(META).removesuffix('.json').split('/')
            return copy.deepcopy(source[(uid, version)])
        builder.get_json = get_json
        with contextlib.redirect_stdout(io.StringIO()):
            builder.build()
        self.assertEqual(source, before)
        manifest = json.loads((self.root/'download-manifest.json').read_text())
        self.assertEqual(manifest['file_count'], 3)
        self.assertEqual(manifest['total_bytes'], 60)
        self.assertEqual(len(manifest['components']), 3)
        meta = self.root/'publish/meta/v1'
        root_index = json.loads((meta/'index.json').read_text())
        for package in root_index['packages']:
            data = (meta/package['uid']/'index.json').read_bytes()
            self.assertEqual(hashlib.sha256(data).hexdigest(), package['sha256'])
            for version in json.loads(data)['versions']:
                payload = (meta/package['uid']/(version['version']+'.json')).read_bytes()
                self.assertEqual(hashlib.sha256(payload).hexdigest(), version['sha256'])
                self.assertNotIn(b'https://example.org', payload)
                self.assertIn(b'https://mirror.example/niijima/files/', payload)
        # Manifest-only runs must not download binary artifacts.
        self.assertFalse((self.root/'publish/files').exists())

    def test_refuse_unknown_and_conflicting_downloads(self):
        builder = Builder(self.args)
        with self.assertRaises(ValueError):
            builder.rewrite({'url': 'https://example.org/unhashed.jar'})
        builder.register('https://example.org/a.jar', 'a'*40, 1)
        with self.assertRaises(ValueError):
            builder.register('https://example.org/a.jar', 'b'*40, 2)

    def test_output_must_stay_inside_publish(self):
        builder = Builder(self.args)
        with self.assertRaises(ValueError):
            checked_target(builder.publish, '../outside')
        with self.assertRaises(ValueError):
            builder.register('https://example.org/%2e%2e/outside', 'a'*40, 1)


if __name__ == '__main__':
    unittest.main()
