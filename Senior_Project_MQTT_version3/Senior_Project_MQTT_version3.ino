// All libraries needed to run project
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>


// OPERATIONAL NOTE: Whenever adding new topics/components, you must update the reconnect() function and the callback function
// This is for pin-controlled assets only, info being published from the microcontroller to the dashboard will utilize the client.publish()
// function in the void loop at the bottom


//NOTES:
// Vin Pin/Rail on Node MCU valve runs about 4 volts

// The variables used to connect the ESP8266 to the router; ssid is the wifi channel name
const char* ssid = "**********";
const char* password = "**********";
// this is the Raspberry Pi's IP Address to connect to the MQTT broker
const char* mqtt_server = "***.***.*.**";


// Initializes the espClient. Change the espClient name if there are multiple ESPs running on the same network
// DHTesp initializes the dht-object that we'll call in this script
WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dht;


// ALL PIN ASSIGNMENTS GO HERE!!!!!!!!!
// 9 pins available in total, D0 -> D8

// old lamp pin = GPIO 4 = D2
// old Tank Check Pin = GPIO 12 = D6

//DO NOT, I REPEAT, NOT, USE D3 OR D4 ON BOARD, ALWAYS HIGH AT 3.3 V

// DHT Sensor - GPIO 5 = D1 on ESP-12E NodeMCU board
// Moisture Sensor 1(Bern) - GPIO 4 = D2 on ESP-12E NodeMCU board
// Pump Control Pin - GPIO 14 = D5 on ESP-12E NodeMCU board
// Moisture Sensor 2(Bell) - GPIO 12 = D6 on ESP-12E NodeMCU board
// Valve_1_Control - GPIO 13 = D7 on ESP-12E NodeMCU board
// Valve_2_Control - GPIO 15 = D8 on ESP-12E NodeMCU board
// Tank Check Pin - Analog Pin 0 = A0 on ESP-12E NodeMCU board


const int DHTPin = 5;
const int Bern_Pin = 4;
const int Bell_Pin = 12;
const int Pump_Control_Pin = 14;
const int Tank_Check_Pin = A0;
const int Valve_1_Control = 13;
const int Valve_2_Control = 15;

// Timers auxiliary variables
unsigned long now = 0;

//all other variables preallocated because we're good programmers

float humidity_value;
float celcius_temp;
float fahren_temp;
int bern_moist_value;
int bell_moist_value;
int tank_analog_read;
int tank_full = 1;
int pump_state_value = 0;
int period;
int bern_valve_open = 0;
int bell_valve_open = 0;
int network_connected = 0;

bool bernard_wait = false;
bool bellsprout_wait = false;
bool master_run = false;

static char temperatureExport[7];
static char humidityExport[7];
static char Bernard_MoistureExport[7];
static char Bellsprout_MoistureExport[7];
static char Tank_Level_Export[7];
static char Pump_StateExport[7];
static char Valve_1_Export[7];
static char Valve_2_Export[7];
static char Network_Export[7];

// This function connects the ESP8266 to the router
void setup_wifi() {
  
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());

  network_connected = 1;
  dtostrf(network_connected, 6, 2, Network_Export);
  client.publish("Garden/Master/Connect", Network_Export);

}



// This function is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
  
  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
//  if(topic=="Garden/Lamp"){
//      if(messageTemp == "on"){
//        digitalWrite(lamp, HIGH);
//      }
//      else if(messageTemp == "off"){
//        digitalWrite(lamp, LOW);
//      }
//  }
  
  if(topic=="Garden/Master/Control"){
      if(messageTemp == "Run"){
        master_run = true;
      }
      else if(messageTemp == "Off"){
        master_run = false;
      }
  }

  if(topic=="Garden/WaterWait/Bernard"){
      if(messageTemp == "Pause"){
        bernard_wait = true;
      }
      else if(messageTemp == "Water"){
        bernard_wait = false;
      }
  }

  if(topic=="Garden/WaterWait/Bellsprout"){
      if(messageTemp == "Pause"){
        bellsprout_wait = true;
      }
      else if(messageTemp == "Water"){
        bellsprout_wait = false;
      }
  }

  Serial.println();
}



// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    network_connected = 0;
    dtostrf(network_connected, 6, 2, Network_Export);
    client.publish("Garden/Master/Connect", Network_Export);
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      network_connected = 1;
      dtostrf(network_connected, 6, 2, Network_Export);
      client.publish("Garden/Master/Connect", Network_Export);


      
      // Subscribe or resubscribe topics HERE!!!!!
      client.subscribe("Garden/Lamp");
      client.subscribe("Garden/Master/Control");
      client.subscribe("Garden/Master/Connect");

      client.subscribe("Garden/Moisture/Bernard");
      client.subscribe("Garden/Valve/Bernard");
      client.subscribe("Garden/WaterWait/Bernard");
      
      client.subscribe("Garden/Moisture/Bellsprout");
      client.subscribe("Garden/Valve/Bellsprout");
      client.subscribe("Garden/WaterWait/Bellsprout");
      
      client.subscribe("Garden/Pump/State");
      client.subscribe("Garden/Pump/Tank");
      
      client.subscribe("Garden/Environment/Temperature");
      client.subscribe("Garden/Environment/Humidity");

      
    }
    
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      network_connected = 0;
      dtostrf(network_connected, 6, 2, Network_Export);
      client.publish("Garden/Master/Connect", Network_Export);
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



// ensure serial monitor is at 115200 baud rate to get text
// MQTT broker and callback are set here
// The callback function is what receives messages and actually controls the LEDs, details above
// the dtostrf function in the loop below is a way to package info to be sent using callback function, under 'client.publish()'

void setup() {
  pinMode(Pump_Control_Pin, OUTPUT);
  pinMode(Valve_1_Control, OUTPUT);
  pinMode(Valve_2_Control, OUTPUT);

  pinMode(Bell_Pin, INPUT);
  pinMode(Bern_Pin, INPUT);
  pinMode(Tank_Check_Pin, INPUT);
  pinMode(DHTPin, INPUT);
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883); //name and port #
  client.setCallback(callback);

  dht.setup(DHTPin, DHTesp::DHT22); //pin #, sensor type

}


void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");

  // Timer to set how often to update sensors
  period = 10000; //period is in millis
  if ((unsigned long)(millis() - now) > period) {
    now = millis();

      //Behavioral Code below
    
      // dtostrf(normal variable, length, decimal places, new static char variable) sets your computed values from microcontroller to the format needed to send them to the dashboard
      // make sure to make a static char variable at the top to store in
      // client.publish("name of room/topic",static char variable with info) Publishes Temperature and Humidity values to the dashboard.
      //THESE ARE THE OUTPUTS FROM CONTROLLER TO DASHBOARD

      // Steps of operation:
      // 1. Pull updated data from all sensors, publish to dashboard for user knowledge
      // 2. Perform water_plants function as needed
    
      //for safety, at the beginning of every loop we will confirm that the pump output is low
      digitalWrite(Pump_Control_Pin,LOW);
      pump_state_value = 0;
      dtostrf(pump_state_value, 6, 2, Pump_StateExport);
      client.publish("Garden/Pump/State", Pump_StateExport);

      network_connected = 1;
      dtostrf(network_connected, 6, 2, Network_Export);
      client.publish("Garden/Master/Connect", Network_Export);

      //below we pull if the tank has water, before we attempt to turn on pump
      //tank_full is default 1, meaning empty, which stops any pump operation. This is for safety
      //it requires a good wifi connection to the circuit for it to be written low, meaning full
      tank_analog_read = analogRead(Tank_Check_Pin);
      if(tank_analog_read <= 100){
        tank_full = 0;
      }
      else{
        tank_full = 1;
      }
      dtostrf(tank_full, 6, 2, Tank_Level_Export);
      client.publish("Garden/Pump/Tank", Tank_Level_Export);

      // below we pull the humidity and temperature from our sensor readings
      humidity_value = dht.getHumidity();
      dtostrf(humidity_value, 6, 2, humidityExport);
      client.publish("Garden/Environment/Humidity", humidityExport);
      
      // Reads temperature as Celsius default, converted to Fahrenheit in code before exporting to dashboard
      celcius_temp = dht.getTemperature();
      fahren_temp = ((celcius_temp * 1.8)+ 32);
      dtostrf(fahren_temp, 6, 2, temperatureExport);
      client.publish("Garden/Environment/Temperature", temperatureExport);

      //below we pull if our plants are considered dry or not
      if(bernard_wait != true){
        bern_moist_value = digitalRead(Bern_Pin);
        dtostrf(bern_moist_value, 6, 2, Bernard_MoistureExport);
        client.publish("Garden/Moisture/Bernard", Bernard_MoistureExport);
      }
      else{
        bern_moist_value = 0;
      }
      
      if(bellsprout_wait != true){
        bell_moist_value = digitalRead(Bell_Pin);
        dtostrf(bell_moist_value, 6, 2, Bellsprout_MoistureExport);
        client.publish("Garden/Moisture/Bellsprout", Bellsprout_MoistureExport);
      }
      else{
        bell_moist_value = 0;
      }
      

      // big boi waterin plants function where the work happens below
      // If the master switch is turned off, nothing should happen
      if(master_run == true){
        water_plants(tank_full,bern_moist_value,bell_moist_value);
      }
      else{
        Serial.println("Master Control Switch is turned off.");
      }
  }
}

