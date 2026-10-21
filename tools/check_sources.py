"""Check source provenance invariants, fixed radio setup, and generated outputs."""
from pathlib import Path
import hashlib, json, re, subprocess, sys
ROOT=Path(__file__).resolve().parents[1]
def digest(value): return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
if __name__=='__main__':
    subprocess.run([sys.executable,str(ROOT/'tools/generate_definitions.py'),'--check'],check=True)
    data=json.loads((ROOT/'definitions/control.json').read_text())
    baseline=json.loads((ROOT/'reference/SOURCE_FINGERPRINTS.json').read_text())
    assert digest(data['functions'])==baseline['sequence_semantics_sha256'],'Sequence order, identity, duration or notes changed from reviewed source'
    assert digest(data['components'])==baseline['wiring_semantics_sha256'],'Mapping changed from reviewed source'
    master=ROOT/'firmware/01_MASTER/TubaTech_Integrated_Master'
    assert 'const uint8_t CHANNEL = 6;' in (master/'TubaTech_Integrated_Master.ino').read_text()
    for folder in (ROOT/'firmware').glob('*/*'):
        if folder.is_dir() and 'Master' not in folder.name:
            assert 'esp_wifi_set_channel(6,' in (folder/'receiver_runtime.h').read_text()
    from configure_wifi import render_config
    for invalid in ['', 'short', 'x'*64, '\n'*8]:
        try: render_config(invalid)
        except ValueError: pass
        else: raise AssertionError('Invalid WPA password accepted')
    print('PASS: source sequence/wiring fingerprints, channel 6 and local Wi-Fi configuration validation')
