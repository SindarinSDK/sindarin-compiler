#!/usr/bin/env python3
"""Exercise Windows text helpers on any host; this is not Windows ABI evidence."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    rustc = shutil.which("rustc")
    if not rustc:
        raise RuntimeError("rustc is required")
    helper = (ROOT / "templates/rust/partials/windows_text_output_support.hbs").read_text()
    source = helper + r'''
fn main() {
    print!("{}", "start\r\n");
    println!("{}:{:.5}", 42, 1.25);
    println!();
    __sn_write_stdout_bytes(&[0xff, 0, b'\n', b'X']);
    __sn_write_stderr_bytes(b"error\n");
}
'''
    with tempfile.TemporaryDirectory(prefix="sn-windows-text-helper-") as directory:
        path = Path(directory)
        src, exe = path / "helper.rs", path / "helper.exe"
        src.write_text(source)
        # Only the platform-independent byte adapter is compiled here. Forcing
        # its cfg on Unix cannot validate the Windows CRT, linker or OS APIs.
        subprocess.run([rustc, "--edition=2021", "--cfg", "windows",
                        "-Aexplicit_builtin_cfgs_in_flags", str(src), "-o", str(exe)],
                       check=True, capture_output=True, timeout=60)
        result = subprocess.run([str(exe)], capture_output=True, timeout=20)
        expected = b"start\r\r\n42:1.25000\r\n\r\n\xff\x00\r\nX"
        if result.returncode != 0 or result.stdout != expected or result.stderr != b"error\r\n":
            raise AssertionError((result.returncode, result.stdout.hex(), result.stderr.hex()))
    print("PASS Windows helper logic: scalar formatting, LF/CRLF, raw bytes, stderr")


if __name__ == "__main__":
    main()
