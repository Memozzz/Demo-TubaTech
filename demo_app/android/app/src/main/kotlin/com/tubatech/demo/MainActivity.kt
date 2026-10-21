package com.tubatech.demo

import android.content.Intent
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.os.Build
import android.provider.Settings
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel

class MainActivity : FlutterActivity() {
    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, "demo_tubatech/wifi")
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "openWifiSettings" -> {
                        val action = if (Build.VERSION.SDK_INT >= 29) Settings.Panel.ACTION_WIFI else Settings.ACTION_WIFI_SETTINGS
                        startActivity(Intent(action))
                        result.success(null)
                    }
                    "bindMasterWifi" -> {
                        val manager = getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager
                        val network = manager.allNetworks.firstOrNull { network ->
                            manager.getNetworkCapabilities(network)?.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) == true &&
                                manager.getLinkProperties(network)?.linkAddresses?.any {
                                    it.address.hostAddress?.startsWith("192.168.4.") == true
                                } == true
                        }
                        result.success(if (network != null) manager.bindProcessToNetwork(network) else {
                            manager.bindProcessToNetwork(null)
                            false
                        })
                    }
                    else -> result.notImplemented()
                }
            }
    }
}
