import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:demo_tubatech/controller.dart';
import 'package:demo_tubatech/main.dart';
import 'control_test.dart' show Transport, loadCatalog, readyStatus;

Map<String, dynamic> partialStatus() {
  final data = readyStatus();
  data['nodes'] = [for (final node in data['nodes'] as List) Map<String, dynamic>.from(node as Map)];
  data['partial_demo_enabled'] = true;
  data['skip_unready_at_start'] = true;
  data['excluded_board_ids'] = [2];
  data['nodes'][2] = {'node_id': 2, 'installed': false, 'excluded': true,
    'ready': false, 'status': 'NOT INSTALLED', 'acknowledged_mask': null};
  return data;
}
void main() {
  test('partial W_B keeps assignments, skips PUMP_1, requires only Slave 1', () async {
    final t = Transport()..status = partialStatus();
    final c = DemoController(t, loadCatalog());
    await c.poll();
    final wb = c.functions.firstWhere((f) => f['id'] == 'W_B');
    expect(c.assignedOwners(wb), {1, 2});
    expect(c.requiredOwners(wb), {1});
    expect(c.skippedComponents(wb), {'PUMP_1'});
    expect(c.startReason(wb), contains('PARTIAL DEMO'));
    expect(c.canStart(wb), true);
    await c.start(wb);
    expect(t.calls.where((p) => p.endsWith('/start')).toList(), ['/api/functions/W_B/start']);
    expect(t.calls.any((p) => p.contains('arm') || p.contains('confirm')), false);
    t.status!['nodes'][1]['ready'] = false;
    expect(c.canStart(wb), true); // Missing before START is skipped for the next run.
    expect(c.requiredOwners(wb), isEmpty);
    expect(c.skippedComponents(wb), {'V2', 'V3', 'PUMP_1'});
    t.status!['nodes'][1]['unsafe_for_start'] = true;
    expect(c.canStart(wb), false); // Never skip uncertain or potentially energized outputs.
    final unaffected = c.functions.firstWhere((f) => f['id'] == 'W_F');
    expect(c.skippedComponents(unaffected), isEmpty);
    expect(c.canStart(unaffected), true);
    c.dispose();
  });
  test('all-excluded phase remains executable, pending functions stay unavailable', () async {
    final c = DemoController(Transport()..status = partialStatus(), loadCatalog());
    await c.poll();
    final allSkipped = c.functions.firstWhere((f) => f['id'] == 'V_C1');
    expect(c.requiredOwners(allSkipped), isEmpty);
    expect(c.skippedComponents(allSkipped), {'VV2', 'VV3', 'VPUMP_1'});
    expect(c.canStart(allSkipped), true);
    expect(allSkipped['phases'][0]['seconds'], 30);
    for (final f in c.functions.where((f) => f['executable'] != true)) { expect(c.canStart(f), false); }
    c.dispose();
  });
  test('live disabled mode restores original readiness requirements', () async {
    final t = Transport()..status = partialStatus();
    t.status!['partial_demo_enabled'] = false;
    final c = DemoController(t, loadCatalog());
    await c.poll();
    final wb = c.functions.firstWhere((f) => f['id'] == 'W_B');
    expect(c.partialDemo, false);
    expect(c.requiredOwners(wb), {1, 2});
    expect(c.skippedComponents(wb), isEmpty);
    expect(c.canStart(wb), false);
    c.dispose();
  });
  test('invalid live exclusions fail closed and cannot exclude MASTER', () async {
    final t = Transport()..status = partialStatus();
    t.status!['excluded_board_ids'] = [0, 2];
    final c = DemoController(t, loadCatalog());
    await c.poll();
    expect(c.compatible, false);
    expect(c.canStart(c.functions.firstWhere((f) => f['id'] == 'W_B')), false);
    c.dispose();
  });
  test('partial STOP message acknowledges participating outputs only', () async {
    final c = DemoController(Transport()..status = partialStatus(), loadCatalog());
    await c.poll();await c.stop();
    expect(c.stopPending, false);
    expect(c.message, contains('participating outputs acknowledged OFF'));
    expect(c.message, contains('excluded outputs NOT EXECUTED'));
    c.dispose();
  });
  testWidgets('app labels excluded board, skipped outputs and partial completion honestly', (tester) async {
    final data = partialStatus();
    data['state'] = 'COMPLETE_PARTIAL';data['active_function'] = 'W_B';data['partial_completion'] = true;
    data['profile'] = {'mappings': [
      {'component': 'V2', 'acknowledged_on': false, 'skipped': false},
      {'component': 'PUMP_1', 'acknowledged_on': null, 'commanded_on': null, 'skipped': true},
    ]};
    final c = DemoController(Transport()..status = data, loadCatalog());
    await c.poll();
    await tester.pumpWidget(MaterialApp(home: ControlScreen(controller: c)));
    expect(find.text('SLAVE 2 · NOT INSTALLED'), findsOneWidget);
    expect(find.textContaining('PARTIAL DEMO COMPLETE'), findsOneWidget);
    expect(find.text('Acknowledged ON: none'), findsOneWidget);
    expect(find.text('Pending / unverified: PUMP_1'), findsNothing);
    expect(find.textContaining('SKIPPED / NOT EXECUTED: PUMP_1'), findsWidgets);
    expect(find.byKey(const Key('stop')), findsOneWidget);
    await tester.pumpWidget(const SizedBox());c.dispose();
  });
}
