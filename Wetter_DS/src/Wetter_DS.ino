//**ARDUINO EINSTELLUNGEN**
//Werkzeuge:
//Board: "WeMos D1 mini"
//Flash Size: "4M (3M SPIFFS)"
//Debug Port: "Disabled"
//Debug Level: "Keine"
//IwIP Variant: "v2 Lower Memory"
//CPU Frequency: "160 MHz" 80 geht auch
//Upload Speed: "115200"
//Erase Flash: "Only Sketch"
//Port: Siehe Gerätemanager
//Programmer: "AVRISP mkII"
//Serieller Monitor: Geschwindigkeit "9600Baud"

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
//#define BLYNK_DEBUG           // Comment this out to disable prints and save space
#define BLYNK_FANCY_LOGO_3D

#define ESP8266_LED 2

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <Wire.h>

#include <MQTTClient.h>

#include <WEMOS_SHT3X.h>  //Library für SHT30 Sensor

#include <TimeLib.h>
#include <WidgetRTC.h>

#include <SFE_MicroOLED.h> // Include the SFE_MicroOLED library

SHT3X sht30(0x45); //SHT30 auf Adresse 45

BlynkTimer timer;
WidgetRTC rtc;

WiFiClientSecure buero;
MQTTClient mclient;

//-------------------Blynk Pinbelegung-------------------
//V0: Temperatur
//V1: Temperatur Minimum
//V2: Temperatur Maximum
//V3: Luftfeuchtigkeit
//V4: Luftfeuchtigkeit Minimum
//V5: Luftfeuchtigkeit Maximum
//V6: Daten Refresh
//V7: Flip der OLED
//V8: Uhrzeit
//V9: Datum
//V10: Wartezeit ----nicht mehr benötigt----
//V11: Reset Min/Max
//V12: RelayOn
//V13: RelayManMode
//V14: RelayAutoTemp
//-------------------Blynk Pinbelegung-------------------

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "xxx"; //Blynk auth
// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[14]; //ssid und pass welches gebucht wird
char pass[14];
String ssid1 = "ssid1"; //ssid und pass mit Index wird gesucht
String pass1 = "pass1";
String ssid2 = "ssid2";
String pass2 = "pass2";
char host[] = "host";
char host2[] = "host2";
bool host1Down = false;
bool host2Down = false;
char fingerprint[] = "xxx"; //MD5
uint16_t port = 8083;
int sda = 4; //SDA für D1 Mini
int scl = 5; //SCL für D1 Mini
bool flip = false;
int xa = 0; //X0 Position für Display
int ya = 0; //Y0 Position für Display
int xb = 0; //X1 Position für Display
int yb = 0; //Y1 Position für Display
int n=0; //für Verschiedenes
int lineWidth = 3; //Linienbreite (Rechteck)
int lineHeight = 1; //Linienhöhe (Rechteck)
int warteZeit = 2;//48/48; //Wartezeit zur nächsten Messung in sec !Teiler von 48 für Display bevorzugen!
String mqttMessage = "";
bool mqconnectet = false;
float temp_old = 0;
float temp_new = 0;
float tempMin = 0;
float tempMax = 0;
float tempOff = 0;//-3; //Temperaturoffset, das original kann nicht stimmen!
float humidity_old = 0;
float humidity_new = 0;
float humidityMin = 0;
float humidityMax = 0;
bool aktuallisieren = false; //Trigger zum aktuallisieren der Anzeige
bool refresh = false; //Trigger von zum aktuallisieren

const int relayPin = D3;
const long interval = 5000;  // pause for 5 seconds
int relayState = LOW;
bool relayManMode = false;
bool relayManOn = false;
bool relayIsOn;
float relayTempSwitch = 25.1;
#define PIN_RESET 255  //
#define DC_JUMPER 0 // I2C Addres: 0 - 0x3C, 1 - 0x3D
MicroOLED oled(PIN_RESET, DC_JUMPER); // I2C Example

