#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>  // Use this, not the blocking one
#include <ESP8266mDNS.h>


#define RELAY_NO true
#define NUM_RELAYS 4

int relayGPIOs[NUM_RELAYS] = {5, 4, 14, 12}; // D1, D2, D5, D6

const char* PARAM_INPUT_1 = "relay";
const char* PARAM_INPUT_2 = "state";

AsyncWebServer server(80);
DNSServer dns;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px}
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>WORK PLACE</h2>
  %BUTTONPLACEHOLD
<script>
function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){
    xhr.open("GET", "/update?relay="+element.id+"&state=1", true);
  } else {
    xhr.open("GET", "/update?relay="+element.id+"&state=0", true);
  }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    for(int i=1; i<=NUM_RELAYS; i++){
      String relayStateValue = relayState(i);
      buttons += "<h4>Relay #" + String(i) + " - GPIO " + relayGPIOs[i-1] + "</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(i) + "\" "+ relayStateValue + "><span class=\"slider\"></span></label>";
    }
    return buttons;
  }
  return String();
}

String relayState(int numRelay){
  if(RELAY_NO){
    return digitalRead(relayGPIOs[numRelay-1]) ? "" : "checked";
  } else {
    return digitalRead(relayGPIOs[numRelay-1]) ? "checked" : "";
  }
}

void setup(){
  Serial.begin(115200);
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,LOW);

  for(int i=0; i<NUM_RELAYS; i++){
    pinMode(relayGPIOs[i], OUTPUT);
    digitalWrite(relayGPIOs[i], RELAY_NO ? HIGH : LOW);
  }

  // Async WiFiManager setup
  AsyncWiFiManager wifiManager(&server, &dns);
  wifiManager.autoConnect("RelayControllerAP");

  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP()); 

      if (MDNS.begin("relaybox")) {
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error starting mDNS");
  }



  // Web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      int relayNum = request->getParam(PARAM_INPUT_1)->value().toInt() - 1;
      int state = request->getParam(PARAM_INPUT_2)->value().toInt();
      if(RELAY_NO){
        digitalWrite(relayGPIOs[relayNum], !state);
      } else {
        digitalWrite(relayGPIOs[relayNum], state);
      }
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Invalid Request");
    }
  });

  server.begin();
}

void loop() {
}