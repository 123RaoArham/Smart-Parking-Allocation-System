"""Simple regression checks for the final C11 CLI. Run: python tests/test_cli.py"""
import pathlib, subprocess, tempfile
root = pathlib.Path(__file__).resolve().parents[1]
source = root / 'Phase_II' / 'src' / 'Smart_Parking.c'
with tempfile.TemporaryDirectory() as tmp:
    exe = pathlib.Path(tmp) / 'parking'
    subprocess.run(['gcc','-std=c11','-O2',str(source),'-o',str(exe)],check=True)
    def run(inputs):
        return subprocess.run([str(exe)],input=inputs,text=True,capture_output=True,check=True).stdout
    baseline = run('3\n6\n5\n0\n')
    for fragment in ('D1 [540,660) ACCEPTED -> S1','D4 [570,630) ACCEPTED -> S4', 'D2 [600,720) ACCEPTED -> S2', 'D3 [660,780) ACCEPTED -> S1', 'VALIDATION PASSED', 'Heap vs array Dijkstra distances: MATCH'):
        assert fragment in baseline, fragment
    assert 'Invalid time or type' in run('2\nD6 720 600 REGULAR A\n0\n')
    assert 'Driver ID already exists' in run('2\nD1 800 900 REGULAR A\n0\n')
    full = run('2\nD6 600 630 REGULAR A\n3\n6\n0\n')
    assert 'D6 [600,630) REJECTED' in full and 'VALIDATION PASSED' in full
    changed = run('2\nD5 540 600 REGULAR A\n3\n4\nD5\n3\n6\n0\n')
    assert 'D5 [540,600) ACCEPTED -> S2' in changed
    assert 'Reservation removed' in changed and 'VALIDATION PASSED' in changed
    print('PASS: baseline, Dijkstra comparison, invalid, duplicate, capacity, add/cancel')
