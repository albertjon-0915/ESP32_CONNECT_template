// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ignore_for_file: public_member_api_docs

import 'dart:async';
import 'dart:developer' as developer;
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:network_info_plus/network_info_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:esp_smartconfig/esp_smartconfig.dart';

// Sets a platform override for desktop to avoid exceptions. See
// https://flutter.dev/desktop#target-platform-override for more info.
void _enablePlatformOverrideForDesktop() {
  if (!kIsWeb && (Platform.isWindows || Platform.isLinux)) {
    debugDefaultTargetPlatformOverride = TargetPlatform.fuchsia;
  }
}

void main() {
  _enablePlatformOverrideForDesktop();
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
      ),
      home: const MyHomePage(title: 'ESP32 Connect App'),
    );
  }
}
// The main reason for running the app/root like in react
// =================================================================== //

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});
  final String title;

  // This is more like variable setup of sorts for stateful widget
  // used when doing input based reactiveness
  // change in states like component state in FF
  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  final NetworkInfo _networkInfo = NetworkInfo();

  final TextEditingController ssid = TextEditingController();
  final TextEditingController pass = TextEditingController();

  String? wifiName, wifiBSSID, wifiIPv4;

  @override
  void initState() {
    super.initState();
    _initNetworkInfo();
  }

  Future<void> _initNetworkInfo() async {
    String? name, bssid;
    try {
      wifiIPv4 = await _networkInfo.getWifiIP();

      if (kIsWeb) {
        wifiName = await _networkInfo.getWifiName();
        return;
      }

      final reqPermission = await Permission.locationWhenInUse.request();
      final fallbackReqPermission = await Permission.location.request();

      if ((reqPermission.isDenied || reqPermission.isRestricted) &&
          (fallbackReqPermission.isDenied ||
              fallbackReqPermission.isRestricted)) {
        wifiName = 'Unauthorized to get Wifi details';
        return;
      }

      name = await _networkInfo.getWifiName();
      bssid = await _networkInfo.getWifiBSSID();

      String? normalizeWifiName(String? stringName) {
        if (stringName == null) {
          return "";
        }

        return name!.replaceAll('"', '');
      }

      ssid.text = normalizeWifiName(name) ?? "";

      setState(() {
        wifiName = normalizeWifiName(name);
        wifiBSSID = bssid;
      });
      
    } on PlatformException catch (e) {
      developer.log('Failed', error: e);
      wifiName = 'Failed to get Wifi Name';
      wifiBSSID = 'Failed to get Wifi BSSID';
    }
  }

  Future<void> provisionCredsentials() async {
    final provisioner = Provisioner.espTouch();

    provisioner.listen((response) {
      Navigator.of(context).pop(response);
    });

    String bssidCreds = (wifiBSSID != null || wifiBSSID!.isNotEmpty)
        ? wifiBSSID!
        : "00:00:00:00:00:00";

    provisioner.start(
      ProvisioningRequest.fromStrings(
        ssid: ssid.text ?? "",
        bssid: bssidCreds,
        password: pass.text,
      ),
    );

    ProvisioningResponse? response = await showDialog<ProvisioningResponse>(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('Provisioning'),
          content: const Text('Provisioning started. Please wait...'),
          actions: <Widget>[
            TextButton(
              onPressed: () {
                Navigator.of(context).pop();
              },
              child: const Text('Stop'),
            ),
          ],
        );
      },
    );

    if (provisioner.running) {
      provisioner.stop();
    }

    if (response != null) {
      _onDeviceProvisioned(response);
    }
  }

  _onDeviceProvisioned(ProvisioningResponse response) {
    showDialog(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('Device provisioned'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: <Widget>[
              Text('Device successfully connected to the ${ssid.text} network'),
              SizedBox.fromSize(size: const Size.fromHeight(20)),
              const Text('Device:'),
              Text('IP: ${response.ipAddressText}'),
              Text('BSSID: ${response.bssidText}'),
            ],
          ),
          actions: <Widget>[
            TextButton(
              onPressed: () {
                Navigator.of(context).pop();
                ssid.text = "";
                pass.text = "";
              },
              child: const Text('OK'),
            ),
          ],
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: const Color.fromARGB(145, 19, 19, 19),
        titleTextStyle: TextStyle(
          color: Colors.white,
          fontSize: 16,
          fontWeight: FontWeight.w400,
        ),
        title: Text(widget.title),
      ),
      body: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: <Widget>[
          Column(
            children: [
              Image.network(
                'https://blog.grobotronics.com/wp-content/uploads/2024/05/esp32-c6-socs.png',
                height: 150,
              ),
              Padding(
                padding: const EdgeInsets.fromLTRB(24, 15, 24, 5),
                child: Text(
                  'Easier way to connect ESP32 to Wi-Fi made possible through provisioning credentials',
                  textAlign: TextAlign.center,
                  style: TextStyle(fontSize: 14, fontWeight: FontWeight.w400),
                ),
              ),
              Padding(
                padding: const EdgeInsets.fromLTRB(24, 15, 24, 0),
                child: Text(
                  (wifiName != null &&
                          wifiBSSID != null &&
                          wifiBSSID!.isNotEmpty)
                      ? 'Connected to $wifiName : $wifiBSSID'
                      : '',
                  textAlign: TextAlign.center,
                  style: TextStyle(fontSize: 14, fontWeight: FontWeight.w400),
                ),
              ),
            ],
          ),
          Padding(
            padding: const EdgeInsets.fromLTRB(32, 32, 32, 40),
            child: Column(
              children: [
                TextFormField(
                  controller: ssid,
                  decoration: InputDecoration(
                    labelText: 'Network name',
                    labelStyle: TextStyle(
                      color: Colors.black45,
                      fontSize: 14,
                      fontWeight: FontWeight.w400,
                    ),
                    enabledBorder: const UnderlineInputBorder(
                      borderSide: BorderSide(color: Colors.black26, width: 0.6),
                    ),
                    focusedBorder: const UnderlineInputBorder(
                      borderSide: BorderSide(color: Colors.black54, width: 1),
                    ),
                  ),
                ),
                TextFormField(
                  controller: pass,
                  decoration: InputDecoration(
                    labelText: 'Password',
                    labelStyle: TextStyle(
                      color: Colors.black45,
                      fontSize: 14,
                      fontWeight: FontWeight.w400,
                    ),
                    enabledBorder: const UnderlineInputBorder(
                      borderSide: BorderSide(color: Colors.black26, width: 0.6),
                    ),
                    focusedBorder: const UnderlineInputBorder(
                      borderSide: BorderSide(color: Colors.black54, width: 1),
                    ),
                  ),
                ),
              ],
            ),
          ),
          ElevatedButton(
            onPressed: provisionCredsentials,
            style: ElevatedButton.styleFrom(
              padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 24)
                  .clamp(
                    const EdgeInsets.symmetric(horizontal: 8, vertical: 8),
                    const EdgeInsets.symmetric(horizontal: 40, vertical: 8),
                  ),
              textStyle: TextStyle(fontSize: 18),
            ),
            child: Text(
              'Provision Credentials',
              style: TextStyle(color: Colors.black87),
            ),
          ),
        ],
      ),
    );
  }
}
