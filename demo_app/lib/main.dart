import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'controller.dart';

void main() => runApp(const DemoApp());

class DemoApp extends StatelessWidget {
  const DemoApp({super.key});
  @override
  Widget build(BuildContext context) => MaterialApp(
    title: 'Demo TubaTech', debugShowCheckedModeBanner: false,
    theme: ThemeData(useMaterial3: true, colorScheme: ColorScheme.fromSeed(
      seedColor: const Color(0xff087f72), brightness: Brightness.dark),
      scaffoldBackgroundColor: const Color(0xff101b25),
      cardTheme: const CardThemeData(margin: EdgeInsets.zero)),
    home: const CatalogLoader());
}

class CatalogLoader extends StatefulWidget {
  const CatalogLoader({super.key});
  @override
  State<CatalogLoader> createState() => _CatalogLoaderState();
}
class _CatalogLoaderState extends State<CatalogLoader> {
  DemoController? controller;
  String? error;
  @override
  void initState() { super.initState(); load(); }
  Future<void> load() async {
    try {
      final data = Map<String, dynamic>.from(jsonDecode(await rootBundle.loadString('assets/catalog.json')) as Map);
      if (!mounted) return;
      controller = DemoController(WifiTransport(), data)..begin();
      setState(() {});
    } catch (e) { if (mounted) setState(() => error = '$e'); }
  }
  @override
  void dispose() { controller?.dispose(); super.dispose(); }
  @override
  Widget build(BuildContext context) => controller == null
      ? Scaffold(body: Center(child: Text(error ?? 'Loading sequences…')))
      : ControlScreen(controller: controller!);
}

