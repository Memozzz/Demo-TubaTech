"""Validate JSON emitted by the actual MASTER host tests in both modes."""
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[1]
def read(name): return json.loads((ROOT/'.build/tests'/name).read_text())
if __name__=='__main__':
    status=read('partial_running.json');done=read('partial_complete.json')
    assert status['partial_demo_enabled'] and status['excluded_board_ids']==[2]
    absent=next(n for n in status['nodes'] if n['node_id']==2)
    assert absent['status']=='NOT INSTALLED' and absent['installed'] is False and absent['excluded']
    control=json.loads((ROOT/'definitions/control.json').read_text())
    assert absent['mac']==control['boards'][2]['mac'] and absent['acknowledged_mask'] is None
    assert not absent['ready'] and not absent['session_verified'] and not absent['outputs_armed'] and not absent['pending']
    mapped={m['component']:m for m in status['profile']['mappings']}
    pump=mapped['PUMP_1']
    assert pump['node_id']==2 and pump['gpio']==16 and pump['relay_module']==5 and pump['relay_input']==1
    assert pump['skipped'] and pump['execution']=='SKIPPED / NOT EXECUTED'
    assert pump['commanded_on'] is None and pump['acknowledged_on'] is None and not pump['mapping_verified']
    assert mapped['V2']['acknowledged_on'] and mapped['V3']['acknowledged_on']
    assert status['remaining_ms']==30000 and status['required_node_mask']==1 and status['skipped_action_count']==1
    assert done['state']=='COMPLETE_PARTIAL' and done['partial_completion'] and not done['fault']
    assert done['all_off_verified'] and done['all_off_scope']=='PARTICIPATING BOARDS ONLY'
    assert all(not m['acknowledged_on'] for m in done['profile']['mappings'] if not m['skipped'])
    functions={f['id']:f for f in read('partial_functions.json')['functions']}
    assert sum(f['executable'] for f in functions.values())==30
    assert {k for k,v in functions.items() if not v['executable']}=={'W_E','W_I','W_O','W_DAILY'}
    assert functions['W_B']['executable'] and functions['W_B']['partial_demo']
    assert [a['component'] for a in functions['W_B']['phases'][0]['actions']]==['V2','V3','PUMP_1']
    assert [a['skipped'] for a in functions['W_B']['phases'][0]['actions']]==[False,False,True]
    assert functions['V_C1']['required_node_mask']==0 and functions['V_C1']['phases'][0]['seconds']==30
    assert all(a['skipped'] for a in functions['V_C1']['phases'][0]['actions'])
    assert not functions['W_F']['partial_demo']
    full=read('integrated_status.json')
    assert full['partial_demo_enabled'] is False and full['excluded_board_ids']==[]
    print('PASS: actual partial/full API JSON, NOT INSTALLED board, null skipped ACKs, preserved PUMP_1 mapping, partial completion and availability')
