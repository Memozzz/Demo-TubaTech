"""Build and run the actual five sketch host tests with GCC/Clang C++17."""
from pathlib import Path
import argparse, shutil, subprocess
ROOT=Path(__file__).resolve().parents[1]
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cxx');args=parser.parse_args()
    compiler=args.cxx or shutil.which('g++') or shutil.which('clang++')
    if not compiler: raise SystemExit('Install g++/clang++, or on Windows use tools/run_host_checks.cmd with MSVC')
    out=ROOT/'.build/tests';out.mkdir(parents=True,exist_ok=True)
    for node in range(5):
        name='integrated_master' if not node else f'integrated_slave{node}'
        source='integrated_master_tests.cpp' if not node else 'integrated_receiver_tests.cpp'
        cmd=[compiler,'-std=c++17','-Wall','-Wextra','-Wno-misleading-indentation',
            '-Ichecks/host/stubs',f'-DTEST_NODE={node}',str(ROOT/'checks/host'/source),'-o',str(out/name)]
        subprocess.run(cmd,cwd=ROOT,check=True)
        subprocess.run([str(out/name)],cwd=ROOT,check=True)
