import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'package:syncfusion_flutter_charts/charts.dart';
import 'package:percent_indicator/circular_percent_indicator.dart';

void main() {
  runApp(FireAlarmApp());
}

class FireAlarmApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      theme: ThemeData.dark(),
      home: Scaffold(
        resizeToAvoidBottomInset: false,
        appBar: AppBar(
          title: Text('SISTEM 3 IN 1 FIRE DETECTOR ALARM'),
          backgroundColor: Color.fromARGB(255, 17, 17, 17),
        ),
        body: FireAlarmPage(),
      ),
      debugShowCheckedModeBanner: false,
    );
  }
}

class FireAlarmPage extends StatefulWidget {
  @override
  _FireAlarmPageState createState() => _FireAlarmPageState();
}

class _FireAlarmPageState extends State<FireAlarmPage> {
  MqttServerClient? client;
  double currentReading = 0.0;
  double temperatureReading = 0.0;
  double gasReading = 0.0;
  List<TemperatureData> temperatureDataList = [];
  List<GasData> gasDataList = [];
  List<CurrentData> currentDataList = [];
  List<ChartData> chartDataList = [];

  @override
  void initState() {
    super.initState();
    connectToBroker();
  }

  void connectToBroker() async {
    client = MqttServerClient.withPort('', 'ClientID', 1883);
    client!.logging(on: true);
    client!.keepAlivePeriod = 20;
    client!.onConnected = onConnected;
    client!.onDisconnected = onDisconnected;

    final connMessage = MqttConnectMessage()
        .withClientIdentifier('ClientID')
        .keepAliveFor(20)
        .startClean()
        .authenticateAs('', '');
    client!.connectionMessage = connMessage;

    try {
      await client!.connect();
    } catch (e) {
      print('Menyambungkan: $e');
      client!.disconnect();
    }
  }

  void onConnected() {
    print('Connected to MQTT broker');
    subscribeToTemperature();
    subscribeToCurrent();
    subscribeToGas();
  }

  void onDisconnected() {
    print('Disconnected from MQTT broker');
  }

  void subscribeToTemperature() {
    const temperatureTopic = 'sensor1/DHT22';
    client!.subscribe(temperatureTopic, MqttQos.atMostOnce);

    client!.updates!.listen((List<MqttReceivedMessage<MqttMessage>> messages) {
      final message = messages[0].payload as MqttPublishMessage;
      final payload =
          MqttPublishPayload.bytesToStringAsString(message.payload.message);

      try {
        final jsonData = json.decode(payload);
        final temperatureString = jsonData['suhu'];

        if (temperatureString is String) {
          final temperature = double.tryParse(temperatureString);
          if (temperature != null) {
            setState(() {
              temperatureReading = double.parse(temperature.toStringAsFixed(3));
              updateTemperatureData(temperatureReading);
            });
          } else {
            print('Invalid value for temperature: $temperatureString');
          }
        } else {
          print('Invalid data format: $temperatureString');
        }
      } catch (e) {
        print('Error parsing JSON: $e');
      }
    });
  }

  void subscribeToCurrent() {
    const currentTopic = 'sensor2/INA219';
    client!.subscribe(currentTopic, MqttQos.atMostOnce);

    client!.updates!.listen((List<MqttReceivedMessage<MqttMessage>> messages) {
      final message = messages[0].payload as MqttPublishMessage;
      final payload =
          MqttPublishPayload.bytesToStringAsString(message.payload.message);

      try {
        final jsonData = json.decode(payload);
        final currentString = jsonData['current'];

        if (currentString is String) {
          final current = double.tryParse(currentString);
          if (current != null) {
            setState(() {
              currentReading = double.parse(current.toStringAsFixed(3));
              updateCurrentData(current);
            });
          } else {
            print('Invalid value for current: $currentString');
          }
        } else {
          print('Invalid data format: $currentString');
        }
      } catch (e) {
        print('Error parsing JSON: $e');
      }
    });
  }

  void subscribeToGas() {
    const gasTopic = 'sensor3/MQ2';
    client!.subscribe(gasTopic, MqttQos.atMostOnce);

    client!.updates!.listen((List<MqttReceivedMessage<MqttMessage>> messages) {
      final message = messages[0].payload as MqttPublishMessage;
      final payload =
          MqttPublishPayload.bytesToStringAsString(message.payload.message);

      try {
        final jsonData = json.decode(payload);
        final gasString = jsonData['gasValue'];

        if (gasString is String) {
          final gas = double.tryParse(gasString);
          if (gas != null) {
            setState(() {
              gasReading = double.parse(gas.toStringAsFixed(3));
              updateGasData(gas);
            });
          } else {
            print('Invalid value for gas: $gasString');
          }
        } else {
          print('Invalid data format: $gasString');
        }
      } catch (e) {
        print('Error parsing JSON: $e');
      }
    });
  }

