#include <ESP8266mDNS.h>
#include <LEAmDNS.h>
#include <LEAmDNS_Priv.h>
#include <LEAmDNS_lwIPdefs.h>



#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"

#define DHTPIN D2      // Hier den Pin definieren, der mit dem DHT11 verbunden ist
#define DHTTYPE DHT11  // DHT11 definieren (benutzen Sie DHT22, wenn Sie einen DHT22 haben)
//#define ledPin LED_BUILTIN

DHT dht(DHTPIN, DHTTYPE);

// Netzwerkkonfiguration
const char* softAP_ssid = "OLRY";
const char* softAP_pw = "12345678";
const int ledPin = LED_BUILTIN;

// Webserver auf Port 80
ESP8266WebServer server(80);

String modeTitle;  // Globale Variable zum Speichern der aktuellen Überschrift
unsigned long lastPrint = 0;
unsigned long counting = 0;
unsigned long freq = 0;
unsigned long rpm = 0;
bool tachoReady = false;
bool blinker = false;             // Handles the blinking if wife connection is lost
const byte tachoPin = D1;
const byte pwmPin = D5;
int currentPWM = 180;              // Speichert den aktuellen PWM-Wert
unsigned long previousMillis = 0;  // Letzte Zeit, bei der die Uhr aktualisiert wurde
const long interval = 60000;       // Intervall in Millisekunden (60000ms = 1 Minute)
String targetTimeDay = "";         // Time for starting daymode in Format HH:MM
String targetTimeNight = "";           // Zielzeit 2 im Format HH:MM
String scheduledTime = "";
String currentTime = "00:00";  // Globale Variable zum Speichern der aktuellen Zeit
int hour = 0;
int minute = 0;
int currentPWM1 = 0;
int currentPWM2 = 0;
String localIP = "";              // Hold the assigned local IP
String SSID = "";                 // SSID of saved wifi-network
String WifiPW = "";               // Password of saved wifi-network
bool activation = false;
void IRAM_ATTR tacho() {
  counting++;
  tachoReady = true;
}

