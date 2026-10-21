import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

abstract class MasterTransport {
  Future<Map<String, dynamic>> request(String path, {Map<String, String>? form});
  void close();
}

class WifiTransport implements MasterTransport {
  static const channel = MethodChannel('demo_tubatech/wifi');
  final HttpClient _client = HttpClient()..connectionTimeout = const Duration(seconds: 2);
  @override
  Future<Map<String, dynamic>> request(String path, {Map<String, String>? form}) async {
    if (Platform.isAndroid) {
      final bound = await channel.invokeMethod<bool>('bindMasterWifi');
      if (bound != true) throw const SocketException('Join TubaTech_Master Wi-Fi');
    }
    final uri = Uri.http('192.168.4.1', path);
    final req = await _client.openUrl(form == null ? 'GET' : 'POST', uri)
        .timeout(const Duration(seconds: 2));
    req.persistentConnection = false;
    if (form != null) {
      req.headers.set(HttpHeaders.contentTypeHeader, 'application/x-www-form-urlencoded');
      req.write(form.entries.map((e) => '${Uri.encodeQueryComponent(e.key)}=${Uri.encodeQueryComponent(e.value)}').join('&'));
    }
    final res = await req.close().timeout(const Duration(seconds: 4));
    final body = await utf8.decoder.bind(res).join().timeout(const Duration(seconds: 4));
    final data = Map<String, dynamic>.from(jsonDecode(body) as Map);
    if (res.statusCode < 200 || res.statusCode >= 300) {
      throw StateError(data['message']?.toString() ?? 'MASTER rejected command (${res.statusCode})');
    }
    return data;
  }
  @override
  void close() => _client.close(force: true);
}

