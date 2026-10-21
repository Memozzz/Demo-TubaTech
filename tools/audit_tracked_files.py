"""Fail if staged/tracked publication files include local secrets or keys."""
from pathlib import Path
import re, subprocess
ROOT=Path(__file__).resolve().parents[1]
if __name__=='__main__':
    files=subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0')
    violations=[]
    forbidden_names={'wifi_config.h','local.properties','key.properties','.env'}
    patterns=[rb'gh[pousr]_[A-Za-z0-9]{20,}',rb'github_pat_[A-Za-z0-9_]{20,}',
        rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----',rb'AKIA[0-9A-Z]{16}',
        rb'#define\s+TUBATECH_AP_PASSWORD\s+"[^"\r\n]+"',
        rb'apPassword\s*=\s*"[^"\r\n]+"']
    for name in filter(None,files):
        path=ROOT/name
        if path.name in forbidden_names or path.suffix.lower() in {'.jks','.keystore','.pem','.p12'} or name.startswith(('deliverables/','.build/')):
            violations.append(name+': forbidden local file');continue
        content=path.read_bytes()
        if any(re.search(pattern,content) for pattern in patterns): violations.append(name+': credential pattern')
    if violations: raise SystemExit('\n'.join(violations))
    print(f'PASS: {len(list(filter(None,files)))} tracked files contain no detected credentials, local Wi-Fi configuration or signing keys')
