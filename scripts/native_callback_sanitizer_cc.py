#!/usr/bin/env python3
"""Instrument native C objects while Rust supplies the final ASAN runtime."""
import os
import shutil
import sys

compiler = os.environ.get('SN_NATIVE_CALLBACK_SAN_CC') or shutil.which('clang')
if not compiler:
    raise SystemExit('Native callback sanitizer checks require an existing clang compiler')
arguments = sys.argv[1:]
if '-c' not in arguments:
    arguments = [argument for argument in arguments if not argument.startswith('-fsanitize=')]
os.execv(compiler, [compiler, *arguments])
