#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#define LED1 13
#define LED2 12

const char webpage[] PROGMEM = R"=====(

<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
</head>
<body>
<style>
  .container {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    height: 100vh;
  }

  .led-container {
    display: flex;
    flex-direction: row;
    align-items: center;
    margin-bottom: 20px;
  }

  .led {
    width: 50px;
    height: 50px;
    border-radius: 50%;
    margin-right: 20px;
    box-shadow: 0 0 10px rgba(0, 0, 0, 0.3);
  }

  .led.on {
    background-color: yellow;
    box-shadow: 0 0 20px yellow;
  }

  .led.off {
    background-color: grey;
  }

  .toggle-button {
    padding: 10px 20px;
    font-size: 16px;
    background-color: #4CAF50;
    color: white;
    border: none;
    border-radius: 4px;
    cursor: pointer;
  }
</style>

<div class="container">
  <h1>IoT with Nagesh Automation</h1>
  <div class="led-container">
    <div id="led1" class="led off"></div>
    <button class="toggle-button" onclick="toggleLED(1)">Toggle LED 1</button>
  </div>
  <div class="led-container">
    <div id="led2" class="led off"></div>
    <button class="toggle-button" onclick="toggleLED(2)">Toggle LED 2</button>
  </div>
</div>

<script>
  var connection = new WebSocket('ws://' + location.hostname + ':81/');

 function toggleLED(led) {
  const ledElement = document.getElementById(`led${led}`);
  const isOn = ledElement.classList.contains('on');

  const data = { [`LED${led}`]: isOn ? 0 : 1 };
  connection.send(JSON.stringify(data));

  ledElement.classList.toggle('on');
  ledElement.classList.toggle('off');

  console.log(`LED ${led} is ${isOn ? 'OFF' : 'ON'}`);
  console.log(data);
}
</script>
</body>
</html>
)=====";
AsyncWebServer server(80);
WebSocketsServer websockets(81);

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      // When a WebSocket client disconnects
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
      // When a new WebSocket connection is established
      IPAddress ip = websockets.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
      websockets.sendTXT(num, "Connected from server");
      break;
    }
    case WStype_TEXT: {
      // When WebSocket server receives a text message
      String message = String((char*)payload);
      
      // Parse the JSON message
      DynamicJsonDocument doc(200);
      DeserializationError error = deserializeJson(doc, message);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      
      // Check if the message contains LED1 status
      if (doc.containsKey("LED1")) {
          int status = doc["LED1"];
          digitalWrite(LED1, status);
      }
      
      // Check if the message contains LED2 status
      if (doc.containsKey("LED2")) {
          int status = doc["LED2"];
          digitalWrite(LED2, status);
      }
      break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  // Start the SoftAP mode
  WiFi.softAP("IoTwithNagesh", "");
  Serial.println("SoftAP started");
  Serial.println(WiFi.softAPIP());

  // Start the mDNS responder
  if (MDNS.begin("ESP")) {
    Serial.println("MDNS responder started");
  }

  // Handle the root URL
  server.on("/", [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", webpage);
  });

  // Start the web server
  server.begin();

  // Start the WebSocket server
  websockets.begin();

  // Register the WebSocket event handler
  websockets.onEvent(handleWebSocketEvent);
}

void loop() {
  // Keep the WebSocket server running
  websockets.loop();
}
