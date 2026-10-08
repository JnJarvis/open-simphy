"""Read-only research inventory; not an Open Simphy importer. No extraction/execution."""
import argparse
from collections import Counter
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import stat
import xml.etree.ElementTree as ET
import zipfile

MAX_ARCHIVE = 64 * 1024 * 1024
MAX_TOTAL = 128 * 1024 * 1024
MAX_XML = 8 * 1024 * 1024


def inspect_bytes(data):
    if len(data) > MAX_ARCHIVE:
        raise ValueError("archive byte limit")
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        entries = archive.infolist()
        if len(entries) > 4096:
            raise ValueError("entry count limit")
        names = set()
        total = 0
        for entry in entries:
            name = entry.filename
            parts = PurePosixPath(name).parts
            if (not name or '\\' in name or ':' in name or name.startswith('/')
                    or '..' in parts or entry.orig_filename != name):
                raise ValueError("unsafe member name")
            if name in names:
                raise ValueError("duplicate member")
            names.add(name)
            if entry.flag_bits & 1:
                raise ValueError("encrypted member")
            if stat.S_ISLNK(entry.external_attr >> 16):
                raise ValueError("symlink member")
            if entry.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED):
                raise ValueError("unsupported compression")
            total += entry.file_size
            if total > MAX_TOTAL or entry.file_size > 1000 * max(1, entry.compress_size):
                raise ValueError("expanded size or ratio limit")
        if 'simulation.xml' not in names:
            raise ValueError("missing simulation.xml")
        if archive.getinfo('simulation.xml').file_size > MAX_XML:
            raise ValueError("XML byte limit")
        xml = bytearray()
        members = []
        for entry in entries:
            digest = hashlib.sha256()
            size = 0
            with archive.open(entry) as source:
                while chunk := source.read(65536):
                    size += len(chunk)
                    if size > entry.file_size:
                        raise ValueError("expanded member exceeds declared size")
                    digest.update(chunk)
                    if entry.filename == 'simulation.xml':
                        xml.extend(chunk)
            if size != entry.file_size:
                raise ValueError("member size mismatch")
            members.append(dict(name=entry.filename, bytes=size,
                                compressed_bytes=entry.compress_size,
                                compression=entry.compress_type, sha256=digest.hexdigest()))
    text = xml.decode('utf-8-sig')
    if '<!DOCTYPE' in text.upper() or '<!ENTITY' in text.upper():
        raise ValueError("DTD/entities forbidden")
    parser = ET.XMLPullParser(events=('start', 'end'))
    depth = 0
    count = 0
    tags = Counter()
    top = []
    root_tag = None
    version = None
    for offset in range(0, len(text), 4096):
        parser.feed(text[offset:offset + 4096])
        for event, element in parser.read_events():
            if event == 'start':
                depth += 1
                count += 1
                if depth > 128 or count > 200000:
                    raise ValueError("XML depth or element limit")
                tags[element.tag] += 1
                if depth == 1:
                    root_tag = element.tag
                    version = element.get('version')
                elif depth == 2:
                    top.append(element.tag)
            else:
                depth -= 1
                element.clear()
    parser.close()
    if root_tag != 'Simulation':
        raise ValueError("unexpected XML root")
    return dict(sha256=hashlib.sha256(data).hexdigest(), bytes=len(data),
                expanded_bytes=total, members=members, xml_version=version,
                xml_root=root_tag, top_level=top, element_counts=dict(sorted(tags.items())),
                result='container-and-XML-inspected', runtime_import_tested=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    results = []
    for path in sorted(args.directory.rglob('*.ssim')):
        try:
            with path.open('rb') as source:
                data = source.read(MAX_ARCHIVE + 1)
            record = inspect_bytes(data)
        except (ValueError, OSError, zipfile.BadZipFile, ET.ParseError, RuntimeError) as error:
            record = dict(result='rejected', reason=str(error), runtime_import_tested=False)
        record['source'] = path.relative_to(args.directory).as_posix()
        results.append(record)
    if not results:
        raise SystemExit('No SSIM archives found')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(archives=len(results), results=dict(Counter(r['result'] for r in results)))))


if __name__ == '__main__':
    main()
