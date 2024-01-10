// ********************************** //
// Everything ESP and MQTT
// ********************************** //
#include "WiFiEsp.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "stdlib.h"

char ssid[] = "MRP_BEERPONG";            // your network SSID (name)
char pass[] = "ARG76VEP3D";           // your network password
int status = WL_IDLE_STATUS;            // the WiFi radio's status

#define MQTT_SERVER      "192.168.24.1"
#define MQTT_SERVERPORT  1883 
#define MQTT_USERNAME    "pi"
#define MQTT_KEY         "raspberry"
#define MQTT_BALL_TOPIC             "/ballincups"
#define MQTT_MOTOR_CONTROL_TOPIC    "/motorcommand"  


// Create an ESP8266 WiFiClient class to connect to the MQTT server
WiFiEspClient wifi_client;

// Setup the MQTT client class by assing in the WiFi client and the MQTT server and login details.
Adafruit_MQTT_Client mqtt(&wifi_client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);

// MQTT Publish Balls
Adafruit_MQTT_Publish ballincups_mqtt_publish = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME MQTT_BALL_TOPIC);

// MQTT Subscribe motor commands
Adafruit_MQTT_Subscribe motorcommand_mqtt_subscribe = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME MQTT_MOTOR_CONTROL_TOPIC);

// publish
#define PUBLISH_INTERVAL 30000
unsigned long prev_post_time;

// debug
#define DEBUG_INTERVAL 2000
unsigned long prev_debug_time;



// ********************************** //
// Defines the constant for calibration.
// ********************************** //

int sensorValue_front_cal = 0;     // A0
int sensorValue_midleft_cal = 0;   // A1 
int sensorValue_midright_cal = 0;  // A2
int sensorValue_backleft_cal = 0;  // A3
int sensorValue_backmid_cal = 0;   // A4
int sensorValue_backright_cal = 0; // A5
int calibration_loop = 50;



// ********************************** //
// Defines for sensor ball_threshold
// ********************************** //

int sensor_threshold = -50;



// ********************************** //
// Defines for sensor output.
// ********************************** //

int sensor_front;
int sensor_midleft;
int sensor_midright;
int sensor_backleft;
int sensor_backmid;
int sensor_backright;



// ********************************** //
// Beerpong transmission values.
// ********************************** //

int transmission_int = 0;



// ********************************** //
// Finite state machine.
// ********************************** //

int state = 0;
int state_calibration = 0;
int state_ball_detection = 1;
int state_motor_control = 2;



// ********************************** //
// Everything movement
// ********************************** //

uint8_t* movement_level;

// Definitions Arduino pins connected to input H Bridge
int EN0 = 3; 
int IN1 = 4; //RIGHT 
int IN2 = 5; //RIGHT
int IN3 = 6; //LEFT
int IN4 = 7; //LEFT
int EN5 = 8;  
int IN5 = 11; //FRONT
int IN6 = 12; //FRONT
int EN7 = 13; 

int set_speed_right = 80;
int set_speed_left = 90;
int set_speed_front = 100;

void all_high(){
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
  digitalWrite(IN5, HIGH);
  digitalWrite(IN6, HIGH);
}

void backward(){
  digitalWrite(IN1, LOW); //RIGHT
  digitalWrite(IN2, HIGH);
  analogWrite(EN0, set_speed_right);
  digitalWrite(IN3, LOW); //LEFT
  digitalWrite(IN4, HIGH);
  analogWrite(EN5, set_speed_left);
} 

void forward(){
  digitalWrite(IN1, HIGH); //RIGHT
  digitalWrite(IN2, LOW);
  analogWrite(EN0, set_speed_right);

  digitalWrite(IN3, HIGH); //LEFT
  digitalWrite(IN4, LOW);
  analogWrite(EN5, set_speed_left);
}

void backwards_right(){
  digitalWrite(IN3, LOW); //LEFT
  digitalWrite(IN4, HIGH);
  analogWrite(EN5, set_speed_left);

  digitalWrite(IN5, LOW); //FRONT
  digitalWrite(IN6, HIGH);
  analogWrite(EN7, set_speed_front);
}

void backwards_left(){
  digitalWrite(IN1, LOW); //RIGHT
  digitalWrite(IN2, HIGH);
  analogWrite(EN0, set_speed_right);

  digitalWrite(IN5, HIGH); //FRONT
  digitalWrite(IN6, LOW);
  analogWrite(EN7, set_speed_front);  
}

void forward_right(){
  digitalWrite(IN1, HIGH); //RIGHT
  digitalWrite(IN2, LOW);
  analogWrite(EN0, set_speed_right);

  digitalWrite(IN5, LOW); //FRONT
  digitalWrite(IN6, HIGH);
  analogWrite(EN7, set_speed_front);  
}

void forward_left(){
  digitalWrite(IN3, HIGH); //left
  digitalWrite(IN4, LOW);
  analogWrite(EN5, set_speed_left);

  digitalWrite(IN5, HIGH); //FRONT
  digitalWrite(IN6, LOW);
  analogWrite(EN7, set_speed_front); 
}

void rotateTriangle() {
  backwards_right();
  delay(1000);
  all_high();
  delay(1000);
  backwards_left();
  delay(1000);
  all_high();
  delay(1000);
  forward();
  delay(1000);
  all_high();
  delay(1000);
}



// ********************************** //
// mqtt_connect function
// ********************************** //