// Fügen Sie hier die vollständige HTML_content-Funktion ein
String HTML_content() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int currentPWMPercent1 = map(currentPWM1, 0, 255, 0, 100);  // Map 0-255 to 0-100% für PWM1
  int currentPWMPercent2 = map(currentPWM2, 0, 255, 0, 100);  // Map 0-255 to 0-100% für PWM2
  int currentPWMPercent = map(currentPWM, 0, 255, 0, 100);    // Map 0-255 to 0-100% für aktuelles PWM
  String html =
    "<html>"
    "<head>"

    // Styles for webpage
    "<style>"
    "body {font-size: 2vw; text-align: center; background-color: #e6e7ee; color: #333; margin: 0; padding: 0;}"
    "input[type=range], input[type=submit], button, input[type=time], input[type=checkbox] {margin: 20px 0;}"
    "input[type=range] {"
    "width: calc(100% - 40px); height: 50px; -webkit-appearance: none; background: #d3d3d3;"
    "outline: none; opacity: 0.7; transition: opacity .2s;"
    "}"
    "input[type=range]::-webkit-slider-thumb {"
    "width: 75px; height: 75px; border-radius: 50%; background: #4CAF50; cursor: pointer; -webkit-appearance: none;"
    "}"
    "input[type=submit], button {"
    "width: 40%; height: 200px; font-size: 200%; background-color: #4CAF50; color: white;"
    "border: none; border-radius: 5px; cursor: pointer; margin-top: 50px; display: inline-block; margin-left: 5%; margin-right: 5%;" /* Zentrieren des Buttons */
    "}"
    "input[type=submit]:active, button:active {background-color: #367c39;}"
    "input[type=time], input[type=checkbox] {"
    "font-size: 200%; padding: 20px; border-radius: 5px;"
    "}"
    ".value {font-size: 2em; margin: 20px 0;}"
    ".value span {font-weight: bold; color: #4CAF50;}"
    "#currentTime {font-size: 1.5em; margin-top: 20px;}"
    "#modeTitle {font-size: 2em; color: #4CAF50; margin-bottom: 20px;}"
    ".modeButton {"
    "margin: 0 10px; height: 150px;" /* Höhe der unteren drei Buttons */
    "}"
    "#currentMode {"
    "margin-top: 50px;" /* Abstand des untersten Buttons nach oben erhöhen */
    "}"
    "#scheduleOptions {"
    "text-align: center; margin-top: 30px;" /* Mehr Platz zwischen Uhrzeit und den nächsten zwei Buttons */
    "}"
    "#activateLabel {"
    "display: block;"
    "font-size: 10vw;"
    "margin-top: 30px;"
    "}"
    "#activationSlider {"
    "display: block;"
    "margin: 20px auto;"
    "}"
    "</style>"
    "</head>"

    // Body content
    "<body onload='setCurrentMode()'>" /* onload Ereignis hinzugefügt */
    "<div id='modeTitle'>Aktuell</div>"
    "<h1>Arduino PWM Control</h1>"
    "<div class='value'>RPM: <span id='rpmValue'>"
    + String(rpm) + "</span></div>"
                    "<input type='range' min='0' max='100' value='"
    + String(currentPWMPercent) + "' onchange='updatePWMValue(this.value)' id='pwmSlider'>"
                                  "<div class='value'>Speed: <span id='pwmValue'>"
    + String(currentPWMPercent) + "%</span></div>"
                                  "<div id='scheduleOptions'>"
                                  "<input type='time' id='scheduledTime'>"
                                  "<label for='activationSlider' id='activateLabel'>Activate</label>"
                                  "<input type='checkbox' id='activationSlider'>"
                                  "</div>"
                                  "<input type='submit' value='Set PWM' onclick='setPWM()'>"
                                  "<div id='currentTime'></div>"
                                  "<div style='text-align: center; margin-top: 20px;'>"
                                  "<button id='dayMode' class='modeButton' onclick='setDayMode()'>Day mode</button>"
                                  "<button id='nightMode' class='modeButton' onclick='setNightMode()'>Night mode</button>"
                                  "</div>"
                                  "<button id='currentMode' class='modeButton' onclick='setCurrentMode()'>Aktuell</button>"
                                  "<div class='value'>Temperature: <span style='color:green;'>"
    + String(t, 1) + "°C</span></div>"
                     "<div class='value'>Air humitidy: <span style='color:green;'>"
    + String(h, 0) + "%</span></div>"
                     "<div class='value'>Night Mode Start: <span>"
    + targetTimeDay + "</span> / PWM: <span>" + String(currentPWMPercent1) + "%</span></div>"
                                                                           "<div class='value'>Day Mode Start: <span>"
    + targetTimeNight + "</span> / PWM: <span>" + String(currentPWMPercent2) + "%</span></div>"

    "<div>Local IP: " + String(localIP) + "</div>"
    "<br>"
    "<div>SSID: <input id='SSID'></div>"
    "<div>Wifi Password: <input type='password' id='WifiPW'></div>"
    "<div><input type='submit' value='Save Wifi and Connect' onclick='setWifi()'></div>"

    // Scripts
    "<script>"
    "document.getElementById('modeTitle').innerText = 'Current';" /* Starte immer im Aktuell Modus */
    "function updatePWMValue(value) {"
      "document.getElementById('pwmValue').innerText = value + '%';"
    "}"
    "function setDayMode() {"
      "document.getElementById('modeTitle').innerText = 'Day Mode';"
      "document.getElementById('scheduleOptions').style.display = 'block';"
      "document.body.style.backgroundColor = 'white';"
      "document.body.style.color = 'black';"
    "}"
    "function setNightMode() {"
      "document.getElementById('modeTitle').innerText = 'Night Mode';"
      "document.getElementById('scheduleOptions').style.display = 'block';" /* Gleiche Optionen wie im Tag Modus */
      "document.body.style.backgroundColor = 'black';"
      "document.body.style.color = 'white';"
    "}"
    "function setCurrentMode() {"
      "document.getElementById('modeTitle').innerText = 'Current';"
      "document.getElementById('scheduleOptions').style.display = 'none';"
      "document.body.style.backgroundColor = '#e6e7ee';"
      "document.body.style.color = 'black';"
    "}"

    "function setWifi() {"
      
      "var xhr = new XMLHttpRequest();"
      "var SSID = document.getElementById('SSID').value;"
      "var WifiPW = document.getElementById('WifiPW').value;"
      "var queryString = '?SSID=' + SSID + '&WifiPW=' + WifiPW;"
      
      "xhr.open('GET', '/wifi' + queryString, true);"
      "xhr.onreadystatechange = function() {"
        "if (xhr.readyState == 4 && xhr.status == 200) {"
          "setTimeout(function() { window.location.reload(); }, 3000);" /* Aktualisiere die Webseite nach 3 Sekunden */
        "}"
      "};"
      "xhr.send();"
    "}"

    "function setPWM() {"
      "var xhr = new XMLHttpRequest();"
      "var pwmValue = document.getElementById('pwmSlider').value;"
      "var scheduledTime = document.getElementById('scheduleOptions').style.display !== 'none' ? document.getElementById('scheduledTime').value : '';"
      "var activation = document.getElementById('scheduleOptions').style.display !== 'none' ? document.getElementById('activationSlider').checked : '';"
      "var currentTime = new Date().toISOString();"                     /* Erfasst die aktuelle Zeit im ISO-Format */
      "var modeTitle = document.getElementById('modeTitle').innerText;" /* Erfasst die aktuelle Überschrift */
      "var queryString = '?value=' + pwmValue + '&time=' + scheduledTime + '&activation=' + activation + '&currentTime=' + encodeURIComponent(currentTime) + '&modeTitle=' + encodeURIComponent(modeTitle);"
      "xhr.open('GET', '/pwm' + queryString, true);"
      "xhr.onreadystatechange = function() {"
        "if (xhr.readyState == 4 && xhr.status == 200) {"
          "setTimeout(function() { window.location.reload(); }, 3000);" /* Aktualisiere die Webseite nach 3 Sekunden */
        "}"
      "};"
      "xhr.send();"
    "}"

    "setInterval(function() {"
      "let now = new Date();"
      "let hours = now.getHours();"
      "let minutes = now.getMinutes();"
      "document.getElementById('currentTime').innerHTML = (hours < 10 ? '0' : '') + hours + ':' + (minutes < 10 ? '0' : '') + minutes;"
    "}, 1000);"
    "</script>"

    "</body>"
    "</html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", HTML_content());
}
void handlePWM() {
  if (server.hasArg("value")) {
    String pwmValue = server.arg("value");
    currentPWM = map(pwmValue.toInt(), 0, 100, 0, 255);  // Map 0-100% to 0-255
    analogWrite(pwmPin, currentPWM);
    if (server.hasArg("time")) {
      String scheduledTime = server.arg("time");
      Serial.print("Geplante Zeit: ");
      Serial.println(scheduledTime);
    }
    if (server.hasArg("activation")) {
      bool activation = server.arg("activation") == "true";
      Serial.print("Aktivierung: ");
      Serial.println(activation ? "Aktiviert" : "Deaktiviert");
    }
    if (server.hasArg("currentTime")) {
      currentTime = server.arg("currentTime");
      Serial.print("Vom Browser empfangene aktuelle Zeit: ");
      Serial.println(currentTime);
    }
    if (server.hasArg("modeTitle")) {
      modeTitle =
        server.arg("modeTitle");
      Serial.print("Aktueller Modus: ");
      Serial.println(modeTitle);
    }
  }
  server.send(200, "text/plain", "");  // Sendet eine leere Antwort zurück
}