class ControlScreen extends StatefulWidget {
  const ControlScreen({super.key, required this.controller});
  final DemoController controller;
  @override
  State<ControlScreen> createState() => _ControlScreenState();
}
class _ControlScreenState extends State<ControlScreen> {
  DemoController get c => widget.controller;
  String selectedId = 'W_B';
  Map<String, dynamic> get selected => c.functions.firstWhere((f) => f['id'] == selectedId);
  static const boardNames = ['MASTER', 'SLAVE 1', 'SLAVE 2', 'SLAVE 3', 'SLAVE 4'];
  @override
  void initState() { super.initState(); c.addListener(changed); }
  void changed() { if (mounted) setState(() {}); }
  @override
  void dispose() { c.removeListener(changed); super.dispose(); }
  Widget section(String title, List<Widget> children) => Card(child: Padding(
    padding: const EdgeInsets.all(16), child: Column(crossAxisAlignment: CrossAxisAlignment.start,
      children: [Text(title, style: const TextStyle(fontSize: 17, fontWeight: FontWeight.w700)),
        const SizedBox(height: 12), ...children])));
  Future<void> wifi() async {
    try { await WifiTransport.channel.invokeMethod('openWifiSettings'); }
    catch (_) { c.message = 'Open phone Wi-Fi settings and join TubaTech_Master.'; changed(); }
  }
  @override
  Widget build(BuildContext context) {
    final s = c.connected ? c.status : null;
    final mappings = ((s?['profile'] as Map?)?['mappings'] as List? ?? []);
    final active = mappings
      .where((m) => (m as Map)['acknowledged_on'] == true).map((m) => (m as Map)['component']).join(', ');
    final unverified = mappings.where((m) => (m as Map)['acknowledged_on'] == null)
      .map((m) => (m as Map)['component']).join(', ');
    final remaining = ((s?['remaining_ms'] as num?)?.toDouble() ?? 0) / 1000;
    return Scaffold(
      appBar: AppBar(title: const Text('Demo TubaTech'), actions: [IconButton(
        tooltip: 'Refresh connection', onPressed: c.poll, icon: const Icon(Icons.refresh))]),
      body: ListView(padding: const EdgeInsets.fromLTRB(16, 4, 16, 20), children: [
        Row(children: [Icon(c.connected ? Icons.wifi : Icons.wifi_off,
          color: c.connected ? Colors.tealAccent : Colors.orangeAccent), const SizedBox(width: 10),
          Expanded(child: Text(c.connected ? 'Connected to MASTER' : 'Disconnected',
            style: const TextStyle(fontSize: 20, fontWeight: FontWeight.bold))),
          TextButton(onPressed: wifi, child: const Text('Wi-Fi'))]),
        if (!c.connected) const Padding(padding: EdgeInsets.symmetric(vertical: 8), child: Text(
          'Join TubaTech_Master using the device Wi-Fi password.\nKeep this Wi-Fi connection if Android reports no internet.')),
        Wrap(spacing: 6, runSpacing: 4, children: List.generate(5, (id) {
          final board = c.boards.where((b) => b['node_id'] == id).firstOrNull;
          final ready = board?['ready'] == true;
          final label = ready ? 'READY' : board?['session_verified'] == true ? 'NOT READY' : 'OFFLINE';
          return Chip(avatar: Icon(Icons.circle, size: 10, color: ready ? Colors.tealAccent : Colors.grey),
            label: Text('${boardNames[id]} · $label', style: const TextStyle(fontSize: 11)));
        })),
        const SizedBox(height: 14),
        section('Live execution', [
          Text(s == null ? 'No live hardware status' : '${s['active_function']} · ${s['state']}',
            style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w600)),
          const SizedBox(height: 8),
          Text(s == null || s['busy'] != true ? 'Step: —' : 'Step ${(s['phase_index'] as num).toInt() + 1}: ${s['phase']}'),
          Text(s == null ? 'Countdown: —' : s['busy'] == true && s['state'] != 'RUNNING'
            ? 'Waiting for output ACKs' : 'Countdown: ${remaining.ceil()} s', style: const TextStyle(fontSize: 24)),
          Text(s == null ? 'Active components: unknown' : 'Acknowledged ON: ${active.isEmpty ? 'none' : active}'),
          if (unverified.isNotEmpty) Text('Pending / unverified: $unverified'),
          if (s != null) const Text('GPIO acknowledgements; physical contact state is unmeasured.', style: TextStyle(fontSize: 11)),
          if (c.fault) Padding(padding: const EdgeInsets.only(top: 8), child: Text('FAULT: ${s?['reason']}',
            style: const TextStyle(color: Colors.orangeAccent))),
          if (c.fault) TextButton(onPressed: c.canReset ? c.resetFault : null,
            child: const Text('Reset fault · verify outputs OFF')),
        ]),
        const SizedBox(height: 14),
        section('Choose a sequence', [
          DropdownButtonFormField<String>(initialValue: selectedId, isExpanded: true,
            decoration: const InputDecoration(labelText: 'Wine / vinegar', border: OutlineInputBorder()),
            items: c.functions.map((f) => DropdownMenuItem(value: f['id'] as String,
              child: Text('${f['id']}  ${f['title']}', overflow: TextOverflow.ellipsis))).toList(),
            onChanged: (v) { if (v != null) setState(() => selectedId = v); }),
          const SizedBox(height: 12),
          Text(c.startReason(selected), style: TextStyle(color: c.canStart(selected) ? Colors.tealAccent : Colors.orangeAccent)),
          if ((selected['note'] as String).isNotEmpty) Padding(padding: const EdgeInsets.only(top: 8),
            child: Text(selected['note'] as String, style: const TextStyle(fontSize: 12))),
          const SizedBox(height: 8),
          Text('Required: ${c.requiredOwners(selected).toList().map((id) => boardNames[id]).join(', ')}'),
        ]),
        const SizedBox(height: 14),
        section('Sequence steps · fixed wiring', [
          for (final entry in (selected['phases'] as List).asMap().entries) ...[
            Text('${entry.key + 1}. ${entry.value['name']} · ${entry.value['seconds']} s',
              style: const TextStyle(fontWeight: FontWeight.bold)),
            for (final action in entry.value['actions'] as List) actionRow(action as Map),
            const SizedBox(height: 10),
          ],
          if ((selected['phases'] as List).isEmpty) const Text('No executable relay steps are defined.'),
          if ((selected['phases'] as List).isNotEmpty) const Text(
            'Finish / STOP: pumps OFF, then all required outputs OFF.\nTimers start after the step’s command acknowledgements.'),
        ]),
      ]),
      bottomNavigationBar: SafeArea(top: false, child: Padding(padding: const EdgeInsets.fromLTRB(16, 8, 16, 12),
        child: Column(mainAxisSize: MainAxisSize.min, children: [
          Text(c.message, maxLines: 3, overflow: TextOverflow.ellipsis, style: const TextStyle(fontSize: 12)),
          const SizedBox(height: 8),
          SizedBox(width: double.infinity, child: FilledButton.icon(
            onPressed: c.canStart(selected) ? () => c.start(selected) : null,
            icon: const Icon(Icons.play_arrow), label: Text('START $selectedId'))),
          const SizedBox(height: 6),
          SizedBox(width: double.infinity, height: 52, child: FilledButton.icon(
            key: const Key('stop'), style: FilledButton.styleFrom(backgroundColor: const Color(0xffbc253b), foregroundColor: Colors.white),
            onPressed: c.stop, icon: const Icon(Icons.stop_circle_outlined),
            label: const Text('STOP ALL OUTPUTS', style: TextStyle(fontSize: 17, fontWeight: FontWeight.bold)))),
        ]))),
    );
  }
  Widget actionRow(Map action) {
    final name = action['component'];
    final map = c.components.firstWhere((m) => m['component'] == name);
    return Padding(padding: const EdgeInsets.only(top: 5), child: Text(
      '${action['on'] == true ? 'ON' : 'OFF'}  $name · ${boardNames[map['node_id'] as int]} GPIO${map['gpio']} · R${map['relay_module'].toString().padLeft(2, '0')} IN${map['relay_input']}'));
  }
}