byte SerialByte;
String SerialString;
void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
  Wire.begin(sda, scl);
  #ifdef  BLYNK_DEBUG
    Serial.print("Wire verbunden: sda: ");
    Serial.print(sda);
    Serial.print(", scl: ");
    Serial.println(scl);
  #endif
  // Before you can start using the OLED, call begin() to init
  // all of the pins and configure the OLED.
  oled.begin();

  WiFi.mode(WIFI_STA);

  // clear(ALL) will clear out the OLED's graphic memory.
  // clear(PAGE) will clear the Arduino's display buffer.
  oled.clear(ALL);  // Clear the display's memory (gets rid of artifacts)
  // To actually draw anything on the display, you must call the
  // display() function.
  oled.clear(0);  // Clear the display's memory (gets rid of artifacts)
  oled.setCursor(0,0);
  oled.display();

  //Serial.println("** Scan Networks **");
  BLYNK_LOG1("** Scan Networks **");
  oled.print("Scan\nNetworks");
  oled.display();
  byte numSsid = WiFi.scanNetworks();

  //Serial.print("SSID List:");
  BLYNK_LOG1("SSID List:");
  //Serial.println(numSsid);
  BLYNK_LOG1(numSsid);
  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    Serial.println(WiFi.encryptionType(thisNet));

    if (WiFi.SSID(thisNet) == ssid1) {
      Serial.print("W-Lan gefunden: ");
      Serial.println(WiFi.SSID(thisNet));
      ssid1.toCharArray(ssid,(ssid1.length() + 1));
      pass1.toCharArray(pass,(pass1.length() + 1));
    }
    else if (WiFi.SSID(thisNet) == ssid2) {
      Serial.print("W-Lan gefunden: ");
      Serial.println(WiFi.SSID(thisNet));
      ssid2.toCharArray(ssid,(ssid2.length() + 1));
      pass2.toCharArray(pass,(pass2.length() + 1));
    }
  }

  //Serial.print("logge mich ein (Blynk.begin): ");
  //Serial.println(ssid);
  BLYNK_LOG2("logge mich ein (Blynk.begin): ",ssid);

  if (pass && strlen(pass)) {
    //WiFi.begin(ssid, pass);
   //Blynk.begin(auth, ssid, pass, host, port);//, fingerprint);
  //Blynk.beginn bleibt in While hängen, wenn keine Verbindung zum Server hergestellt werden kann
  Blynk.connectWiFi(ssid, pass);
  host1Down = false;
  Blynk.config(auth, host, port);
  if (Blynk.connect() != true) {
    //Wenn der erste nicht geht den 2ten Host versuchen
    host1Down = true;
    BLYNK_LOG2("HOST 1 nicht erreichbar! Versuche HOST2: ", host2);
    Blynk.config(auth, host2, port);
    if (Blynk.connect() != true) {
      BLYNK_LOG1("HOST 2 nicht erreichbar! Schade!");
      host2Down = true;
    }
    else {
      BLYNK_LOG1("HOST 2 erreichbar! Juhu!");
    }
  }
  } else {
    WiFi.begin(ssid);
  }

  oled.clear(0);  // Clear the display's memory (gets rid of artifacts)
  n = 0;
  while ((WiFi.status() != WL_CONNECTED) and n<40) {
    n++;
    oled.setCursor(0,0);
    oled.print("Wifi:\n");
    oled.print(ssid);
    oled.print("\nIP:\n");
    oled.print(WiFi.localIP());
    oled.print("\nWarte auf IP:");
    oled.print(n);
    oled.display();
    delay(500);
    //Serial.println("noch ein Versuch");
    BLYNK_LOG1("Warte auf WiFi connected");
  }
  oled.clear(0);  // Clear the display's memory (gets rid of artifacts)
  Serial.println("Connected to WiFi");

  if ( WiFi.status() != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    //while(true);
    while (!mclient.disconnect()) {
      delay(100);
    }
    delay(5000);
    ESP.deepSleep(5 * 60 * 1000000); // deepSleep time is defined in microseconds.
  }

  oled.setCursor(0,0);
  oled.clear(0);  // Clear the display's memory (gets rid of artifacts)

  oled.print("Wifi:\n");
  oled.print(ssid);
  oled.print("\nIP:\n");
  oled.print(WiFi.localIP());
  oled.display();
  delay(1000);

  pinMode(ESP8266_LED, OUTPUT);

  if (host1Down && !host2Down) {
    mclient.begin(host2, 1883, buero);
  }
  else {
    mclient.begin(host, 1883, buero);
  }
  mclient.onMessage(messageReceived);
  mqttconnect();

  setSyncInterval(15*60); // Sync interval in seconds (5 minutes)
  // Display digital clock every 1 milseconds
  timer.setInterval(1L, clockDisplay);

  #ifdef  BLYNK_DEBUG
    BLYNK_LOG1("Timer gestartet");
  #endif

  pinMode(relayPin, OUTPUT);

  mclient.publish("/buero/relayManOn", String(relayManOn));
  Blynk.virtualWrite(12, (relayManOn));  // virtual pin 12
  mclient.publish("/buero/relayManMode", String(relayManMode));
  Blynk.virtualWrite(13, (relayManMode));  // virtual pin 13
  mclient.publish("/buero/relayTempSwitch", String(relayTempSwitch));
  Blynk.virtualWrite(14, (relayTempSwitch));  // virtual pin 14

 timer.run();
  Serial.println("Setup fertig!");
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

