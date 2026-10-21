"""Build a test-only adapter around MASTER's actual, unchanged HttpFrame."""
from pathlib import Path
import os, shutil, subprocess

ROOT=Path(__file__).resolve().parents[1]
def build():
    folder=ROOT/'.build/tests';folder.mkdir(parents=True,exist_ok=True)
    source=ROOT/'checks/host/http_frame_probe.cpp'
    output=folder/('http_frame_probe.exe' if os.name=='nt' else 'http_frame_probe')
    if os.name=='nt':
        command=['cl.exe','/nologo','/std:c++17','/EHsc',str(source),'/Fe:'+str(output),'/Fo:'+str(folder/'http_frame_probe.obj')]
        if shutil.which('cl.exe'):
            subprocess.run(command,check=True,cwd=ROOT)
        else:
            vcvars=Path(os.environ.get('ProgramFiles(x86)',r'C:\Program Files (x86)'))/'Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
            if not vcvars.exists(): raise SystemExit('Run from a Visual Studio C++ x64 Native Tools prompt, or install MSVC 2022 Build Tools')
            script=folder/'build_http_frame_probe.cmd'
            script.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(command)+'\n',encoding='utf-8')
            subprocess.run(['cmd.exe','/d','/c',str(script)],check=True,cwd=ROOT)
    else:
        compiler=os.environ.get('CXX') or shutil.which('g++') or shutil.which('clang++')
        if not compiler: raise SystemExit('Install a C++17 compiler (g++ or clang++)')
        subprocess.run([compiler,'-std=c++17',str(source),'-o',str(output)],check=True,cwd=ROOT)
    print(output)
    return output

if __name__=='__main__': build()
