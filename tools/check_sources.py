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
    sketch=(master/'TubaTech_Integrated_Master.ino').read_text()
    assert 'const uint8_t CHANNEL = 6;' in sketch
    assert 'peer.channel = CHANNEL;' in sketch
    wifi=(master/'local_wifi.h').read_text()
    assert 'WiFi.softAP("TubaTech_Master",apPassword.c_str(),CHANNEL,false,2)' in wifi
    def macs_in(block):
        return [':'.join(re.findall(r'0x([0-9A-Fa-f]{2})', values)).upper()
                for values in re.findall(r'\{((?:0x[0-9A-Fa-f]{2}\s*,?\s*){6})\}',block)]
    expected=[b['mac'] for b in data['boards']]
    nodes=re.search(r'Node nodes\[\] = \{(.*?)\n\};',sketch,re.S).group(1)
    assert macs_in(nodes)==expected[1:]+expected[:1],'MASTER peer/local MACs differ from definitions'
    config=(master/'node_config.h').read_text()
    positions=re.search(r'inline Position positions\[\]=\{(.*?)\n\};',config,re.S).group(1)
    assert macs_in(positions)==expected[1:],'Configured slave MACs differ from definitions'
    master_mac=re.search(r'MASTER_MAC\[6\]=\{([^}]+)\}',config).group(1)
    assert ':'.join(re.findall(r'0x([0-9A-Fa-f]{2})',master_mac)).upper()==expected[0]
    bank=(master/'relay_bank.h').read_text()
    compact=re.sub(r'\s+','',bank)
    assert 'digitalWrite(demo::outputPins[p.channel],p.value?LOW:HIGH);' in compact
    assert 'digitalWrite(demo::outputPins[ch],HIGH);pinMode(demo::outputPins[ch],OUTPUT);' in compact
    assert [f['id'] for f in data['functions'] if not f['executable']]==['W_E','W_I','W_O','W_DAILY']
    for folder in (ROOT/'firmware').glob('*/*'):
        if folder.is_dir() and 'Master' not in folder.name:
            receiver=(folder/'receiver_runtime.h').read_text()
            assert 'esp_wifi_set_channel(6,' in receiver and 'peer.channel=6;' in receiver
            assert (folder/'relay_bank.h').read_text()==bank,'Relay polarity/boot driver copies differ'
    from configure_wifi import render_config
    for invalid in ['', 'short', 'x'*64, '\n'*8]:
        try: render_config(invalid)
        except ValueError: pass
        else: raise AssertionError('Invalid WPA password accepted')
    print('PASS: five runtime MACs, channel 6 on AP/peers/receivers, active-LOW and boot-OFF drivers, sequence/wiring fingerprints')
    print('UNAVAILABLE: W_E (PC1 sensors), W_I (PC2 sensors), W_O (undefined J-to-K route), W_DAILY (unspecified orchestration)')