void loop()
{
  if (Serial.available() > 0)                                                                       // Wenn Serieller Datenpuffer größer 0 Byte ist dann mach weiter
  {
   SerialString = char(Serial.read());                                                                   // Serieller Datenpuffer in "incomingByte" schreiben !! immer nue 1 Byte
                                                                                                   // Das ausgelesene Zeichen wird gelöscht aus dem Serieller Datenpuffer
   Serial.print("Empfangen erhielt: ");
   Serial.println(SerialString);
   Blynk.virtualWrite(99, SerialString);
  }


  #ifdef  BLYNK_DEBUG
    BLYNK_LOG1("\n LOOP start >>");
  #endif

  Blynk.run();
  #ifdef  BLYNK_DEBUG
    BLYNK_LOG1("Blynk.run gestartet");
  #endif

  oled.setCursor(0,0);
  oled.clear(0);  // Clear the display's memory (gets rid of artifacts)

  #ifdef  BLYNK_DEBUG
    BLYNK_LOG1("mclient.loop >>");
  #endif
  mclient.loop();
  #ifdef  BLYNK_DEBUG
    BLYNK_LOG1("mclient.loop <<");
  #endif

//  digitalWrite(ESP8266_LED, LOW);
//  delay(500);
//  digitalWrite(ESP8266_LED, HIGH);

  if(sht30.get()==0){
    #ifdef  BLYNK_DEBUG
      BLYNK_LOG1("SHT30: Werte gelesen");
    #endif
  }
  else
  {
    BLYNK_LOG1("SHT30: Error!");
    oled.println("SHT30:\nError!\n");              //Oled kann eine Sonderzeichen
    delay(1000);
  }

//get and print temperatures
  temp_new = sht30.cTemp;
  if ((temp_new < 125.1) && (temp_new > -40.1)){ // nur im Wertebereich
  if ((temp_old < (temp_new-0.1)) || (temp_old >  (temp_new+0.1))) {  //Nur aktuallisieren, wenn sich de Temperatur um mehr als 0.1° geändert hat
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    showLoadingPoint(3);
    aktuallisieren = true;
    if (!mclient.connected()) {
      mqttconnect();
    }
    temp_old = temp_new;
    if ((temp_new < tempMin) || (temp_new == 0)) {
      tempMin = temp_new;
      Blynk.virtualWrite(1, temp_new);  // virtual pin 1
      mclient.publish("/buero/tempMin", String(temp_new));
      BLYNK_LOG2("TempMin: ", tempMin);
    }
    if ((temp_new > tempMax) || (tempMax == 0)) {
      tempMax = temp_new;
      Blynk.virtualWrite(2, (tempMax));  // virtual pin 2
      mclient.publish("/buero/tempMax", String(tempMax));
           BLYNK_LOG2("TempMax: ", tempMax);
    }
    BLYNK_LOG3("Temp: ", temp_new+tempOff, " °C");
    Blynk.virtualWrite(0, (temp_new+tempOff));  // virtual pin 0
    mclient.publish("/buero/temp", String(temp_new+tempOff));
    showLoadingPoint(-3);
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  }
  }
  else {BLYNK_LOG2("Temperatur außerhalb!: ", temp_new);}
  oled.print("Tmp:");
  oled.print(temp_new+tempOff);
  oled.print("C");              //Oled kann eine Sonderzeichen
//  oled.print('\n');

//get and print humidity data
  humidity_new = sht30.humidity;
  if ((humidity_new < 100.1) && (humidity_new > 0.0)) { //nur im Wertebereich
  if ((humidity_old < (humidity_new-0.5)) || (humidity_old >  (humidity_new+0.5))) {  //Nur aktuallisieren, wenn sich de Temperatur um mehr als 0.5% geändert hat
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    aktuallisieren = true;
    showLoadingPoint(3);
    if (!mclient.connected()) {
      mqttconnect();
    }
    humidity_old = humidity_new;
    if ((humidity_new < humidityMin) || (humidityMin == 0)) {
      humidityMin = humidity_new;
      Blynk.virtualWrite(4, (humidityMin));  // virtual pin 4
      mclient.publish("/buero/humidityMin", String(humidityMin));
      BLYNK_LOG2("HumMin: ", humidityMin);
    }
    if ((humidity_new > humidityMax) || (humidityMax == 0)) {
      humidityMax = humidity_new;
      Blynk.virtualWrite(5, (humidityMax));  // virtual pin 5
      mclient.publish("/buero/humidityMax", String(humidityMax));
      BLYNK_LOG2("HumMax: ", humidityMax);
    }
    BLYNK_LOG3("Humidity: ", humidity_new, " %");
    Blynk.virtualWrite(3, humidity_new); // virtual pin 3
    mclient.publish("/buero/humidity", String(humidity_new));
    showLoadingPoint(-3);
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  }
  }
  else {BLYNK_LOG2("Humidity außerhalb!: ", humidity_new);}
  oled.print("Hmd:");
  oled.print(humidity_new);
  oled.print("%");
  oled.print('\n');

  relayIsOn = relaySwitch(relayManMode, relayManOn, relayTempSwitch, temp_new);
  oled.print("Switch: ");
  oled.print(relayIsOn);
  oled.print('\n');

  if (aktuallisieren) {
    clockDisplay();
    aktuallisieren = false;
  }

//  mclient.unsubscribe("/terrasse/temp");
  delay(10);  // <- fixes some issues with WiFi stability

  oled.println("Terrasse:");
  oled.print(mqttMessage);
  oled.println(" C");
//  Blynk.virtualWrite(1, mqttMessage); // virtual pin 1


//  yDisp = oled.getLCDHeight()-1;
  xa = oled.getLCDWidth()-lineWidth;
  ya = 0;
  xb = oled.getLCDWidth()-1;
  yb = oled.getLCDHeight()-1;

  lineHeight = oled.getLCDHeight();
//  oled.line(xa, ya, xa, yb);       //obere  horr
//  oled.line(xb, ya, xb, yb);       //untere horr
//  oled.line(xa, ya, xb, ya);       //linke  senk
//  oled.line(xa, yb, xb, yb);       //rechte senk
/*  Serial.println("for Schleife >>");
  for (int i=0; i<(warteZeit+1); i++) {
  Serial.print("for Schleife i = ");
  Serial.println(i);
    lineHeight = ((i*oled.getLCDHeight())/warteZeit);
    Serial.print("for Schleife lineHeight = ");
    Serial.println(lineHeight);
    #ifdef  BLYNK_DEBUG
      oled.setCursor(0, yb-oled.getFontHeight());
      oled.print(i);
      oled.print("*");
      oled.print(oled.getLCDHeight());
      oled.print("/");
      oled.print(warteZeit);
      oled.print("=");
      oled.print(lineHeight);
    #endif
    oled.rectFill(xa, ya, lineWidth, lineHeight);
    if (mqttMessage=="[Kai]DispOff") {
      oled.clear(0);  // Clear the display's memory (gets rid of artifacts)
    }
    oled.display();
    delay(1000); //1sec.
  }
  Serial.println("for Schleife <<");
*/
  oled.display();
  oled.flipVertical(flip);
  oled.flipHorizontal(flip);
//  if (warteZeit > 8) {
//    setup();
//  }
  //delay(20000);//20 sec.
  //ESP.deepSleep(10 * 1000000); //(5 * 60 * 1000000); // deepSleep time is defined in microseconds.
  #ifdef  BLYNK_DEBUG
    BLYNK_LOG1("LOOP <<");
  #endif
}

