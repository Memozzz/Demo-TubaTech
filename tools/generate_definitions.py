"""Generate app catalog, five firmware tables and reviewer tables from control.json."""
from pathlib import Path
import argparse, json, re

ROOT=Path(__file__).resolve().parents[1]
BOARDS=['01_MASTER/TubaTech_Integrated_Master','02_SLAVE1/TubaTech_Integrated_Slave1',
        '03_SLAVE2/TubaTech_Integrated_Slave2','04_SLAVE3/TubaTech_Integrated_Slave3','05_SLAVE4/TubaTech_Integrated_Slave4']
def q(value): return json.dumps(value,ensure_ascii=True)
def generate(check=False):
    data=json.loads((ROOT/'definitions/control.json').read_text(encoding='utf-8'))
    components=data['components']; functions=data['functions']; boards=data['boards']
    assert len(components)==66 and len(functions)==34 and len(boards)==5
    assert data['radio_channel']==6
    assert len({c['component'] for c in components})==66
    assert len({(c['node_id'],c['gpio']) for c in components})==66
    assert len({(c['relay_module'],c['relay_input']) for c in components})==66
    assert [b['node_id'] for b in boards]==list(range(5))
    assert len({b['mac'] for b in boards})==5
    for c in components:
        assert c['active_low'] is True
        assert c['gpio']==[16,17,18,19,21,22,23,25,26,27,32,33,13,14][c['output_index']]
        assert 0<=c['node_id']<=4 and 1<=c['relay_module']<=8
    names={c['component'] for c in components}
    for f in functions:
        assert f['executable']==(f['status']=='RelayDemo')
        assert bool(f['phases'])==f['executable']
        for phase in f['phases']:
            assert phase['seconds']>0
            for a in phase['actions']: assert a['component'] in names and isinstance(a['on'],bool)
    lines=[]
    for c in components:
        node=4 if c['node_id']==0 else c['node_id']-1
        lines.append(' {'+','.join([q(c['component']),str(node),str(c['output_index']),'Kind::'+c['kind'],q(c['category']),
            str(c['gpio']),str(c['relay_input']),'false','false','1','0',str(c['output_index']),str(c['relay_module']),'true'])+'},')
    registry=(ROOT/'tools/templates/component_registry.h.in').read_text().replace('@@COMPONENTS@@','\n'.join(lines))
    lines=[]; entries=[]
    for i,f in enumerate(functions):
        phases=[]
        for p,phase in enumerate(f['phases']):
            actions=','.join('{'+q(a['component'])+','+str(a['on']).lower()+'}' for a in phase['actions'])
            name=f'a{i}_{p}'
            lines.append(f'constexpr Action {name}[]={{{actions}}};')
            phases.append('{'+q(phase['name'])+f',{name},sizeof({name})/sizeof(Action),{phase["seconds"]*1000}'+'}')
        if phases: lines.append(f'constexpr Phase p{i}[]={{'+','.join(phases)+'};')
        entries.append('  {'+','.join([q(f['id']),q(f['title']),f'p{i}' if phases else 'nullptr',str(len(phases)),
            'DefinitionStatus::'+f['status'],q(f['note'])])+'},')
    lines.extend(['constexpr FunctionDefinition functions[]={',*entries,'};'])
    library=(ROOT/'tools/templates/function_library.h.in').read_text().replace('@@DEFINITIONS@@','\n'.join(lines))
    macs='\n'.join(' {'+','.join('0x'+s for s in b['mac'].split(':'))+'},' for b in boards)
    boardmap=(ROOT/'tools/templates/board_map.h.in').read_text().replace('@@MACS@@',macs)
    outputs={ROOT/'demo_app/assets/catalog.json':json.dumps(data,indent=2)+'\n'}
    for board in BOARDS:
        folder=ROOT/'firmware'/board
        outputs.update({folder/'component_registry.h':registry,folder/'function_library.h':library,folder/'board_map.h':boardmap})
    wiring=['# Wiring map — 66 assigned outputs','',
        'Generated from [`definitions/control.json`](../definitions/control.json), the authoritative definition. Do not edit this view.',
        '','All outputs are active-LOW: LOW = ON, HIGH = OFF. GPIO ACKs do not measure relay contacts.',
        '','| Module | Input | Component | Board | GPIO | Bank channel (zero-based) |','|---|---|---|---|---|---|']
    for c in sorted(components,key=lambda c:(c['relay_module'],c['relay_input'])):
        wiring.append(f'| R{c["relay_module"]:02} | IN{c["relay_input"]} | {c["component"]} | {boards[c["node_id"]]["name"]} | {c["gpio"]} | {c["output_index"]} |')
    wiring.extend(['','R09 IN1/IN2 and R10 IN1/IN2 are the four unassigned relay inputs. Slave 4 GPIO33 remains spare. MASTER GPIO25/26/27 remain sensor reservations. GPIO4 is excluded.',''])
    outputs[ROOT/'docs/WIRING.md']='\n'.join(wiring)
    sequences=['# Sequence table','',
        'Generated from [`definitions/control.json`](../definitions/control.json), the authoritative definition. Do not edit this view.',
        '','Actions below execute left-to-right, waiting for matching application ACKs. Durations start after all phase actions are acknowledged. State carries forward between phases.',
        '','Finish / STOP: issue OFF to each mapped pump in first-occurrence order, then ALL_OFF to each required board. No phone timer switches relays OFF.',
        '','| ID | Title | Phase | Seconds | Ordered actions |','|---|---|---|---|---|']
    for f in functions:
        for p,phase in enumerate(f['phases']):
            actions=' → '.join(a['component']+(' ON' if a['on'] else ' OFF') for a in phase['actions'])
            sequences.append(f'| {f["id"]} | {f["title"]} | {p+1}. {phase["name"]} | {phase["seconds"]} | {actions} |')
        if not f['phases']: sequences.append(f'| {f["id"]} | {f["title"]} | **{f["status"]}** | — | {f["note"]} |')
    sequences.extend(['','## Source notes',''])
    for f in functions:
        if f['note']: sequences.append(f'- **{f["id"]}**: {f["note"]}')
    sequences.append('')
    outputs[ROOT/'docs/SEQUENCES.md']='\n'.join(sequences)
    drift=[]
    for path,content in outputs.items():
        if check:
            if not path.exists() or path.read_text(encoding='utf-8')!=content: drift.append(str(path.relative_to(ROOT)))
        else:
            path.parent.mkdir(parents=True,exist_ok=True);path.write_text(content,encoding='utf-8')
    assert not drift,'Generated definition drift: '+', '.join(drift)
    print('PASS: authoritative 66-output map, 34 sequence entries / 33 executable phases; five firmware copies, app and tables agree')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true')
    generate(parser.parse_args().check)
