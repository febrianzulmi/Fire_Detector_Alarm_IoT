#include <CTBot.h>

#include <DHT.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_I2C.h>=

#define DHTPIN 13
#define DHTTYPE DHT22


CTBot myBot ;

String token = "";
const int id = 1417192729;

int MQ2PIN = 35;

float p = 0.54;
float RL = 10.0;
float M = 47.38;
float m = -0.45720;
float b = 1.25;
//float Ro = 3.70;
float Ro;
float R2;


DHT dht(DHTPIN, DHTTYPE);
Adafruit_INA219 ina219;

float suhu1 = 0.0;
float suhu = 0.0;
float current = 0.0;
float gasValue = 0.0;

float suhuTadi = 0.0;
unsigned long tadiSuhu = 0;
const long intervalSuhu = 1000;

float arusTadi = 0;
unsigned long tadiINA219 = 0;
const long intervalINA219 = 1000; // Interval in milliseconds

float gasTadi = 0;
unsigned long tadiMQ2 = 0;
const long intervalMQ2 = 1000; // Interval in milliseconds

const char* ssid = "";              // SSID WiFi
const char* password = "";         // Password WiFi
const char* mqtt_server = ""; // MQTT Server
const char* userMQTT = "";         // Username MQTT
const char* passMQTT = "";       // Password MQTT

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long LastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

const char* topicDHT22 = "sensor1/DHT22";
const char* topicINA219 = "sensor2/INA219";
const char* topicMQ2 = "sensor3/MQ2";

// Tingkatan bahaya suhu
const float suhuSafe = 20.0;
const float suhuWarning = 35.0;
const float suhuDanger = 40.0;

// Tingkatan bahaya arus
const float currentSafe = 0.5;
const float currentWarning = 1.5;
const float currentDanger = 2.0;

// Tingkatan bahaya gas
const float gasSafe = 300.0;
const float gasWarning = 500.0;
const float gasDanger = 1000.0;

// LCD 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD I2C address

// RGB LED
const int redPin = 2;
const int greenPin = 4;
const int bluePin = 5;

// Buzzer
const int pinbuzzer = 33;


void setup_wifi()
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message Arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msg;

  for (int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }

  Serial.println(msg);
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT Connection...");
    String clientId = "node-redxx1";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), userMQTT, passMQTT))
    {
      Serial.println("connected");
      client.subscribe(topicDHT22);
      client.subscribe(topicINA219);
      client.subscribe(topicMQ2);
    }
    else
    {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  dht.begin();
  ina219.begin();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.print("WELCOME");
  lcd.backlight();
  lcd.blink();


  // Initialize RGB LED
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(pinbuzzer, OUTPUT);

  myBot.wifiConnect(ssid, password);
  myBot.setTelegramToken(token);

  if (myBot.testConnection())
    Serial.println("Koneksi Berhasil");
  else
    Serial.println("Koneksi Gagal");
  delay (500);

  // Initialize Buzzer
  //  pinMode(buzzerPin, OUTPUT);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  bacaSuhu();
  bacaSensorINA219();
  bacaSensorMQ2();
}

void bacaSuhu()
{
  if (millis() - tadiSuhu >= intervalSuhu)
  {
    tadiSuhu = millis();

    suhu1 =  dht.readTemperature();
    suhu = 1.0149 * suhu1 - 1.0058;

    if (suhu != suhuTadi)
    {
      suhuTadi = suhu;
      publishSuhu();

      // Pengondisian untuk tingkatan bahaya suhu
      if (suhu >= suhuDanger)
      {
        Serial.println("Tingkat bahaya suhu: Danger");

        digitalWrite(redPin, HIGH);
        digitalWrite(greenPin, LOW);
        digitalWrite(bluePin, LOW);
        digitalWrite(pinbuzzer, HIGH);
        delay(100);
        myBot.sendMessage(id, "DANGER! ADA KEBAKARAN ");

        //        tone(buzzerPin, 1000, 1000);
      }
      else if (suhu >= suhuWarning)
      {
        Serial.println("Tingkat bahaya suhu: Warning");

        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, LOW);
        digitalWrite(bluePin, HIGH);
        //        tone(buzzerPin, 800, 1000);
        //        digitalWrite(pinbuzzer, HIGH);
        delay(5000);
        myBot.sendMessage(id, "WARNING! SUHU RUANGAN TERLALU TINGGI. BERPOTENSI KEBAKARAN ");

      }
      else if (suhu >= suhuSafe)
      {
        Serial.println("Tingkat bahaya suhu: Safe");

        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, HIGH);
        digitalWrite(bluePin, LOW);
        //digitalWrite(pinbuzzer, HIGH);
        myBot.sendMessage(id, "KONDISI RUANGAN AMAN");

        //        noTone(buzzerPin);
      }
    }
  }
  lcd.setCursor(0, 1);
  lcd.print("Suhu: ");
  lcd.print(suhu);
  lcd.print(" C     ");
  lcd.blink();

}