void showLoadingPoint(int rad) {
  if (rad > 0) {
    oled.circleFill(oled.getLCDWidth()-(2*rad), oled.getLCDHeight()-(2*rad), rad);
    oled.display();
  }
  if (rad < 0) {
    rad = -rad;
    oled.circle(oled.getLCDWidth()-(2*rad), oled.getLCDHeight()-(2*rad), rad);
    oled.display();
  }
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if (topic == "/buero/tempMin") {
    tempMin = payload.toFloat();
    Blynk.virtualWrite(1, (tempMin));  // virtual pin 1
  }
  if (topic == "/buero/tempMax") {
    tempMax = payload.toFloat();
    Blynk.virtualWrite(2, (tempMax));  // virtual pin 2
  }
  if (topic == "/buero/humidityMin") {
    humidityMin = payload.toFloat();
    Blynk.virtualWrite(4, (humidityMin));  // virtual pin 4
  }
  if (topic == "/buero/humidityMax") {
    humidityMax = payload.toFloat();
    Blynk.virtualWrite(5, (humidityMax));  // virtual pin 5
  }
  if (topic == "/terrasse/temp") {
    mqttMessage = payload;
  }
  if (topic == "/buero/updateRange") {
    warteZeit = payload.toInt();
    Blynk.virtualWrite(V10, warteZeit);
  }
  if (topic == "/buero/flip") {
    flip = payload.toInt();
    Blynk.virtualWrite(V7, flip);
  }
  if (topic == "/buero/relayTempSwitch") {
    relayTempSwitch = payload.toFloat();
  }
  if (topic == "/buero/relayManualMode") {
    relayManMode = payload.toInt();
  }
  if (topic == "/buero/relayManOn") {
    relayManOn = payload.toInt();
  }
  if (topic == "/fingerprint") {
    payload.toCharArray(fingerprint,20);
  }
}

