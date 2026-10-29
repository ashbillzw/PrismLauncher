#!/usr/bin/env python3
"""Finalize mirror URLs completely offline using the frozen upstream metadata.
Serve only publish/. This script does not download, upload, or change game files.
"""
import argparse
import hashlib
import json
from pathlib import Path
from urllib.parse import urlsplit
from prepare_mirror import Builder, encoded, sha


class OfflineBuilder(Builder):
    def fetch(self, url, target, expected=None, algorithm='sha1'):
        if not target.is_file():
            raise FileNotFoundError(f'Missing frozen metadata; no network fallback: {target}')
        data = target.read_bytes()
        if expected and sha(data, algorithm) != expected:
            raise ValueError(f'Frozen metadata checksum mismatch: {target}')
        return data


def configure(bundle, base_url):
    parsed = urlsplit(base_url)
    if parsed.scheme != 'https' or not parsed.hostname or parsed.username or parsed.query or parsed.fragment or parsed.hostname.endswith('.invalid'):
        raise ValueError('Supply the final public HTTPS root URL mapped to publish/, without credentials, query, or fragment.')
    base_url = base_url.rstrip('/') + '/'
    manifest_path = bundle / 'download-manifest.json'
    before = json.loads(manifest_path.read_text(encoding='utf-8'))
    marker_path = bundle / 'download-complete.json'
    verified = json.loads(marker_path.read_text(encoding='utf-8')) if marker_path.exists() else None
    args = argparse.Namespace(output=bundle, base_url=base_url, minecraft=before['minecraft'], forge=before['forge'], proxy=None, download_files=False)
    OfflineBuilder(args).build()
    after = json.loads(manifest_path.read_text(encoding='utf-8'))
    if before['files'] != after['files'] or before['components'] != after['components']:
        raise ValueError('Frozen metadata changed the file closure; download certification not preserved')
    if verified:
        verified['base_url'] = base_url
        verified['manifest_sha256'] = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
        marker_path.write_bytes(encoded(verified))
    settings = 'NiijimaPackUrl='+base_url+'packs/pack.zip\nMetaURLOverride='+base_url+'meta/v1/\nResourceURLOverride='+base_url+'assets/\n'
    (bundle / 'launcher-settings.txt').write_text(settings, encoding='utf-8')
    (bundle / 'deployment-status.json').write_bytes(encoded({'metadata_configured':True,'payload_complete':bool(verified),'base_url':base_url,'pack_included':False}))
    print('Frozen metadata regenerated offline; existing resource files were preserved.')
    print('\nFinal launcher settings (pack.zip is supplied separately):\n'+settings)
    return after


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle', type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument('--base-url', required=True)
    args = parser.parse_args()
    configure(args.bundle.resolve(), args.base_url)
