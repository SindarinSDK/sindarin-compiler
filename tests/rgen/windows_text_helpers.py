#!/usr/bin/env python3
"""Exercise Windows text helpers on any host; this is not Windows ABI evidence."""
from pathlib import Path
import shutil
import re
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    rustc = shutil.which("rustc")
    if not rustc:
        raise RuntimeError("rustc is required")
    with tempfile.TemporaryDirectory(prefix="sn-windows-text-helper-") as directory:
        path = Path(directory)
        src, exe = path / "helper.rs", path / "helper.exe"
        compiler = ROOT / ("bin/sn.exe" if os.name == "nt" else "bin/sn")
        fixture = ROOT / "tests/rgen/windows_output_helper_hygiene.sn"
        subprocess.run([str(compiler), str(fixture), "--emit-rust", "-O0",
                        "--no-install", "-o", str(src)],
                       check=True, capture_output=True, timeout=60)
        emitted = src.read_text()
        # OS argv adapters require the actual target's standard library. The
        # helper simulation exercises output only; real argv runs stay in CI.
        begin = emitted.index("#[cfg(unix)]\nfn __sn_args()")
        end = emitted.index("impl std::fmt::Debug for SnString", begin)
        emitted = emitted[:begin] + emitted[end:]
        error_helper = re.search(r"fn (\w+)\(message: &'static str\) -> !", emitted)
        if not error_helper:
            raise AssertionError("missing generated checked error helper")
        # A forced cfg cannot supply the Windows CRT. Substitute only its
        # transport entry with the platform-independent adapter under test;
        # actual CRT buffering, imports and stream ordering remain hosted gates.
        adapter = re.search(r"fn (\w+)<W: std::io::Write>", emitted)
        if not adapter:
            raise AssertionError("missing generated text adapter")
        stdio = re.compile(r"fn (\w+)\(bytes: &\[u8\], stderr: bool\) \{.*?\n\}\n", re.S)
        def simulated_stdio(match):
            name = match.group(1)
            writer = adapter.group(1)
            return (f"fn {name}(bytes: &[u8], stderr: bool) {{\n"
                    f"    if stderr {{ {writer}(&mut std::io::stderr().lock(), bytes); }}\n"
                    f"    else {{ {writer}(&mut std::io::stdout().lock(), bytes); }}\n"
                    "}\n")
        emitted, replacements = stdio.subn(simulated_stdio, emitted)
        if replacements != 1:
            raise AssertionError("missing or ambiguous CRT transport entry")
        # The original main now flushes its C stdout at fallthrough. This forced
        # cfg simulation has no Windows CRT; the substituted byte adapter above
        # already flushes its writer. Actual main/CRT lifecycle is checked by
        # the hosted main-return gate, so remove exactly that main footer here.
        footer = re.compile(
            r"(?m)^    unsafe \{\n"
            r"        #\[cfg\(windows\)\]\n"
            r"        let stream = crate::\w+\(1\);\n"
            r"        #\[cfg\(not\(windows\)\)\]\n"
            r"        let stream = crate::\w+;\n"
            r"        crate::\w+\(stream\);\n"
            r"    \}\n")
        emitted, replacements = footer.subn("", emitted)
        if replacements != 1:
            raise AssertionError("missing or ambiguous main CRT flush footer")
        source = emitted.replace("fn main()", "fn __sn_original_main()", 1) + r'''
fn main() {
    __sn_original_main();
    print!("{}", "start\r\n");
    println!("{}:{:.5}", 42, 1.25);
    println!();
    __sn_print_string(&SnString::from_slice(&[0xff, 0, b'\n', b'X']));
    ERROR_HELPER("error");
}
'''
        source = source.replace("ERROR_HELPER", error_helper.group(1))
        src.write_text(source)
        # Only the platform-independent byte adapter is compiled here. Forcing
        # its cfg on Unix cannot validate the Windows CRT, linker or OS APIs.
        subprocess.run([rustc, "--edition=2021", "--cfg", "windows",
                        "-Aexplicit_builtin_cfgs_in_flags", str(src), "-o", str(exe)],
                       check=True, capture_output=True, timeout=60)
        result = subprocess.run([str(exe)], capture_output=True, timeout=20)
        expected = b"sum:21\r\nstart\r\r\n42:1.25000\r\n\r\n\xff\x00\r\nX"
        if result.returncode != 1 or result.stdout != expected or result.stderr != b"error\r\n":
            raise AssertionError((result.returncode, result.stdout.hex(), result.stderr.hex()))
    print("PASS Windows helper logic: scalar formatting, LF/CRLF, raw bytes, stderr")


if __name__ == "__main__":
    main()