void mqtt_connect()
{
  int8_t ret;
  
  // Stop if already connected.
  if (! mqtt.connected())
  {
    Serial.println("Connecting to MQTT");
    while ((ret = mqtt.connect()) != 0)
    { // connect will return 0 for connected
      
         Serial.println(mqtt.connectErrorString(ret));
         mqtt.disconnect();
         delay(5000);  // wait 5 seconds
    }

  }
  Serial.println("MQTT Connected");
}



void setup() {
  //setup all the pins for controlling the motors.
  Serial.println("Setting up the motor pins for control");
  // Set the output pins
  pinMode(EN0, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(EN5, OUTPUT);
  pinMode(IN5, OUTPUT);
  pinMode(IN6, OUTPUT);
  pinMode(EN7, OUTPUT);
  
  // Start the serial communication
  Serial.begin(115200);
  Serial3.begin(115200);
  WiFi.init(&Serial3);
  

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true); // don't continue
  }

  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(4000);
  }
  Serial.println("You're connected to the network");

  delay(5000); // important as MQQT sometimes can fall otherweise.
  // Setup MQTT subscription for motorcommand_mqtt_subscribe feed.
  //mqtt.subscribe(&motorcommand_mqtt_subscribe);
  //delay(5000);

}



// ********************************** //
// Calibration state //
// ********************************** //

void calibration() {
  // Calibration loop to set the value to close to zero. 
  for (int i = 0; i < calibration_loop; i++) {
    // Read the analog values from A0-A5
    sensorValue_front_cal += (analogRead(A0));
    sensorValue_midleft_cal += (analogRead(A1));
    sensorValue_midright_cal += (analogRead(A2));
    sensorValue_backleft_cal += (analogRead(A3));
    sensorValue_backmid_cal += (analogRead(A4));
    sensorValue_backright_cal += (analogRead(A5));
    delay(100);
  }
  sensorValue_front_cal = (sensorValue_front_cal / calibration_loop);
  sensorValue_midleft_cal = (sensorValue_midleft_cal / calibration_loop);
  sensorValue_midright_cal = (sensorValue_midright_cal / calibration_loop);
  sensorValue_backleft_cal = (sensorValue_backleft_cal / calibration_loop);
  sensorValue_backmid_cal = (sensorValue_backmid_cal / calibration_loop);
  sensorValue_backright_cal = (sensorValue_backright_cal / calibration_loop);
  delay(100);
}



// ********************************** //
// Ball detection state with calibrated sensor readings //
// ********************************** //

void read_calibrated_sensor() {
  sensor_front = (analogRead(A0) - sensorValue_front_cal);
  sensor_midleft = (analogRead(A1) - sensorValue_midleft_cal);
  sensor_midright = (analogRead(A2) - sensorValue_midright_cal);
  sensor_backleft = (analogRead(A3) - sensorValue_backleft_cal);
  sensor_backmid = (analogRead(A4) - sensorValue_backmid_cal);
  sensor_backright = (analogRead(A5) - sensorValue_backright_cal);
  delay(100);
}

void ball_detection_threshold_loop() {
  // Put the ball in the front cup, and let it stay there for X seconds

    int sf = 0;
    int sml = 0;
    int smr = 0;
    int sbl = 0;
    int sbm = 0;
    int sbr = 0;
    int range = 20;
    for (int i = 0; i < range; i++) {
      read_calibrated_sensor();
      sf   += sensor_front;
      sml  += sensor_midleft;
      smr  += sensor_midright;
      sbl  += sensor_backleft;
      sbm  += sensor_backmid;
      sbr  += sensor_backright;
      delay(100);
    }
    sf = sf / range;
    sml = sml / range;
    smr = smr / range;
    sbl = sbl / range;
    sbm = sbm / range;
    sbr = sbr / range; 
    
    if(sf < sensor_threshold){
       transmission_int += 1;
    }
    if(sml < sensor_threshold){
       transmission_int += 2;
    }
    if(smr < sensor_threshold){
       transmission_int += 4;
    }
    if(sbl < sensor_threshold){
       transmission_int += 8;
    }
    if(sbm < sensor_threshold){
       transmission_int += 16;
    }
    if(sbr < sensor_threshold){
       transmission_int += 32;
    }    
    
    char payload[10];
    itoa(transmission_int, payload, 10);
 
    Serial.println("starting connection to server...");
    mqtt_connect();
    if (! ballincups_mqtt_publish.publish(payload))
    {
      Serial.println("MQTT failed");
    }
    else
    {
      Serial.print(". Publish succesfull");
    }
     transmission_int = 0;
    rotateTriangle();
    
     delay(100);
}



// ********************************** //
// Unused motor control loop //
// ********************************** //

void motor_control_loop(){

  // First connection to MQTT
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &motorcommand_mqtt_subscribe) {
      Serial.print(F("Got: "));
      Serial.println((char *)motorcommand_mqtt_subscribe.lastread);
      movement_level = motorcommand_mqtt_subscribe.lastread;
    }
  }
  delay(100);
  rotateTriangle();
}



// ********************************** //
// Arduino main loop
// ********************************** //

void loop() {
  switch (state) {
    case 0:
      Serial.println("state_calibration");
      //calibration();
      state = state_ball_detection;
      break;
    case 1:
      
      Serial.println("state_ball_detection");
      ball_detection_threshold_loop();
      state = state_ball_detection; // Staying here for now
      break;
    case 2:
      Serial.println("state_motor_control");
      motor_control_loop();
      state = state_ball_detection;
      break;
    default:
      break;
  }
}
