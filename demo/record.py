#!/usr/bin/env python3
"""Record the actual OpenTTD framebuffer using the ICU4X Rust backend."""
import os, pathlib, subprocess
root = pathlib.Path(__file__).resolve().parent.parent
frames = 11
output = root/'demo/output/word'
dest = output/'icu4x'
dest.mkdir(parents=True, exist_ok=True)
for previous in dest.glob('frame-*'):
    if previous.suffix in {'.ppm', '.png'}: previous.unlink()
(dest/'trace.tsv').unlink(missing_ok=True)
config = dest/'openttd.cfg'
config.write_text('[misc]\nlanguage = english.lng\ngui_scale = 200\n[gui]\nautosave_on_exit = false\n[network]\nparticipate_survey = no\n')
(dest/'private.cfg').write_text('[network]\nparticipate_survey = no\n')
env = dict(os.environ, CALIF_WORD_BACKEND='icu4x', CALIF_RECORD_DIR=str(dest))
args = [str(root/'build/openttd'), '-X', '-x', '-c', str(config), '-v', f'null:render,ticks={frames * 20 + 10}', '-b', '32bpp-simple', '-s', 'null', '-m', 'null', '-r', '800x480', '-I', 'OpenGFX']
with (dest/'run.log').open('w') as log:
    subprocess.run(args, cwd=root/'build', env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=120)
assert len(list(dest.glob('frame-*.ppm'))) == frames, 'Missing frames; see run.log'
trace = [line.split('\t') for line in (dest/'trace.tsv').read_text().splitlines()]
assert len(trace) == frames, 'Unexpected recorded state count'
assert [int(row[0]) for row in trace] == list(range(frames)), 'Missing recorded steps'
assert all(int(row[4]) > 0 for row in trace), 'Rust backend was not exercised'
(output/'verification.txt').write_text(f'PASS: recorded all {frames} ICU4X editing steps.\nPASS: every recorded step has positive Rust segmentation calls.\n')
print((output/'verification.txt').read_text())

for ppm in sorted(dest.glob('frame-*.ppm')):
    subprocess.run(['sips', '-s', 'format', 'png', str(ppm), '--out', str(ppm.with_suffix('.png'))], check=True, stdout=subprocess.DEVNULL)
print('Captured frames:', dest)
