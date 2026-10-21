"""Compile all five matching ESP32 sketches without connecting hardware."""
from pathlib import Path
import argparse, os, shutil, subprocess

ROOT=Path(__file__).resolve().parents[1]
BOARDS=['01_MASTER/TubaTech_Integrated_Master','02_SLAVE1/TubaTech_Integrated_Slave1',
        '03_SLAVE2/TubaTech_Integrated_Slave2','04_SLAVE3/TubaTech_Integrated_Slave3','05_SLAVE4/TubaTech_Integrated_Slave4']
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cli');args=parser.parse_args()
    cli=args.cli or shutil.which('arduino-cli')
    if not cli and os.name=='nt':
        candidate=Path(os.environ['LOCALAPPDATA'])/'Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe'
        if candidate.exists(): cli=str(candidate)
    if not cli: raise SystemExit('Install Arduino CLI or provide --cli PATH')
    if not (ROOT/'firmware'/BOARDS[0]/'wifi_config.h').exists():
        raise SystemExit('Run python tools/configure_wifi.py first (or --generate for software-only compilation)')
    logs=ROOT/'checks/logs';logs.mkdir(parents=True,exist_ok=True)
    for board in BOARDS:
        name=Path(board).name;log=logs/(name+'-compile.log')
        output=ROOT/'deliverables/firmware'/name;output.mkdir(parents=True,exist_ok=True)
        cmd=[cli,'compile','--fqbn','esp32:esp32:esp32','--build-path',str(ROOT/'.build/arduino'/name),
             '--output-dir',str(output),str(ROOT/'firmware'/board)]
        with log.open('w',encoding='utf-8') as stream:
            result=subprocess.run(cmd,stdout=stream,stderr=subprocess.STDOUT)
        if result.returncode:
            print(log.read_text(encoding='utf-8')[-6000:]);raise SystemExit(result.returncode)
        print(f'PASS: {name} ESP32 compile',flush=True)
