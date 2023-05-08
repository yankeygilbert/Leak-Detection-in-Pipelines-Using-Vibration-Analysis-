/*------------------------------------------------------------------------------
  Authors: GILBERT BOAKYE YANKEY AND THOMAS KANKAM
  Platforms: ESP8266
  Language: C++/Arduino

  ------------------------------------------------------------------------------*/
// Libraries imported for use
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Ticker.h>
#include <SoftwareSerial.h>

SoftwareSerial nodemcu(D6,D5); //tx, rx
Ticker timer;
bool get_data = false;
int dt;

// Connecting to the Internet
char  ssid [] = "";
char pass [] = "";

// Running a web server
ESP8266WebServer server;

// Adding a websocket to the server
WebSocketsServer webSocket = WebSocketsServer(81);

// Serving a web page (from flash memory)
// formatted as a string literal!
char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta http-equiv="X-UA-Compatible" content="IE=edge">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">

  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  
  <script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.5.0/Chart.min.js'></script>

  <title>Leak || Detection</title>

  <style>
  html {
    font-family: Arial;
    display: inline-block;
    margin: 0px auto;
    text-align: center;
  }

  h2 {
    font-size: 3.0rem;
  }

  p {
    font-size: 3.0rem;
  }

  .dht-labels {
    font-size: 1.5rem;
    vertical-align: middle;
    padding-bottom: 15px;
  }
</style>

</head>

<body onload="javascript:init()">

  <h2>Leak Detection System</h2>

  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="dht-labels">Status</span>
    <span id="leak" class="dht-labels">**</span>
  </p>

  <!-- Adding a slider for controlling data rate -->
<div>
  <input type="range" min="1" max="10" value="5" id="dataRateSlider" oninput="sendDataRate()" />
  <label for="dataRateSlider" id="dataRateLabel">Rate: 0.2Hz</label>
</div>

<hr />

<div>
  <canvas id="line-chart" width="800" height="225"></canvas>
</div>

<script>
  let webSocket, dataPlot
  let maxDataPoints = 20
  let nominalValue = 30

  function removeData(){
    dataPlot.data.labels.shift();
    dataPlot.data.datasets[0].data.shift();
  }

  function addData(label, data) {
    if(dataPlot.data.labels.length > maxDataPoints) removeData();
    dataPlot.data.labels.push(label);
    dataPlot.data.datasets[0].data.push(data);
    dataPlot.update();
  }

  function init() {
    webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
    dataPlot = new Chart(document.getElementById("line-chart"), {
      type: 'line',
      data: {
        labels: [],
 []       datasets: [{
          data: [],
          label: " Frequency (Hz)",
          borderColor: "#3e95cd",
          fill: false
        }]
      }
    });
    webSocket.onmessage = function(event) {
      var data = JSON.parse(event.data);
      var today = new Date();
      var t = today.getHours() + ":" + today.getMinutes() + ":" + today.getSeconds();
      
       var d_arr = [];

       d_arr.push(data); 

       if (d_arr.length === 10)
       {
          const average = d_arr.reduce((a, b) => a + b, 0) / d_arr.length ;

          if (average > nominalValue) {
          document.getElementById("leak").innerHTML = "Status: leak detected";
          }
          elseif (average < nominalValue)
          {
            document.getElementById("leak").innerHTML = "Status: no leakage";
          }
       }

      else{
        document.getElementById("leak").innerHTML = "Status: no leakage";
      }
      addData(t, data.value);
      console.log(data);
    }
  }

  // Slider Function
  function sendDataRate(){
    var dataRate = document.getElementById("dataRateSlider").value;
    webSocket.send(dataRate);
    dataRate = 1.0/dataRate;
    document.getElementById("dataRateLabel").innerHTML = "Rate: " + dataRate.toFixed(2) + "Hz";
  }
</script>

</body>

</html>
)=====";



 void  connectWifi()
  {
    Serial.println("Connecting to wifi network ...");
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        WiFi.begin(ssid , pass);
        delay(1000);
        Serial.println (WiFi.status());   
         }
  }
  
void getData() {
  get_data = true;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  // Do something with the data from the client
  if(type == WStype_TEXT){
    float dataRate = (float) atof((const char *) &payload[0]);
    timer.detach();
    timer.attach(dataRate, getData);
  }
}


void setup() {
  
  WiFi.begin(ssid, pass);
  Serial.begin(9600);
  nodemcu.begin(115200);
  while(WiFi.status()!=WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/",[](){
    server.send_P(200, "text/html", webpage);
  });


  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  timer.attach(2, getData);
}

void loop() {
     
  while (nodemcu.available()>0)
  {
    if (WiFi.status() != WL_CONNECTED) {
      connectWifi();
    }
  webSocket.loop();
  server.handleClient();
   //Serial.println(nodemcu.read());
   dt= nodemcu.read();

  if(get_data){
     Serial.println(dt);
    String json = "{\"value\":";
    json += dt  ;
    json += "}";
    webSocket.broadcastTXT(json.c_str(), json.length());
    get_data = false;
   
  }
}
   }
