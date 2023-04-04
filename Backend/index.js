const { json } = require('express');
var express = require('express');
const bodyParser = require("body-parser");
const mqtt = require('mqtt');
var app = express();
const port1 = 4000;

const topic = 'testiitr';

const host = 'broker.hivemq.com';
const port = '1883';
const clientId = `mqtt_${Math.random().toString(16).slice(3)}`;

const connectUrl = `mqtt://${host}:${port}`;

const client = mqtt.connect(connectUrl, {
  clientId,
  clean: false,
  connectTimeout: 4000,
  reconnectPeriod: 1000,
})

var macAddress = [];
var names = [];
var rssi = [];
var datetime = [];


app.use(bodyParser.urlencoded({
    extended:true
}));

app.use(bodyParser.json());
client.on('connect', () => {
  console.log('Connected');
  client.subscribe(topic+"/status", function (err) {
    if (!err) {
      console.log(`Subscribed to ${topic}/status`);
    }
  });
})

client.on('message', function (topic, message) {
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

app.post('/add-macAddress', function(req,res) {
    let mac = req.body.macAddress;
    let userName = req.body.name;
    macAddress.push(mac);
    names.push(userName);
    rssi.push(null);
    datetime.push(null);

    client.publish(topic+"/mac", JSON.stringify(macAddress), { qos: 1, retain: true }, (error) => {
      if (error) {
        console.error(error)
      }
    });

});

// client.publish(topic+"/mac", JSON.stringify(macAddress), { qos: 1, retain: true }, (error) => {
//   if (error) {
//     console.error(error)
//   }
// });

var server = app.listen(port1,function(){
  console.log("Server started at port " + port1);
})
