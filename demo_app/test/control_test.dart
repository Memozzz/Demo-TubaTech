import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:demo_tubatech/controller.dart';
import 'package:demo_tubatech/main.dart';

Map<String, dynamic> loadCatalog() => Map<String, dynamic>.from(jsonDecode(File('assets/catalog.json').readAsStringSync()) as Map);
Map<String, dynamic> readyStatus({bool busy = false, bool fault = false}) => {
  'layout_id': 'five-board-fixed-v1', 'catalog_version': 'final-66-2026-10-19', 'demo_control_version': 1,
  'stop_epoch': 7, 'boot_id': 123, 'busy': busy, 'fault': fault, 'reason': 'test fault', 'all_off_verified': !busy,
  'active_function': busy ? 'W_B' : 'none', 'phase': busy ? 'Wine initial -> PC1' : 'none', 'phase_index': 0,
  'state': busy ? 'RUNNING' : 'IDLE', 'remaining_ms': busy ? 12345 : 0,
  'nodes': List.generate(5, (id) => {'node_id': id, 'ready': true, 'acknowledged_mask': 0}),
  'profile': {'mappings': []},
};
class Transport extends MasterTransport {
  Map<String, dynamic>? status = readyStatus();
  final calls = <String>[];
  final forms = <Map<String, String>>[];
  Completer<Map<String, dynamic>>? startReply;
  bool failStop = false;
  @override
  Future<Map<String, dynamic>> request(String path, {Map<String, String>? form}) async {
    calls.add(path);
    if (form != null) forms.add(form);
    if (path == '/api/status') {
      if (status == null) throw const SocketException('offline');
      return status!;
    }
    if (path.endsWith('/start') && startReply != null) return startReply!.future;
    if (path == '/api/stop' && failStop) throw const SocketException('offline');
    return {'message': path == '/api/stop' ? 'STOP accepted; wait for acknowledged shutdown status' : 'START accepted'};
  }
  @override
  void close() {}
}
void main() {
  test('fixed source catalog includes 66 outputs, full sequences and W_B wiring', () {
    final c = DemoController(Transport(), loadCatalog());
    expect(c.components.length, 66);
    expect(c.functions.length, 34);
    expect(c.functions.where((f) => f['executable'] == true).length, 30);
    final wb = c.functions.firstWhere((f) => f['id'] == 'W_B');
    expect(c.requiredOwners(wb), {1, 2});
    expect(wb['phases'][0]['seconds'], 30);
    final v2 = c.components.firstWhere((m) => m['component'] == 'V2');
    expect([v2['node_id'], v2['gpio']], [1, 16]);
    final v3 = c.components.firstWhere((m) => m['component'] == 'V3');
    expect([v3['node_id'], v3['gpio']], [1, 17]);
    final pump = c.components.firstWhere((m) => m['component'] == 'PUMP_1');
    expect([pump['node_id'], pump['gpio']], [2, 16]);
    c.dispose();
  });
  test('offline boards only block sequences which use them; start is exactly one POST', () async {
    final t = Transport();
    final c = DemoController(t, loadCatalog());
    await c.poll();
    final wb = c.functions.firstWhere((f) => f['id'] == 'W_B');
    t.status!['nodes'][3]['ready'] = false;
    t.status!['nodes'][4]['ready'] = false;
    expect(c.canStart(wb), true);
    await c.start(wb);
    expect(t.calls.where((p) => p.endsWith('/start')).length, 1);
    expect(t.forms.single['stop_epoch'], '7');
    expect(t.forms.single.containsKey('unloaded'), false);
    t.status!['nodes'][1]['ready'] = false;
    expect(c.canStart(wb), false);
    t.status = null;
    await c.poll();
    expect(c.connected, false);
    expect(c.status, null);
    expect(c.canStart(wb), false);
    c.dispose();
  });
  test('STOP remains effective while START response is pending and late response stays ignored', () async {
    final t = Transport()..startReply = Completer();
    final c = DemoController(t, loadCatalog());
    await c.poll();
    final starting = c.start(c.functions.firstWhere((f) => f['id'] == 'W_B'));
    await Future<void>.delayed(Duration.zero);
    await c.stop();
    expect(t.calls, contains('/api/stop'));
    expect(c.message, contains('STOP complete'));
    t.startReply!.complete({'message': 'START accepted'});
    await starting;
    expect(c.message, contains('STOP complete'));
    expect(t.forms[1]['control_order'], isNot(t.forms[0]['control_order']));
    c.dispose();
  });
  test('unreachable STOP stays unconfirmed; reconnect sends STOP before clearing it', () async {
    final t = Transport()..status = null..failStop = true;
    final c = DemoController(t, loadCatalog());
    await c.stop();
    expect(c.stopPending, true);
    expect(c.message, contains('STOP unconfirmed'));
    t.status = readyStatus();
    t.failStop = false;
    await c.poll();
    expect(c.stopPending, true); // A pre-STOP OFF status cannot confirm STOP.
    await c.poll();
    expect(c.stopPending, false);
    expect(t.calls.where((p) => p == '/api/stop').length, 2);
    c.dispose();
  });
  test('STOP fault can be recovered explicitly without automatic restart', () async {
    final t = Transport()..status = readyStatus(fault: true);
    t.status!['all_off_verified'] = false;
    final c = DemoController(t, loadCatalog());
    await c.poll();
    await c.stop();
    expect(c.stopPending, true);
    expect(c.canReset, true);
    await c.resetFault();
    expect(t.calls, contains('/api/reset'));
    expect(c.stopPending, false);
    expect(t.calls.any((p) => p.endsWith('/start')), false);
    c.dispose();
  });
  testWidgets('app opens offline, shows sequence wiring and accessible STOP without animation', (tester) async {
    final t = Transport()..status = null..failStop = true;
    final c = DemoController(t, loadCatalog());
    await tester.pumpWidget(MaterialApp(home: ControlScreen(controller: c)));
    expect(find.text('Demo TubaTech'), findsOneWidget);
    expect(find.text('Disconnected'), findsOneWidget);
    expect(find.text('Countdown: —'), findsOneWidget);
    expect(find.textContaining('SLAVE 4 · OFFLINE'), findsOneWidget);
    expect(find.byKey(const Key('stop')), findsOneWidget);
    expect(t.calls, isEmpty);
    await tester.tap(find.byKey(const Key('stop')));
    await tester.pump();
    expect(c.stopPending, true);
    expect(find.textContaining('STOP unconfirmed'), findsOneWidget);
    await tester.pumpWidget(const SizedBox());
    c.dispose();
  });
  testWidgets('live countdown displays MASTER value without a phone timer', (tester) async {
    final t = Transport()..status = readyStatus(busy: true);
    final c = DemoController(t, loadCatalog());
    await c.poll();
    await tester.pumpWidget(MaterialApp(home: ControlScreen(controller: c)));
    expect(find.text('Countdown: 13 s'), findsOneWidget);
    await tester.pump(const Duration(seconds: 5));
    expect(find.text('Countdown: 13 s'), findsOneWidget);
    expect(find.byKey(const Key('stop')), findsOneWidget);
    await tester.pumpWidget(const SizedBox());
    c.dispose();
  });
  testWidgets('small phone can browse vinegar while offline and always reach STOP', (tester) async {
    tester.view.physicalSize = const Size(360, 640);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);
    final c = DemoController(Transport()..status = null, loadCatalog());
    await tester.pumpWidget(MaterialApp(home: ControlScreen(controller: c)));
    await tester.scrollUntilVisible(find.byType(DropdownButtonFormField<String>), 100);
    await tester.pumpAndSettle();
    await tester.tap(find.byType(DropdownButtonFormField<String>));
    await tester.pumpAndSettle();
    final target = find.text('V_I  Vinegar clean output');
    await tester.scrollUntilVisible(target, 220, scrollable: find.byType(Scrollable).last);
    await tester.pumpAndSettle();
    await tester.tap(target);
    await tester.pumpAndSettle();
    expect(find.text('START V_I'), findsOneWidget);
    expect(find.byKey(const Key('stop')), findsOneWidget);
    expect(tester.takeException(), null);
    await tester.pumpWidget(const SizedBox());
    c.dispose();
  });
}
