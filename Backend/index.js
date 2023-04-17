const { json } = require('express');
var express = require('express');
const bodyParser = require("body-parser");
const mqtt = require('mqtt');
var app = express();

// Fill the following values
const macTopic = '';            // MQTT topic for subscribing to mac address list
const statusTopic = '';         // MQTT topic to publish rssi of nearby devices

const mqttHost = '';            // MQTT Broker URL
const mqttPort = '';            // MQTT Port

const serverPort = 4000;        // (Optional) Change the port of Backend Server


// Parse the provided values
const clientId = `mqtt_${Math.random().toString(16).slice(3)}`;
const connectUrl = `mqtt://${mqttHost}:${mqttPort}`;

// connect the mqtt cient to mqtt server
const client = mqtt.connect(connectUrl, {
  clientId,
  clean: false,
  connectTimeout: 4000,
  reconnectPeriod: 1000,
})

// variables to store different values
var macAddress = [];
var names = [];
var rssi = [];
var datetime = [];


app.use(bodyParser.urlencoded({
    extended:true
}));

app.use(bodyParser.json());

// Subsribe to status topic and publish intital list on connection
client.on('connect', () => {
  console.log('Connected');
  client.subscribe(statusTopic, function (err) {
    if (!err) {
      console.log(`Subscribed to ${statusTopic}`);
    }
  });
  client.publish(macTopic, JSON.stringify(macAddress), { qos: 1, retain: true }, (error) => {
    if (error) {
      console.error(error)
    }
  });
})

// Update status of devices if we receive a new message on statusTopic
client.on('message', function (statusTopic, message) {
  // Convert the message buffer to a string
  const jsonObj = JSON.parse(message.toString());
  const now = new Date();
  for (const key in jsonObj) {
    if (jsonObj.hasOwnProperty(key)) {
      const idx = parseInt(key);
      if(idx < macAddress.length){
        rssi[idx] = jsonObj[key];
        datetime[idx] = now.toLocaleString();
        console.log(`${names[idx]}: ${jsonObj[key]}`);
      }
    }
  }
});

// Endpoint for frontend to fetch latest copy of data
app.get('/fetch-mac-data',function(req,res) {
  let payLoad = [];
  let length = Math.min(macAddress.length, names.length, rssi.length, datetime.length);
  for(let i=0;i<length;i++) {
    let element = {
      "id" : i+1,
      "name" : names[i],
      "macAddress" : macAddress[i],
      "lastSeen" : (datetime[i] == null)? "NA" : datetime[i],
      "rssi" : (rssi[i] == null)? "NA" : rssi[i]
    }
    payLoad.push(element);
  }

  res.end(JSON.stringify(payLoad));

})

// endpoint for frontend to post a new mac address
app.post('/add-macAddress', function(req,res) {
    let mac = req.body.macAddress;
    let userName = req.body.name;
    macAddress.push(mac);
    names.push(userName);
    rssi.push(null);
    datetime.push(null);

    client.publish(macTopic, JSON.stringify(macAddress), { qos: 1, retain: true }, (error) => {
      if (error) {
        console.error(error)
      }
    });

});
// Start the Backend server at specified port number
var server = app.listen(serverPort,function(){
  console.log("Server started at Port " + serverPort);
});