// Digital clock display of the time
void clockDisplay()
{
  // You can call hour(), minute(), ... at any time
  // Please see Time library examples for details
  #ifdef  BLYNK_DEBUG
    BLYNK_LOG1(">>clockDisplay()");
  #endif
  String currentTime = String(hour()) + ":" +  minute() + ":" + second();
  String currentDate = String(day()) + " " + month() + " " + year();
  BLYNK_LOG4("Current time: ", currentTime, " ", currentDate);

  // Send time to the App
  Blynk.virtualWrite(V8, currentTime);
  // Send date to the App
  Blynk.virtualWrite(V9, currentDate);
  mclient.subscribe("/buero/time");
  mclient.publish("/buero/time", (currentTime +" "+ currentDate));
//  mclient.unsubscribe("/buero/time");
  #ifdef  BLYNK_DEBUG
    Serial.println("<< clockDisplay()");
  #endif
}

BLYNK_CONNECTED() {
  #ifdef  BLYNK_DEBUG
    BLYNK_LOG1(">> BLYNK_CONNECTED");
  #endif
  // Synchronize time on connection
  rtc.begin();
  delay(1000);
  while (year() < 1971) {
    BLYNK_LOG2("Uhr noch nicht auf Stand year = ", year());
    rtc.begin();
  }
    BLYNK_LOG2("Uhr synchronisiert, year = ", year());
  #ifdef  BLYNK_DEBUG
    BLYNK_LOG1("BLYNK_CONNECTED <<");
  #endif
}