  void updateTemperatureData(double temperature) {
    final now = DateTime.now();
    final data = TemperatureData(now, temperature);
    final chartData = ChartData(now, temperature, 'suhu');
    setState(() {
      temperatureDataList.add(data);
      chartDataList.add(chartData);
      temperatureReading = double.parse(temperature.toStringAsFixed(3));
    });
  }

  void updateCurrentData(double current) {
    final now = DateTime.now();
    final data = CurrentData(now, current);
    final chartData = ChartData(now, current, 'Current');
    setState(() {
      currentDataList.add(data);
      chartDataList.add(chartData);
      currentReading = double.parse(current.toStringAsFixed(3));
    });
  }

  void updateGasData(double gas) {
    final now = DateTime.now();
    final data = GasData(now, gas);
    final chartData = ChartData(now, gas, 'Gas');
    setState(() {
      gasDataList.add(data);
      chartDataList.add(chartData);
      gasReading = double.parse(gas.toStringAsFixed(3));
    });
  }

  void checkThresholds() {
    if (currentReading > 10) {
      showNotification('Warning', 'High current detected!');
    }
    if (temperatureReading > 30) {
      showNotification('Warning', 'High temperature detected!');
    }
    if (gasReading > 80) {
      showNotification('Warning', 'High gas level detected!');
    }
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: EdgeInsets.all(10),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Sensor Data',
            style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
          ),
          SizedBox(height: 10),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              CircularPercentIndicator(
                radius: 50,
                lineWidth: 5,
                percent: currentReading / 10000,
                center: Text(
                  '$currentReading  mA',
                  style: TextStyle(fontSize: 20),
                ),
                progressColor: Colors.yellow,
              ),
              CircularPercentIndicator(
                radius: 50,
                lineWidth: 5,
                percent: temperatureReading / 100,
                center: Text(
                  '$temperatureReading°C',
                  style: TextStyle(fontSize: 20),
                ),
                progressColor: Colors.red,
              ),
              CircularPercentIndicator(
                radius: 50,
                lineWidth: 5,
                percent: gasReading / 10000,
                center: Text(
                  '$gasReading ppm',
                  style: TextStyle(fontSize: 20),
                ),
                progressColor: Colors.green,
              ),
            ],
          ),
          SizedBox(height: 20),
          Text(
            'Temperature Data',
            style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
          ),
          SizedBox(height: 10),
          Expanded(
            child: SfCartesianChart(
              primaryXAxis: DateTimeAxis(),
              primaryYAxis: NumericAxis(),
              series: <ChartSeries>[
                LineSeries<TemperatureData, DateTime>(
                  dataSource: temperatureDataList,
                  xValueMapper: (TemperatureData data, _) => data.time,
                  yValueMapper: (TemperatureData data, _) => data.temperature,
                ),
              ],
            ),
          ),
          SizedBox(height: 20),
          Text(
            'Gas Data',
            style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
          ),
          SizedBox(height: 10),
          Expanded(
            child: SfCartesianChart(
              primaryXAxis: DateTimeAxis(),
              primaryYAxis: NumericAxis(),
              series: <ChartSeries>[
                LineSeries<GasData, DateTime>(
                  dataSource: gasDataList,
                  xValueMapper: (GasData data, _) => data.time,
                  yValueMapper: (GasData data, _) => data.gas,
                ),
              ],
            ),
          ),
          SizedBox(height: 20),
          Text(
            'Current Data',
            style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
          ),
          SizedBox(height: 10),
          Expanded(
            child: SfCartesianChart(
              primaryXAxis: DateTimeAxis(),
              primaryYAxis: NumericAxis(),
              series: <ChartSeries>[
                LineSeries<CurrentData, DateTime>(
                  dataSource: currentDataList,
                  xValueMapper: (CurrentData data, _) => data.time,
                  yValueMapper: (CurrentData data, _) => data.current,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  void showNotification(String s, String t) {}
}

class TemperatureData {
  final DateTime time;
  final double temperature;

  TemperatureData(this.time, this.temperature);
}

class GasData {
  final DateTime time;
  final double gas;

  GasData(this.time, this.gas);
}

class CurrentData {
  final DateTime time;
  final double current;

  CurrentData(this.time, this.current);
}

class ChartData {
  final DateTime time;
  final double value;
  final String type;

  ChartData(this.time, this.value, this.type);
}