void connectToWifi() {
  WiFi.begin(SSID, WifiPW);
  Serial.println("Connecting");
  delay(500);
  int retry = 0;   // counter for retries
  // try to connect at least 10 times
  while (WiFi.status() != WL_CONNECTED && retry < 20)
  {
    digitalWrite(ledPin, HIGH);
    delay(500);
    Serial.printf("Connection status: %d\n", WiFi.status());
    digitalWrite(ledPin, LOW);
    delay(500);
    retry++;
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  digitalWrite(ledPin, LOW);
}

// Handle savings of Wifi info
void saveWifiInfo() {
  if (server.hasArg("SSID") && server.hasArg("WifiPW")){
    Serial.println(server.arg("SSID"));
    Serial.println(server.arg("WifiPW"));
    // Write SSID to EEPROM
    for (int i = 0; i < server.arg("SSID").length(); i++) {
      EEPROM.write(40 + i, server.arg("SSID")[i]);
    }
    EEPROM.write(40 + server.arg("SSID").length() + 1, '\0');   // add zero-terminator to end of string, to know end when we read it
    //EEPROM.commit();  // Änderungen im EEPROM speichern
    // Write WifiPW to EEPROM
    for (int i = 0; i < server.arg("WifiPW").length(); i++) {
      EEPROM.write(70 + i, server.arg("WifiPW")[i]);
    }
    EEPROM.write(70 + server.arg("WifiPW").length() + 1, '\0');   // add zero-terminator to end of string, to know end when we read it
    EEPROM.commit();  // Änderungen im EEPROM speichern
  }
  else {
    Serial.println("Error: No SSID / WifiPW transmitted");
  }
  server.send(200, "text/plain", "");  // Send back empty response
}

void setup() {
  digitalWrite(ledPin, HIGH);
  Serial.begin(115200);
  delay(1000);  // wait for proper connection for serial monitor
  pinMode(tachoPin, INPUT_PULLUP);
  pinMode(pwmPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(tachoPin), tacho, RISING);
  pinMode(ledPin, OUTPUT);

  // load EEPROM to cache
  EEPROM.begin(512);
  // Read target times for Day and Night
  char readTargetTimeDay[6];  // 5 für die maximale Stringlänge + 1 für das Nullzeichen
  char readTargetTimeNight[6];  // 5 für die maximale Stringlänge + 1 für das Nullzeichen
  for (int i = 0; i < 5; i++) {
    readTargetTimeDay[i] = EEPROM.read(0 + i);
    readTargetTimeNight[i] = EEPROM.read(20 + i);
  }
  readTargetTimeDay[5] = '\0';  // Füge das Nullzeichen am Ende des Strings hinzu
  readTargetTimeNight[5] = '\0';  // Füge das Nullzeichen am Ende des Strings hinzu
  targetTimeDay = String(readTargetTimeDay);
  targetTimeNight = String(readTargetTimeNight);
  
  // read PWM for Day and Night Mode
  EEPROM.get(10, currentPWM1); // Day Mode
  EEPROM.get(30, currentPWM2); // Night Mode

  // read SSID and WifiPW strings
  //delay(5000);
  Serial.println("before SSID");
  //delay(5000);
  SSID = readEEPROMString(40);
  Serial.println("adter SSID");
  //delay(5000);
  //WifiPW = readEEPROMString(70);
  //delay(5000);
  
  // Set mode to both AP and station
  WiFi.mode(WIFI_AP_STA);
  // Start the soft AP
  WiFi.softAP(softAP_ssid, softAP_pw);
 
  // Connect to WIFI
  connectToWifi();

  localIP = IpAddress2String(WiFi.localIP());
  Serial.println(localIP);
  Serial.print("Connected, AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print(SSID);
  Serial.print(WifiPW);


  // Webserver Routen definieren
  server.on("/", handleRoot);
  server.on("/pwm", handlePWM);
  server.on("/wifi", saveWifiInfo);

  //start mDNS reponder
  if (!MDNS.begin("esp8266"))   {  Serial.println("Error setting up MDNS responder!");  }
  else                          {  Serial.println("mDNS responder started");  }

  // Start webserver
  server.begin();
  // Start temp and humidity sensor
  dht.begin();
  // load EEPROM to cache

}

String readEEPROMString(char address)
{
  char data[30];              //Max 30 Bytes
  int i=0;                    // Iterator
  unsigned char c;            // Initialize char for holding read char
  c=EEPROM.read(address);     // Read first char from EEPROM
  while(c != '\0' && i<30)  // Read until null character
  {
    c=EEPROM.read(address+i);  
    data[i]=c;
    i++;
  }
  //data[i-1]='\0';             // Add zero-terminator at end
  String str = String(data);
  int l = str.length();
  str = str.substring(0,l-1);
  Serial.print("Readstring: ");
  Serial.println(str);
  delay(500);
  return str;
}

void loop() {
  MDNS.update();
  unsigned long currentMillis = millis();
  // check for wifi connection, and blink if no connection
  //if (currentBlinkMillis - previousBlinkMillis >= blinkInterval || currentBlinkMillis < previousBlinkMillis) {
      // check for wifi connection and blink if no connection
    //if (WiFi.status() != WL_CONNECTED){
      // use blinker here
      //digitalWrite(ledPin, HIGH);
      //}

  
  // Überprüfe, ob das Intervall abgelaufen ist unter Berücksichtigung des Überlaufs von millis()
  if (currentMillis - previousMillis >= interval || currentMillis < previousMillis) {
    previousMillis = currentMillis;  // Aktualisiere die letzte Zeit
    minute++;                        // Erhöhe die Minuten
    if (minute >= 60) {
      minute = 0;  // Zurücksetzen der Minuten
      hour++;      // Erhöhe die Stunde
      if (hour >= 24) {
        hour = 0;  // Zurücksetzen der Stunden
      }
    }
    // Aktuelle Zeit als String formatieren
    currentTime = (hour < 10 ? "0" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute);

    //float h = dht.readHumidity();
    //float t = dht.readTemperature();
    // Aktuelle Zeit anzeigen

    Serial.println(currentTime);
    Serial.print("Day Mode target time: ");
    Serial.println(targetTimeDay);
    Serial.print("Night Mode target time: ");
    Serial.println(targetTimeNight);

    // Überprüfen, ob eine der Zielzeiten erreicht ist
    if (currentTime == targetTimeDay) {
      onTargetTimeDayReached();  // Funktion für Zielzeit 1
    }
    if (currentTime == targetTimeNight) {
      ontargetTimeNightReached();  // Funktion für Zielzeit 2
    }
  }
  unsigned long now = millis();
  if (now - lastPrint >= 1000) {
    freq = counting;
    rpm = freq * 60 / 2;  // Hier müsste Ihre Logik zur Umrechnung in RPM stehen
    if (server.hasArg("time")) {
      scheduledTime = server.arg("time");
      Serial.print("Geplante Zeit: ");
      Serial.println(scheduledTime);
      Serial.print("RPM: ");
      Serial.println(rpm);

      // Ausgeben der übermittelten Parameter
      Serial.print("Geschwindigkeit (PWM): ");
      Serial.println(currentPWM);
    }
    if (server.hasArg("activation")) {
      activation = server.arg("activation") == "true";
      Serial.print("Aktivierung: ");
      Serial.println(activation ? "Aktiviert" : "Deaktiviert");
    }
    if (server.hasArg("currentTime")) {
      currentTime = server.arg("currentTime");
      Serial.print("Vom Browser empfangene aktuelle Zeit: ");
      Serial.println(currentTime);
      hour = 1 + currentTime.substring(11, 13).toInt();  // Stunden extrahieren (position 11 bis 12)
      minute = currentTime.substring(14, 16).toInt();    // Minuten extrahieren (position 14 bis 15)
      currentTime = (hour < 10 ? "0" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute);
      Serial.println(currentTime);
    }
    if (server.hasArg("modeTitle")) {
      modeTitle = server.arg("modeTitle");
      Serial.print("Aktueller Modus: ");
      Serial.println(modeTitle);
    }
    if (activation == true && modeTitle == "Day Mode") {
      targetTimeDay = scheduledTime;
      currentPWM1 = currentPWM;
      // Schreibe die Strings ins EEPROM
      for (int i = 0; i < targetTimeDay.length(); i++) {
        EEPROM.write(0 + i, targetTimeDay[i]);
      }
      EEPROM.put(10, currentPWM1);
      EEPROM.commit();  // Änderungen im EEPROM speichern
      modeTitle = "";
      activation = false;
      // Aktuelle Zeit anzeigen
      Serial.println(currentTime);
      Serial.println(targetTimeDay);
      Serial.println(targetTimeNight);
      Serial.println(minute);
    }
    if (activation == true && modeTitle == "Night Mode") {
      targetTimeNight = scheduledTime;
      currentPWM2 = currentPWM;
      // Schreibe die Strings ins EEPROM
      for (int i = 0; i < targetTimeNight.length(); i++) {
        EEPROM.write(20 + i, targetTimeNight[i]);
      }
      EEPROM.put(30, currentPWM2);
      EEPROM.commit();  // Änderungen im EEPROM speichern
      modeTitle = "";
      activation = false;

      // Aktuelle Zeit anzeigen
      Serial.println(currentTime);
      Serial.println(targetTimeDay);
      Serial.println(targetTimeNight);
    }

    lastPrint = now;
    counting = 0;
  }

  // Webserver im Hintergrund laufen lassen
  server.handleClient();
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +
           String(ipAddress[1]) + String(".") +
           String(ipAddress[2]) + String(".") +
           String(ipAddress[3]);
}

void onTargetTimeDayReached() {
  Serial.println("Time for Day Mode reached!");
  analogWrite(pwmPin, currentPWM1);
  // Füge hier den Code ein, der ausgeführt werden soll, wenn die Zielzeit 1 erreicht ist
}

void ontargetTimeNightReached() {
  Serial.println("Time for Night Mode reached!!");
  analogWrite(pwmPin, currentPWM2);
  // Füge hier den Code ein, der ausgeführt werden soll, wenn die Zielzeit 2 erreicht ist
}