void mqttconnect() {
  Serial.print("MQTT: checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nMQTT: connecting...");
  mqconnectet = mclient.connect("buero", "kai", "123");
  n = 0;
  while ((!mqconnectet) && (n < 20)) {
    n++;
    #ifdef BLYNK_DEBUG
      oled.clear(0);  // Clear the display's memory (gets rid of artifacts)
      oled.setCursor(0,0);
      oled.print("MQTT: \nconnecting... \n");
      oled.print(mqconnectet);
      oled.print("\nN: ");
      oled.print(n);
      oled.display();
    #endif
    delay(1000);
    oled.clear(0);  // Clear the display's memory (gets rid of artifacts)
    oled.setCursor(0,0);
    oled.print("Verbinde\nmit MQTT\nVersuch:");
    oled.print(n);
    showLoadingPoint(6);
    oled.display();
    Serial.print(n);
    mqconnectet = mclient.connect("buero", "kai", "123");
    Serial.print(".");
    Serial.print(mqconnectet);
    Serial.print(" ");
    clockDisplay();
  }
  if (!mqconnectet) {
    Serial.println("MQTT Time out!");
    Serial.print(n);
    Serial.print(".");
    Serial.println(mqconnectet);
    delay(1000);
    //mqttconnect();
    setup();
  }
  else {
    Serial.println("connected!");
  }

  mclient.subscribe("/buero/temp");
  mclient.subscribe("/buero/tempMin");
  mclient.subscribe("/buero/tempMax");
  mclient.subscribe("/buero/humidity");
  mclient.subscribe("/buero/humidityMin");
  mclient.subscribe("/buero/humidityMax");
  mclient.subscribe("/buero/time");
  mclient.subscribe("/buero/updateRange");
  mclient.subscribe("/buero/flip");
  mclient.subscribe("/buero/relayTempSwitch");
  mclient.subscribe("/buero/relayManualMode");
  mclient.publish("/buero/relayManualMode", String(0));
  mclient.subscribe("/buero/relayManOn");
  mclient.subscribe("/fingerprint");
  mclient.subscribe("/terrasse/temp");
  // client.unsubscribe("/hello");
}

BLYNK_WRITE(V2)
{
  warteZeit = param.asInt(); // assigning incoming value from pin V2 to a variable
  mclient.publish("/buero/updateRange", String(warteZeit));
  // process received value
}

BLYNK_WRITE(V6)
{
  //Temperatur und Luftfeuchtigkeit auf 100 Setzen erzeugt ein Aktuallisieren der Werte
  temp_old = 100;
  humidity_old = 100;
}

BLYNK_WRITE(V7)
{
  flip = param.asInt(); // assigning incoming value from pin V7 to a variable
  mclient.publish("/buero/flip", String(flip));
  ;// process received value
}

BLYNK_WRITE(V11)
{   //reset Min/Max
  temp_old = 100;
  tempMin = 0;
  tempMax = 0;
  humidity_old = 100;
  humidityMin = 0;
  humidityMax = 0;
}

BLYNK_WRITE(V12)
{
  relayManOn = param.asInt(); // assigning incoming value from pin V12 to a variable
  mclient.publish("/buero/relayManOn", String(relayManOn));
  ;// process received value
}

BLYNK_WRITE(V13)
{
  relayManMode = param.asInt(); // assigning incoming value from pin V13 to a variable
  mclient.publish("/buero/relayManMode", String(relayManMode));
  ;// process received value
}

BLYNK_WRITE(V14)
{
  relayTempSwitch = param.asFloat(); // assigning incoming value from pin V14 to a variable
  mclient.publish("/buero/relayTempSwitch", String(relayTempSwitch));
  ;// process received value
}

bool relaySwitch(bool manMode, bool switchOn, float autoOnTemp, float isTemp) {
  if (!manMode) {
    if ( (isTemp >= autoOnTemp) && (!relayState)) {
      relayState = 1;
      digitalWrite(relayPin, relayState);
      Serial.print(isTemp);
      Serial.print(" >= ");
      Serial.println(autoOnTemp);
      Serial.println("Relay eingeschaltet");
    }
    else if ( (isTemp < (autoOnTemp-1)) && (relayState)) {
      relayState = 0;
      digitalWrite(relayPin, relayState);
      Serial.print(isTemp);
      Serial.print(" < ");
      Serial.println(autoOnTemp);
      Serial.println("Relay ausgeschaltet");
    }
  }
  if (manMode) {
    if ( switchOn && !relayState) {
      relayState = 1;
      digitalWrite(relayPin, relayState);
      Serial.println("Relay manuell eingeschaltet");
    }
    else if ( !switchOn && relayState) {
      relayState = 0;
      digitalWrite(relayPin, relayState);
      Serial.println("Relay manuell ausgeschaltet");
    }
  }
  return relayState;
}
