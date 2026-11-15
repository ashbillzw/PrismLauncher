"""Stamp and register a Niijima distribution ZIP. Run on the Windows packaging host."""
import argparse
from contextlib import contextmanager
import json
import msvcrt
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from zipfile import ZipFile

MARKER = "overrides/niijima-pack.json"
INDEX = "modrinth.index.json"
REPO_INDEX = Path(__file__).resolve().parent / "deploy/packs/latest.json"


def read_json(data):
    return json.loads(data.decode("utf-8-sig"))


def encode(data):
    return (json.dumps(data, ensure_ascii=False, indent=2) + "\n").encode()


@contextmanager
def registration_lock(path):
    # The OS releases the lock if the process exits during registration.
    with open(path, "a+b", opener=lambda p, flags: os.open(p, flags | os.O_TEMPORARY)) as handle:
        handle.seek(0)
        msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
        yield


def payload_entries(archive):
    return sorted((e.filename, e.CRC, e.file_size) for e in archive.infolist() if e.filename != MARKER and not e.is_dir())


def channel_table(table):
    if "channels" not in table:
        return {"cdks": {}, "channels": {"master": table}}
    return table


def register(source, packs, registry=REPO_INDEX, channel="master", pack_id=None):
    with ZipFile(source) as archive:
        version = read_json(archive.read(INDEX))["versionId"]
        metadata = read_json(archive.read(MARKER))
        payload = payload_entries(archive)
        if archive.namelist().count(MARKER) != 1 or metadata.get("token") != 114514:
            raise ValueError("Missing or invalid Niijima pack marker")
    if not channel or not all(c.isascii() and (c.isalnum() or c in "-_") for c in channel):
        raise ValueError("Invalid channel name")
    filename = ("" if channel == "master" else f"{channel}-") + f"新岛铁道 v{version}.zip"
    if not isinstance(version, str) or not version.strip() or any(c in '<>:"/\\|?*' or ord(c) < 32 for c in version):
        raise ValueError("Invalid versionId for a ZIP filename")
    target = packs / filename
    if target.resolve() == source.resolve():
        raise ValueError("Source ZIP must be separate from distribution ZIP")
    paths = list(dict.fromkeys([registry.resolve(), (packs / "latest.json").resolve()]))
    # Lock the canonical index across registrations, even for different packs directories.
    with registration_lock(registry.with_suffix(".registration.lock")), tempfile.TemporaryDirectory(dir=packs) as work:
        tables = [channel_table(read_json(p.read_bytes()) if p.exists() else {}) for p in paths]
        channels = {}
        for table in tables:
            for name, releases in table["channels"].items():
                merged = channels.setdefault(name, {})
                for key, entry in releases.items():
                    if str(int(key)) != key or not 0 < int(key) < 2**64 or merged.setdefault(key, entry) != entry:
                        raise ValueError(f"Invalid or conflicting pack-id {name}/{key}")
        entries = channels.setdefault(channel, {})
        # The repository owns the CDK mapping; do not resurrect deleted codes from distribution copies.
        record = {"version": version, "file": filename}
        matches = [k for k, v in entries.items() if v["version"] == version or v["file"] == filename]
        if matches and (len(matches) != 1 or entries[matches[0]] != record):
            raise ValueError("Conflicting registration for this version or filename")
        key = matches[0] if matches else str(max(map(int, entries), default=0) + 1)
        if pack_id is not None:
            explicit = str(pack_id)
            if (matches and key != explicit) or (not matches and pack_id <= max(map(int, entries), default=0)):
                raise ValueError("Requested pack-id conflicts with the channel history")
            key = explicit
        if not 0 < int(key) < 2**64 or metadata.get("pack-id", key) != key or metadata.get("channel", channel) != channel:
            raise ValueError("Invalid or mismatched pack-id")
        metadata["pack-id"] = key
        if channel != "master":
            metadata["channel"] = channel
        work = Path(work)
        if target.exists():
            with ZipFile(target) as archive:
                if read_json(archive.read(MARKER)) != metadata or payload_entries(archive) != payload:
                    raise ValueError("Existing distribution ZIP does not match the registration")
        else:
            stage = work / filename
            shutil.copyfile(source, stage)
            marker = work / MARKER
            marker.parent.mkdir()
            marker.write_bytes(encode(metadata))
            subprocess.run(["7z", "a", "-up1q1r2x2y2z2w2", str(stage), MARKER], cwd=work, check=True)
            with ZipFile(stage) as archive:
                if archive.namelist().count(MARKER) != 1 or read_json(archive.read(MARKER)) != metadata:
                    raise ValueError("Failed to stamp pack-id")
            stage.rename(target)
        for path, table in zip(paths, tables):
            for name in channels:
                table["channels"].setdefault(name, {})
            table["channels"][channel][key] = record
            index = {"cdks": tables[0]["cdks"], "channels": table["channels"]}
            if not path.exists() or read_json(path.read_bytes()) != index:
                with tempfile.TemporaryDirectory(dir=path.parent) as staging:
                    tmp = Path(staging) / "latest.json"
                    tmp.write_bytes(encode(index))
                    os.replace(tmp, path)
    print(f"{channel} pack-id {key}: {target}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zip", type=Path)
    parser.add_argument("packs", type=Path)
    parser.add_argument("--channel", default="master")
    parser.add_argument("--pack-id", type=int, help="Align a release with a numbered version on another channel")
    args = parser.parse_args()
    register(args.zip.resolve(), args.packs.resolve(), channel=args.channel, pack_id=args.pack_id)

