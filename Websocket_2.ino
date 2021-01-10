#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#error "Board not found"
#endif

#include <WebSocketsServer.h>

#include <ArduinoJson.h>



#define LED1 13
#define LED2 17


char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<script>
var connection = new WebSocket('ws://'+location.hostname+':81/');
fetch("/status").then(response =>response.json()).then(data => set_state(data));
var button_1_status = 0;
var button_2_status = 0;

setInterval(() => {
fetch("/status").then(response =>response.json()).then(data => set_state(data)); 
},1000);

function set_state(states)
{
  document.getElementById("button_1").disabled = states.LED1 == 0?"":"true";
  document.getElementById("button_2").disabled = states.LED1 == 0?"true":"";
  document.getElementById("button_3").disabled = states.LED2 == 0?"":"true";
  document.getElementById("button_4").disabled = states.LED2 == 0?"true":"";
}

function button_1_on()
{
   button_1_status = 1; 
  console.log("LED 1 is ON");
  send_data();
}
function button_1_off()
{
  button_1_status = 0;
console.log("LED 1 is OFF");
send_data();
}
function button_2_on()
{
   button_2_status = 1; 
  console.log("LED 2 is ON");
  send_data();
}
function button_2_off()
{
  button_2_status = 0;
console.log("LED 2 is OFF");
send_data();
}
function send_data()
{
  var full_data = '{"LED1" :'+button_1_status+',"LED2":'+button_2_status+'}';
  connection.send(full_data);
}
</script>
<body>
<center>
<h1 style="color:Tomato;"><b>Mark's Home Automation</b></h1>
<h3> LED 1 </h3>
<button id="button_1" onclick="button_1_on()" >On</button> <button id="button_2" onclick="button_1_off()" >Off</button>
<h3> LED 2 </h3>
<button id="button_3" onclick="button_2_on()">On</button>  <button id="button_4" onclick="button_2_off()">Off</button>
</center>
</body>
</html>

)=====";

#include <ESPAsyncWebServer.h>

static int LED1_status =0;
static int LED2_status =0;

AsyncWebServer server(80); // server port 80
WebSocketsServer websockets(81); // server port 81

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Page Not found");
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) 
  {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = websockets.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        char pointer[80];
        sprintf(pointer,  "Connected from server, LED1: %d, LED2: %d", LED1_status, LED2_status);
        websockets.sendTXT(num, pointer);
        Serial.printf(pointer);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      String message = String((char*)( payload));
      Serial.println(message);

      
     DynamicJsonDocument doc(200);
    // deserialize the data
    DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  LED1_status = doc["LED1"];
  LED2_status = doc["LED2"];
  digitalWrite(LED1,LED1_status);
  digitalWrite(LED2,LED2_status);




  }
}

void setup(void)
{
  
  Serial.begin(115200);
  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  
  WiFi.softAP("mark", "marksanne");
  Serial.println("softap");
  Serial.println("");
  Serial.println(WiFi.softAPIP());


  if (MDNS.begin("ESP")) { //esp.local/
    Serial.println("MDNS responder started");
  }



  server.on("/", [](AsyncWebServerRequest * request)
  { 
  request->send_P(200, "text/html", webpage);

  });

   server.on("/status", HTTP_GET, [](AsyncWebServerRequest * request)
  { 
  char json[80];
  sprintf(json, "{\"LED1\":%d, \"LED2\":%d}",LED1_status,LED2_status);
  request->send(200, "text/json", json);
  
  });

  server.onNotFound(notFound);

  server.begin();  // it will start webserver
  websockets.begin();
  websockets.onEvent(webSocketEvent);

}


void loop(void)
{
 websockets.loop();
}