void water_plants(int tank, int plant1, int plant2){
  if(tank == 1){
    Serial.println("Water Tank is Empty, please refill it!");
    return;
  }

  if((plant1 == 0) && (plant2 == 0)){
    return;
  }

  if((plant1 == 1) && (plant2 == 1)){
    digitalWrite(Pump_Control_Pin,HIGH);
    pump_state_value = 1;
    dtostrf(pump_state_value, 6, 2, Pump_StateExport);
    client.publish("Garden/Pump/State", Pump_StateExport);

    digitalWrite(Valve_1_Control,HIGH);
    bern_valve_open = 1;
    dtostrf(bern_valve_open, 6, 2, Valve_1_Export);
    client.publish("Garden/Valve/Bernard", Valve_1_Export);

    delay(4000);

    digitalWrite(Valve_1_Control,LOW);
    bern_valve_open = 0;
    dtostrf(bern_valve_open, 6, 2, Valve_1_Export);
    client.publish("Garden/Valve/Bernard", Valve_1_Export);

    digitalWrite(Valve_2_Control,HIGH);
    bell_valve_open = 1;
    dtostrf(bell_valve_open, 6, 2, Valve_2_Export);
    client.publish("Garden/Valve/Bellsprout", Valve_2_Export);

    delay(3000);

    digitalWrite(Pump_Control_Pin,LOW);
    digitalWrite(Valve_2_Control,LOW);

    pump_state_value = 0;
    dtostrf(pump_state_value, 6, 2, Pump_StateExport);
    client.publish("Garden/Pump/State", Pump_StateExport);

    bell_valve_open = 0;
    dtostrf(bell_valve_open, 6, 2, Valve_2_Export);
    client.publish("Garden/Valve/Bellsprout", Valve_2_Export);

    return;
  }
  
  if((plant1 == 1) && (plant2 == 0)){
    digitalWrite(Pump_Control_Pin,HIGH);
    pump_state_value = 1;
    dtostrf(pump_state_value, 6, 2, Pump_StateExport);
    client.publish("Garden/Pump/State", Pump_StateExport);

    digitalWrite(Valve_1_Control,HIGH);
    bern_valve_open = 1;
    dtostrf(bern_valve_open, 6, 2, Valve_1_Export);
    client.publish("Garden/Valve/Bernard", Valve_1_Export);

    delay(5000);

    digitalWrite(Pump_Control_Pin,LOW);
    digitalWrite(Valve_1_Control,LOW);

    pump_state_value = 0;
    dtostrf(pump_state_value, 6, 2, Pump_StateExport);
    client.publish("Garden/Pump/State", Pump_StateExport);

    bern_valve_open = 0;
    dtostrf(bern_valve_open, 6, 2, Valve_1_Export);
    client.publish("Garden/Valve/Bernard", Valve_1_Export);

    return;
  }

  if((plant1 == 0) && (plant2 == 1)){
    digitalWrite(Pump_Control_Pin,HIGH);
    pump_state_value = 1;
    dtostrf(pump_state_value, 6, 2, Pump_StateExport);
    client.publish("Garden/Pump/State", Pump_StateExport);

    digitalWrite(Valve_2_Control,HIGH);
    bell_valve_open = 1;
    dtostrf(bell_valve_open, 6, 2, Valve_2_Export);
    client.publish("Garden/Valve/Bellsprout", Valve_2_Export);

    delay(5000);

    digitalWrite(Pump_Control_Pin,LOW);
    digitalWrite(Valve_2_Control,LOW);
    
    pump_state_value = 0;
    dtostrf(pump_state_value, 6, 2, Pump_StateExport);
    client.publish("Garden/Pump/State", Pump_StateExport);

    bell_valve_open = 0;
    dtostrf(bell_valve_open, 6, 2, Valve_2_Export);
    client.publish("Garden/Valve/Bellsprout", Valve_2_Export);

    return;
  }

}