void bacaSensorINA219()
{
  if (millis() - tadiINA219 >= intervalINA219)
  {
    tadiINA219 = millis();

    current = (ina219.getCurrent_mA() + 0.8);

    if (current != arusTadi)
    {
      arusTadi = current;
      publishINA219();

      // Pengondisian untuk tingkatan bahaya arus
      if (current >= currentDanger)
      {
        Serial.println("Tingkat bahaya arus: Danger");

        digitalWrite(redPin, HIGH);
        digitalWrite(greenPin, LOW);
        digitalWrite(bluePin, LOW);
        digitalWrite(pinbuzzer, HIGH);
        delay(100);
        myBot.sendMessage(id, "DANGER! ADA KEBAKARAN  ");

        //      tone(buzzerPin, 1000, 1000);
      }
      else if (current >= currentWarning)
      {
        Serial.println("Tingkat bahaya arus: Warning");

        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, LOW);
        digitalWrite(bluePin, HIGH);
        digitalWrite(pinbuzzer, HIGH);
        delay(5000);
        myBot.sendMessage(id, "WARNING! ARUS TERJADI KEBOCORAN ");

        //      tone(buzzerPin, 800, 1000);
      }
      else if (current <= currentSafe)
      {
        Serial.println("Tingkat bahaya arus: Safe");

        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, HIGH);
        digitalWrite(bluePin, LOW);
        digitalWrite(pinbuzzer, LOW);

        myBot.sendMessage(id, "KONDISI ARUS AMAN ");

        //      tone(buzzerPin, 800, 1000);
        //      noTone(buzzerPin);
      }
      lcd.setCursor(0, 1);
      lcd.print("Arus: ");
      lcd.print(current);
      lcd.print(" mA  ");
      delay(500);


    }
    lcd.setCursor(0, 0 );
    lcd.print("SISTEM DETEKTOR 3 IN 1");
    delay(500);
  }
}

void bacaSensorMQ2()
{
  if (millis() - tadiMQ2 >= intervalMQ2)
  {
    tadiMQ2 = millis();

    gasValue = analogRead(MQ2PIN);
    float voltage = (gasValue / 1023.0) * 5.0;
    float Ro = ((RL * (5 - voltage)) / voltage);
    float R2 = (5 * RL / voltage) - RL;
    float ratio = R2 / Ro;
    float ppm = (0.4 * pow((Ro / RL), -1.36));
    float mg = (ppm * M) / 24.45;
    float ug = mg * 1000;
    float g = ug * 0.000001;
    float volume = g / 0.54;

    //Setelah kalibrasi
    float volume_kal = (0.149 * (volume)) - 0.0007; //Persamaan kalibrasi
    float g_kal = volume_kal * 0.54;
    float ug_kal = g_kal * 0.000001;
    float mg_kal = ug_kal / 1000;
    float ppm_kal = (mg_kal * 24.45) / M;

    if (gasValue != gasTadi)
    {
      gasTadi = gasValue;
      publishMQ2();

      // Pengondisian untuk tingkatan bahaya gas
      if (gasValue >= gasDanger)
      {
        Serial.println("Tingkat bahaya gas: Danger");
        digitalWrite(redPin, HIGH);
        digitalWrite(greenPin, LOW);
        digitalWrite(bluePin, LOW);
        digitalWrite(pinbuzzer, HIGH);
        myBot.sendMessage(id, "DANGER! ADA KEBAKARAN KARENA GAS");
        delay(100);

        //      tone(buzzerPin, 1000, 1000);
      }
      else if (gasValue >= gasWarning)
      {
        Serial.println("Tingkat bahaya gas: Warning");


        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, LOW);
        digitalWrite(bluePin, HIGH);
        digitalWrite(pinbuzzer, LOW);
        myBot.sendMessage(id, "WARNING! GAS TERLALU TINGGI. BERPOTENSI KEBAKARAN ");

        //      tone(buzzerPin, 800, 1000);
      }
      else if (gasValue <= gasSafe)
      {
        Serial.println("Tingkat bahaya gas: Safe");


        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, HIGH);
        digitalWrite(bluePin, LOW);
        digitalWrite(pinbuzzer, LOW);
        myBot.sendMessage(id, "KONDISI GAS AMAN");
        //        digitalWrite(pinbuzzer, LOW);


        //      noTone(buzzerPin);
      }

      lcd.setCursor(0, 1);
      lcd.print("Gas: ");
      lcd.print(gasValue);
      lcd.print(" ppm  ");
      delay(500);
      lcd.clear();
    }
    lcd.setCursor(0, 0 );
    lcd.print("SISTEM DETEKTOR 3 IN 1");
    delay(500);
  }
}

void publishSuhu()
{
  StaticJsonDocument<200> doc;
  doc["suhu"] = String(suhu);
  String output;
  serializeJson(doc, output);
  client.publish(topicDHT22, output.c_str());
}

void publishINA219()
{
  StaticJsonDocument<200> doc;
  doc["current"] = String(current);
  String output;
  serializeJson(doc, output);
  client.publish(topicINA219, output.c_str());
}

void publishMQ2()
{
  StaticJsonDocument<200> doc;
  doc["gasValue"] = String(gasValue);
  String output;
  serializeJson(doc, output);
  client.publish(topicMQ2, output.c_str());
}
