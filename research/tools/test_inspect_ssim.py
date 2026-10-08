"""Synthetic archive tests only; no third-party fixtures or script execution."""
import io
import stat
import unittest
import warnings
import zipfile
from inspect_ssim import inspect_bytes


def package(members):
    target = io.BytesIO()
    with warnings.catch_warnings():
        warnings.simplefilter('ignore', UserWarning)
        with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED) as archive:
            for name, content in members:
                archive.writestr(name, content)
    return target.getvalue()


XML = '<Simulation version="4.2"><World><Script>do_not_execute()</Script></World></Simulation>'


class InspectionTests(unittest.TestCase):
    def test_valid_assets_and_script_are_inert(self):
        result = inspect_bytes(package([('simulation.xml', XML), ('image.png', b'not decoded')]))
        self.assertEqual(result['xml_version'], '4.2')
        self.assertEqual(result['element_counts']['Script'], 1)
        self.assertFalse(result['runtime_import_tested'])
        self.assertEqual(len(result['members']), 2)

    def test_missing_xml(self):
        with self.assertRaisesRegex(ValueError, 'missing'):
            inspect_bytes(package([('other.xml', XML)]))

    def test_duplicate_xml(self):
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            inspect_bytes(package([('simulation.xml', XML)] * 2))

    def test_unsafe_names(self):
        for name in ('../escape', '/absolute', 'C:stream'):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, 'unsafe'):
                inspect_bytes(package([('simulation.xml', XML), (name, b'x')]))

    def test_backslash_bytes(self):
        raw = package([('simulation.xml', XML), ('dir/file', b'x')])
        raw = raw.replace(b'dir/file', b'dir' + bytes([92]) + b'file')
        with self.assertRaisesRegex(ValueError, 'unsafe'):
            inspect_bytes(raw)

    def test_symlink(self):
        link = zipfile.ZipInfo('link')
        link.create_system = 3
        link.external_attr = (stat.S_IFLNK | 0o777) << 16
        with self.assertRaisesRegex(ValueError, 'symlink'):
            inspect_bytes(package([('simulation.xml', XML), (link, b'target')]))

    def test_dtd(self):
        with self.assertRaisesRegex(ValueError, 'DTD'):
            inspect_bytes(package([('simulation.xml', '<!DOCTYPE Simulation><Simulation/>')]))

    def test_wrong_root(self):
        with self.assertRaisesRegex(ValueError, 'root'):
            inspect_bytes(package([('simulation.xml', '<Other/>')]))

    def test_depth_limit(self):
        with self.assertRaisesRegex(ValueError, 'depth'):
            inspect_bytes(package([('simulation.xml', '<Simulation>' + '<x>' * 129 + '</x>' * 129 + '</Simulation>')]))

    def test_truncated_zip(self):
        with self.assertRaises(zipfile.BadZipFile):
            inspect_bytes(package([('simulation.xml', XML)])[:20])


if __name__ == '__main__':
    unittest.main()
