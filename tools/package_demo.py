"""Create a LOCAL-ONLY demo bundle; includes the private MASTER credential."""
from pathlib import Path
import hashlib, json, re, shutil, subprocess, zipfile

ROOT=Path(__file__).resolve().parents[1]
SKETCHES=['01_MASTER/TubaTech_Integrated_Master','02_SLAVE1/TubaTech_Integrated_Slave1',
          '03_SLAVE2/TubaTech_Integrated_Slave2','04_SLAVE3/TubaTech_Integrated_Slave3','05_SLAVE4/TubaTech_Integrated_Slave4']
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()

if __name__=='__main__':
    git=['git','-c','safe.directory='+str(ROOT).replace('\\','/')]
    commit=subprocess.check_output([*git,'rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    if subprocess.check_output([*git,'status','--porcelain'],cwd=ROOT,text=True).strip():
        raise SystemExit('Commit the verified source before packaging')
    version=re.search(r'^version:\s*([^+\s]+)\+(\d+)',(ROOT/'demo_app/pubspec.yaml').read_text(),re.M).group(1)
    control=json.loads((ROOT/'definitions/control.json').read_text())
    output=ROOT/'deliverables'/f'Demo-TubaTech-{version}-LOCAL'
    output.mkdir(parents=True,exist_ok=True)
    apk=ROOT/'demo_app/build/app/outputs/flutter-apk/app-release.apk'
    apk_name=f'Demo-TubaTech-{version}.apk';shutil.copy(apk,output/apk_name)
    config=ROOT/'firmware'/SKETCHES[0]/'wifi_config.h'
    value=re.search(r'^#define\s+TUBATECH_AP_PASSWORD\s+(.+)$',config.read_text(),re.M).group(1)
    password=json.loads(value)
    (output/'PRIVATE-WIFI.txt').write_text('LOCAL ONLY — DO NOT PUBLISH\nSSID: TubaTech_Master\nPassword: '+password+
        '\nChannel: 6\nMASTER API: http://192.168.4.1\nSource commit: '+commit+'\n',encoding='utf-8')
    images=[]
    for board,sketch in zip(control['boards'],SKETCHES):
        role=board['name'].replace(' ','')
        name=role+'_'+board['mac'].replace(':','')+'.merged.bin'
        source=ROOT/'deliverables/firmware'/Path(sketch).name/(Path(sketch).name+'.ino.merged.bin')
        assert source.exists(),f'Missing compiled image for {role}'
        shutil.copy(source,output/name)
        images.append(dict(role=board['name'],factory_station_mac=board['mac'],file=name,flash_offset='0x0',
            sha256=digest(output/name),arduino_source='firmware/'+sketch+'/'+Path(sketch).name+'.ino'))
    manifest=dict(source_commit=commit,version=version,radio_channel=6,active_low=True,assigned_outputs=66,
        apk=dict(file=apk_name,sha256=digest(output/apk_name)),firmware=images,
        credential_file='PRIVATE-WIFI.txt',publication='LOCAL ONLY: MASTER image embeds the private credential',
        physical_tests='NOT TESTED')
    (output/'LOCAL-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
    guide=(ROOT/'docs/DEMO_UPLOAD.md').read_text(encoding='utf-8')
    guide=guide.replace('../firmware/','https://github.com/Memozzz/Demo-TubaTech/blob/'+commit+'/firmware/')
    (output/'UPLOAD.md').write_text(guide,encoding='utf-8')
    files=sorted(p for p in output.iterdir() if p.is_file() and p.name!='SHA256SUMS.txt')
    (output/'SHA256SUMS.txt').write_text(''.join(digest(p)+'  '+p.name+'\n' for p in files))
    archive=output.parent/(output.name+'.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as zipped:
        for path in sorted(output.iterdir()): zipped.write(path,path.name)
    print('LOCAL ONLY bundle:',archive)
    print('Private Wi-Fi file:',output/'PRIVATE-WIFI.txt')
    print('PASS: five role/MAC-labelled merged images, APK and local manifest; do not upload this bundle')
