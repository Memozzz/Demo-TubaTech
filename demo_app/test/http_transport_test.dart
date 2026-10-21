import 'dart:convert';
import 'dart:io';
import 'package:flutter_test/flutter_test.dart';
import 'package:demo_tubatech/controller.dart';

final probe = File('../.build/tests/http_frame_probe${Platform.isWindows ? '.exe' : ''}').absolute;

Future<Map<String, dynamic>> frameResult(List<int> bytes) async {
  final process = await Process.start(probe.path, []);
  final output = utf8.decoder.bind(process.stdout).join();
  final errors = utf8.decoder.bind(process.stderr).join();
  process.stdin.add(bytes);
  await process.stdin.close();
  final result = jsonDecode(await output) as Map<String, dynamic>;
  expect(await process.exitCode, 0, reason: await errors);
  return result;
}

class CapturedRequest {
  CapturedRequest(this.raw, this.headers, this.body, this.framing);
  final List<int> raw, body;
  final Map<String, String> headers;
  final Map<String, dynamic> framing;
}

// A raw TCP listener sees exactly what HttpClient puts on the wire, before any
// server-side HTTP parser can normalize headers or dechunk the request body.
class MasterFramingServer {
  MasterFramingServer(this.server) {
    server.listen((socket) {
      sockets.add(socket);
      final bytes = <int>[];
      var responding = false;
      socket.listen((chunk) {
        if (responding) return;
        bytes.addAll(chunk);
        final wire = latin1.decode(bytes);
        final end = wire.indexOf('\r\n\r\n');
        if (end < 0) return;
        final headers = <String, String>{};
        for (final line in wire.substring(0, end).split('\r\n').skip(1)) {
          final colon = line.indexOf(':');
          headers[line.substring(0, colon).toLowerCase()] = line.substring(colon + 1).trim();
        }
        final length = int.tryParse(headers['content-length'] ?? '') ?? 0;
        if (headers.containsKey('transfer-encoding')) {
          if (!wire.endsWith('0\r\n\r\n')) return;
        } else if (bytes.length < end + 4 + length) {
          return;
        }
        responding = true;
        pending.add(respond(socket, List.of(bytes), headers, end + 4));
      });
    });
  }
  final ServerSocket server;
  final sockets = <Socket>[];
  final pending = <Future<void>>[];
  final requests = <CapturedRequest>[];
  String get authority => '127.0.0.1:${server.port}';
  Future<void> respond(Socket socket, List<int> raw, Map<String, String> headers, int bodyAt) async {
    final parsed = await frameResult(raw);
    requests.add(CapturedRequest(raw, headers, raw.sublist(bodyAt), parsed));
    final accepted = parsed['accepted'] == true;
    final body = utf8.encode(jsonEncode({
      'framing_accepted': accepted,
      'path': parsed['path'],
      'message': accepted ? 'MASTER framing accepted' : 'MASTER framing rejected',
    }));
    socket.add(ascii.encode('HTTP/1.1 ${accepted ? 200 : 400} Response\r\n'
        'Content-Type: application/json\r\nContent-Length: ${body.length}\r\nConnection: close\r\n\r\n'));
    socket.add(body);
    await socket.flush();
    await socket.close();
  }
  Future<void> close() async {
    await server.close();
    await Future.wait(pending);
    for (final socket in sockets) { socket.destroy(); }
  }
}

void main() {
  setUpAll(() async {
    // Python + a C++17 compiler are already used by the firmware host checks.
    // Build the adapter from the actual header each run; never copy its rules.
    final build = await Process.run(Platform.isWindows ? 'python' : 'python3',
        ['../tools/build_http_frame_probe.py']);
    expect(build.exitCode, 0, reason: '${build.stdout}\n${build.stderr}');
  });

  for (final entry in {'START': '/api/functions/W_B/start', 'STOP': '/api/stop', 'RESET': '/api/reset'}.entries) {
    test('${entry.key} real HTTP sends exact Content-Length and passes MASTER HttpFrame', () async {
      final server = MasterFramingServer(await ServerSocket.bind(InternetAddress.loopbackIPv4, 0));
      var bindings = 0;
      final transport = WifiTransport(authority: server.authority, bindWifi: () async { bindings++; return true; });
      addTearDown(() async { transport.close(); await server.close(); });
      final form = {'boot_id': '123', 'request_id': 'wire_${entry.key.toLowerCase()}_0001',
        'stop_epoch': '7', 'note tag': 'café & +/🚰'};
      final expected = utf8.encode('boot_id=123&request_id=wire_${entry.key.toLowerCase()}_0001'
          '&stop_epoch=7&note+tag=caf%C3%A9+%26+%2B%2F%F0%9F%9A%B0');
      final response = await transport.request(entry.value, form: form);
      expect(response['framing_accepted'], true);
      expect(bindings, 1);
      final captured = server.requests.single;
      expect(captured.headers['content-length'], '${expected.length}');
      expect(captured.headers.containsKey('transfer-encoding'), false);
      expect(captured.headers['content-type'], 'application/x-www-form-urlencoded');
      expect(captured.body, expected);
      expect(captured.framing['method'], 'POST');
      expect(captured.framing['path'], entry.value);
      expect(captured.framing['body'], utf8.decode(expected));
    });
  }

  test('empty STOP form sends Content-Length zero and passes MASTER HttpFrame', () async {
    final server = MasterFramingServer(await ServerSocket.bind(InternetAddress.loopbackIPv4, 0));
    final transport = WifiTransport(authority: server.authority, bindWifi: () async => true);
    addTearDown(() async { transport.close(); await server.close(); });
    expect((await transport.request('/api/stop', form: {}))['framing_accepted'], true);
    expect(server.requests.single.headers['content-length'], '0');
    expect(server.requests.single.headers.containsKey('transfer-encoding'), false);
    expect(server.requests.single.body, isEmpty);
  });

  test('old streamed POST reproduces chunked encoding and MASTER rejection', () async {
    final server = MasterFramingServer(await ServerSocket.bind(InternetAddress.loopbackIPv4, 0));
    final client = HttpClient();
    addTearDown(() async { client.close(force: true); await server.close(); });
    final request = await client.postUrl(Uri.http(server.authority, '/api/stop'));
    request.headers.set(HttpHeaders.contentTypeHeader, 'application/x-www-form-urlencoded');
    request.write('boot_id=123&request_id=legacy_wire_0001'); // The pre-fix serializer.
    final response = await request.close();
    expect(response.statusCode, 400);
    expect((jsonDecode(await utf8.decoder.bind(response).join()) as Map)['framing_accepted'], false);
    expect(server.requests.single.headers['transfer-encoding'], 'chunked');
    expect(server.requests.single.headers.containsKey('content-length'), false);
  });

  test('MASTER parser rejects chunked, missing-length and changed Content-Type POSTs', () async {
    const base = 'POST /api/stop HTTP/1.1\r\nHost: 127.0.0.1\r\n';
    for (final invalid in [
      '${base}Content-Type: application/x-www-form-urlencoded\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n',
      '${base}Content-Type: application/x-www-form-urlencoded\r\n\r\n',
      '${base}Content-Type: application/json\r\nContent-Length: 0\r\n\r\n',
    ]) {
      expect((await frameResult(ascii.encode(invalid)))['accepted'], false);
    }
  });
}