class DemoController extends ChangeNotifier {
  DemoController(this.transport, this.catalog);
  final MasterTransport transport;
  final Map<String, dynamic> catalog;
  Map<String, dynamic>? status;
  bool connected = false, polling = false, sending = false, stopPending = false;
  bool _disposed = false, _stopAccepted = false;
  int _generation = 0, _order = 0;
  final String clientId = 'demo_${DateTime.now().microsecondsSinceEpoch}_${Random.secure().nextInt(1 << 30)}';
  String message = 'Join TubaTech_Master Wi-Fi to control relays.';
  Timer? _poller;
  DateTime? _lastStatusAt;
  List<Map<String, dynamic>> get functions => (catalog['functions'] as List).cast<Map<String, dynamic>>();
  List<Map<String, dynamic>> get components => (catalog['components'] as List).cast<Map<String, dynamic>>();
  bool get compatible => connected && status?['layout_id'] == 'five-board-fixed-v1' &&
      status?['catalog_version'] == catalog['catalog_version'] && status?['demo_control_version'] == 1;
  bool get busy => connected && status?['busy'] == true;
  bool get fault => connected && status?['fault'] == true;
  bool get canReset => compatible && fault && !busy && !sending;
  List<Map<String, dynamic>> get boards => connected
      ? ((status?['nodes'] as List?) ?? []).map((e) => Map<String, dynamic>.from(e as Map)).toList() : [];
  Set<int> requiredOwners(Map<String, dynamic> function) {
    final names = <String>{};
    for (final p in function['phases'] as List) {
      for (final a in (p as Map)['actions'] as List) { names.add((a as Map)['component'] as String); }
    }
    return components.where((c) => names.contains(c['component'])).map((c) => c['node_id'] as int).toSet();
  }
  bool canStart(Map<String, dynamic> function) => compatible && !sending && !stopPending && !busy && !fault &&
      status?['recovering'] != true && function['executable'] == true && requiredOwners(function).every(
        (id) => boards.any((b) => b['node_id'] == id && b['ready'] == true && b['acknowledged_mask'] == 0 &&
            b['command_uncertain'] != true));
  String startReason(Map<String, dynamic> function) {
    if (function['executable'] != true) return function['note'] as String;
    if (!connected) return 'Disconnected · sequences remain available to view.';
    if (!compatible) return 'Upload the matching Demo TubaTech MASTER firmware.';
    if (stopPending) return 'Waiting for acknowledged STOP.';
    if (fault) return 'MASTER fault: ${status?['reason']}';
    if (busy || sending) return 'MASTER operation in progress.';
    if (status?['recovering'] == true) return 'MASTER is verifying outputs OFF.';
    if (!canStart(function)) return 'Waiting for required boards to report ready and OFF.';
    return 'Ready · MASTER executes all steps and timers.';
  }
  void _emit() { if (!_disposed) notifyListeners(); }
  void begin() {
    poll();
    _poller = Timer.periodic(const Duration(milliseconds: 650), (_) {
      if (connected && _lastStatusAt != null &&
          DateTime.now().difference(_lastStatusAt!).inMilliseconds > 2500) {
        connected = false;
        status = null;
        _emit();
      }
      poll();
    });
  }
  Future<void> poll() async {
    if (polling || _disposed) return;
    polling = true;
    final generation = _generation;
    try {
      final data = await transport.request('/api/status');
      if (_disposed || generation != _generation) return;
      if (data['layout_id'] == null || data['boot_id'] is! num || data['nodes'] is! List) {
        throw const FormatException('Not a TubaTech MASTER response');
      }
      status = data;
      _lastStatusAt = DateTime.now();
      connected = true;
      if (stopPending && !sending) {
        final knownOff = data['busy'] == false && data['all_off_verified'] == true;
        if (_stopAccepted && knownOff) {
          stopPending = false;
          message = 'STOP complete · required outputs acknowledged OFF.';
        } else {
          await _sendStop();
        }
      }
    } catch (_) {
      if (generation == _generation && !_disposed) {
        connected = false;
        status = null;
      }
    } finally {
      polling = false;
      _emit();
    }
  }
  Map<String, String> _fields() => {
    'boot_id': '${status?['boot_id'] ?? 0}',
    'request_id': '${clientId}_${++_order}',
    'client_id': clientId,
    'control_order': '$_order',
    'stop_epoch': '${status?['stop_epoch'] ?? 0}',
  };
  Future<void> start(Map<String, dynamic> function) async {
    if (!canStart(function)) return;
    final generation = ++_generation;
    sending = true;
    message = 'Sending START ${function['id']}…';
    _emit();
    try {
      final reply = await transport.request('/api/functions/${function['id']}/start', form: _fields());
      if (generation == _generation) message = reply['message'] as String;
    } catch (e) {
      if (generation == _generation) {
        message = 'START unconfirmed: ${e.toString().replaceFirst('Bad state: ', '')}. STOP requested.';
        stopPending = true;
        _stopAccepted = false;
        connected = false;
        status = null;
      }
    } finally {
      if (generation == _generation) sending = false;
      _emit();
    }
    await poll();
  }
  Future<void> stop() async {
    ++_generation; // A late START response cannot overwrite STOP feedback.
    stopPending = true;
    _stopAccepted = false;
    message = 'Sending STOP…';
    await _sendStop();
    await poll();
  }
  Future<void> _sendStop() async {
    sending = true;
    _emit();
    try {
      final reply = await transport.request('/api/stop', form: _fields());
      _stopAccepted = true;
      message = '${reply['message']}';
    } catch (_) {
      connected = false;
      status = null;
      message = 'STOP unconfirmed · MASTER unreachable. Retrying on connection.';
    } finally { sending = false; _emit(); }
  }
  Future<void> resetFault() async {
    if (!canReset) return;
    ++_generation;
    sending = true;
    _emit();
    try {
      final reply = await transport.request('/api/reset', form: _fields());
      stopPending = false;
      _stopAccepted = false;
      message = '${reply['message']}';
    } catch (e) { message = '$e'; }
    finally { sending = false; _emit(); }
    await poll();
  }
  @override
  void dispose() { _disposed = true; _poller?.cancel(); transport.close(); super.dispose(); }
}
