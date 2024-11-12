// library
#include <Arduino.h>

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#include <ESP32Servo.h>

// wifi and firebase config
#define WIFI_SSID "Ainsworth's"
#define WIFI_PASSWORD "ainzsama"

#define API_KEY "AIzaSyCyxDQzCLMsQ2P-bPhYcw9StXtvz79zldA"

#define DATABASE_URL "https://sweep-e-default-rtdb.asia-southeast1.firebasedatabase.app"

#define USER_EMAIL "iotsweepe@gmail.com"
#define USER_PASSWORD "iotsweepe123"

// pinout
// motor driver
#define leftBACK 4
#define leftFWRD 16
#define rightFWRD 14
#define rightBACK 27
#define ena1 17
#define ena2 26

//servo
#define servo 25

// ultrasonic sensor
#define sensorTrig 12
#define sensorEcho 13

//relay
#define fanRelay 5

#define MAX_SPEED 255
  
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

Servo mainServo;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

void initializePinOut(){
  // motor driver
  pinMode(leftFWRD,OUTPUT);
  pinMode(leftBACK,OUTPUT);
  pinMode(rightBACK,OUTPUT);
  pinMode(rightFWRD,OUTPUT);
  // pin ena enb
  ledcAttach(ena1, 5000, 8);
  ledcAttach(ena2, 5000, 8);

  // servo
  mainServo.setPeriodHertz(50);
  mainServo.attach(servo);
  
  // sensor
  pinMode(sensorTrig, OUTPUT);
  pinMode(sensorEcho,INPUT);

  //Relay (fan)
  pinMode(fanRelay, OUTPUT_OPEN_DRAIN);  
}

void establishWifiConnection(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void establishFirebaseConnection(){
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;


  config.database_url = DATABASE_URL;

 
  config.token_status_callback = tokenStatusCallback; 


  Firebase.reconnectNetwork(true);


  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  Firebase.begin(&config, &auth);

  Firebase.setDoubleDigits(5);
}

long readDistance(String view){
  digitalWrite(sensorTrig,LOW);
  delayMicroseconds(2);
  digitalWrite(sensorTrig,HIGH);
  delayMicroseconds(10);
  digitalWrite(sensorTrig,LOW);

  long duration = pulseIn(sensorEcho, HIGH);
  long distance = duration/58.2;
  Serial.printf("Distance %s : %ld\n",view, distance);
  return distance;
}

void setAngle(int angle){
  ledcWrite(servo,map(angle,0,180,26,128));
}

void setup()
{

  Serial.begin(115200);

  initializePinOut();

  establishWifiConnection();

  establishFirebaseConnection();
   
  setAngle(90);
}

int scoutLeft()
{
  setAngle(45);
  delay(1500);
  int distanceFront = readDistance("LEFt");
  setAngle(90);
  delay(1500);
  return distanceFront;
}

int scoutRight()
{
  setAngle(135);
  delay(1500);
  int distanceFront = readDistance("RIGHT");
  setAngle(90);
  delay(1500);
  return distanceFront;
}

void setSpeed(double speed){
  ledcWrite(ena1, (int) speed);
  ledcWrite(ena2, (int) speed);
}


void setMovementIdle(){
  digitalWrite(leftFWRD, LOW);
  digitalWrite(leftBACK, LOW);
  digitalWrite(rightBACK, LOW); 
  digitalWrite(rightFWRD, LOW);
}

void setMovementFWRD(){
  digitalWrite(leftFWRD, HIGH);
  delay(10);
  digitalWrite(rightFWRD, HIGH);
  delay(10);    
}

void setMovementBACK(){
  digitalWrite(leftBACK, HIGH);
  delay(10);
  digitalWrite(rightBACK, HIGH);
  delay(10);  
}

void setMovementLEFT(){
  setSpeed(200.0);
  digitalWrite(leftFWRD, HIGH);
  delay(10);
  digitalWrite(rightBACK,HIGH);
  delay(10);
}

void setMovementRGHT(){
  setSpeed(200.0);
  digitalWrite(rightFWRD, HIGH);
  delay(10);
  digitalWrite(leftBACK, HIGH);
  delay(10);
}

int distanceFront = 100;

void autoMode(){

  int distanceRight = 0;
  int distanceLeft = 0;
  delay(500);
  distanceFront = readDistance("FRONT");
  delay(500);
  if (distanceFront <= 25){
    setMovementIdle();
 
    delay(1000);
    distanceLeft = scoutLeft();
    delay(1000);
    distanceRight = scoutRight();
    delay(1000);
    Serial.printf("Auto moving %s\n", distanceLeft >= distanceRight ? "LEFT" : "RIGHT");
    if(distanceLeft >= distanceRight)
    {
      setMovementLEFT();
      delay(700);
      setMovementIdle();
    }
    else
    {
      setMovementRGHT();
      delay(700);
      setMovementIdle();
    }
  }
  else{
    setMovementFWRD();
    delay(500);
    setMovementIdle();
  }
   delay(100);
}
 
void setFan(bool fanStatus){
  digitalWrite(fanRelay, !fanStatus ? HIGH : LOW);
  Serial.printf("Fan status changed!\n");
}

bool isTest = false; 

void testSensor(){  
  readDistance("TEST");
  delay(1000);  
}
 
void testServo(){
  for (int i = 0; i <= 180; i +=20){
    setAngle(i);
    delay(1500);
  }
}

void testRelay(){
  delay(2000);
  digitalWrite(fanRelay,HIGH);
  delay(3500);
  digitalWrite(fanRelay,LOW);
  Serial.printf("Test relay \n");
}

void loop()
{

  // Firebase.ready() should be called repeatedly to handle authentication tasks.
  if (Firebase.ready())
  // if (Firebase.ready() && (millis() - sendDataPrevMillis > 100 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    

    bool fanStatus = Firebase.getBool(fbdo, FPSTR("FAN")) ? fbdo.to<bool>() ? true : false : false;
    bool autoStatus = Firebase.getBool(fbdo, FPSTR("AUTO")) ? fbdo.to<bool>() ? true : false : false;
    Serial.printf("Fan status = %s\n", fanStatus ? "true" : "false");
    Serial.printf("Auto status = %s\n", autoStatus ? "true" : "false");
    
    double speedMultiplier = Firebase.getDouble(fbdo, FPSTR("SPEED")) ? fbdo.to<double>() : 0.0;

    Serial.printf("Speed Multipler is %f\n", speedMultiplier);


    String strafeState = Firebase.getString(fbdo, FPSTR("STRAFE")) ? fbdo.to<const char *>() : "IDLE";
    
    Serial.printf("Strafe State = %s\n", strafeState);

    // speed multiplier
    double finalSpeed = MAX_SPEED * speedMultiplier;
    Serial.printf("Final Speed = %d\n", (int) finalSpeed);

    setSpeed(finalSpeed);
    setFan(fanStatus);
    if (autoStatus){
      setMovementIdle();
      // testServo();
      // testSensor();
      setSpeed(145.0);
      autoMode();
    }
    else if (strafeState == "IDLE"){
      setMovementIdle();
    }
    else if (strafeState == "FWRD"){
      setMovementFWRD();
    }
    else if (strafeState == "BACK"){
      setMovementBACK();
    }
    else if (strafeState == "LEFT"){
      setMovementLEFT();
    }
    else if (strafeState == "RGHT"){
      setMovementRGHT();
    }
    
    // bool bVal;
    // Serial.printf("Get bool ref... %s\n", Firebase.getBool(fbdo, F("/test/bool"), &bVal) ? bVal ? "true" : "false" : fbdo.errorReason().c_str());

    // Serial.printf("Set int... %s\n", Firebase.setInt(fbdo, F("/test/int"), count) ? "ok" : fbdo.errorReason().c_str());

    // Serial.printf("Get int... %s\n", Firebase.getInt(fbdo, F("/test/int")) ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());

    // int iVal = 0;
    // Serial.printf("Get int ref... %s\n", Firebase.getInt(fbdo, F("/test/int"), &iVal) ? String(iVal).c_str() : fbdo.errorReason().c_str());

    // Serial.printf("Set float... %s\n", Firebase.setFloat(fbdo, F("/test/float"), count + 10.2) ? "ok" : fbdo.errorReason().c_str());

    // Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, F("/test/float")) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());

    // Serial.printf("Set double... %s\n", Firebase.setDouble(fbdo, F("/test/double"), count + 35.517549723765) ? "ok" : fbdo.errorReason().c_str());

    // Serial.printf("Get double... %s\n", Firebase.getDouble(fbdo, F("/test/double")) ? String(fbdo.to<double>()).c_str() : fbdo.errorReason().c_str());

    // Serial.printf("Set string... %s\n", Firebase.setString(fbdo, F("/test/string"), "Hello World!") ? "ok" : fbdo.errorReason().c_str());

    // Serial.printf("Get string... %s\n", Firebase.getString(fbdo, F("/test/string")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str());



    // For generic set/get functions.

    // For generic set, use Firebase.set(fbdo, <path>, <any variable or value>)

    // For generic get, use Firebase.get(fbdo, <path>).
    // And check its type with fbdo.dataType() or fbdo.dataTypeEnum() and
    // cast the value from it e.g. fbdo.to<int>(), fbdo.to<std::string>().

    // The function, fbdo.dataType() returns types String e.g. string, boolean,
    // int, float, double, json, array, blob, file and null.

    // The function, fbdo.dataTypeEnum() returns type enum (number) e.g. firebase_rtdb_data_type_null (1),
    // firebase_rtdb_data_type_integer, firebase_rtdb_data_type_float, firebase_rtdb_data_type_double,
    // firebase_rtdb_data_type_boolean, firebase_rtdb_data_type_string, firebase_rtdb_data_type_json,
    // firebase_rtdb_data_type_array, firebase_rtdb_data_type_blob, and firebase_rtdb_data_type_file (10)

    count++;
  }
}

/// PLEASE AVOID THIS ////

// Please avoid the following inappropriate and inefficient use cases
/**
 *
 * 1. Call get repeatedly inside the loop without the appropriate timing for execution provided e.g. millis() or conditional checking,
 * where delay should be avoided.
 *
 * Everytime get was called, the request header need to be sent to server which its size depends on the authentication method used,
 * and costs your data usage.
 *
 * Please use stream function instead for this use case.
 *
 * 2. Using the single FirebaseData object to call different type functions as above example without the appropriate
 * timing for execution provided in the loop i.e., repeatedly switching call between get and set functions.
 *
 * In addition to costs the data usage, the delay will be involved as the session needs to be closed and opened too often
 * due to the HTTP method (GET, PUT, POST, PATCH and DELETE) was changed in the incoming request.
 *
 *
 * Please reduce the use of swithing calls by store the multiple values to the JSON object and store it once on the database.
 *
 * Or calling continuously "set" or "setAsync" functions without "get" called in between, and calling get continuously without set
 * called in between.
 *
 * If you needed to call arbitrary "get" and "set" based on condition or event, use another FirebaseData object to avoid the session
 * closing and reopening.
 *
 * 3. Use of delay or hidden delay or blocking operation to wait for hardware ready in the third party sensor libraries, together with stream functions e.g. Firebase.RTDB.readStream and fbdo.streamAvailable in the loop.
 *
 * Please use non-blocking mode of sensor libraries (if available) or use millis instead of delay in your code.
 *
 * 4. Blocking the token generation process.
 *
 * Let the authentication token generation to run without blocking, the following code MUST BE AVOIDED.
 *
 * while (!Firebase.ready()) <---- Don't do this in while loop
 * {
 *     delay(1000);
 * }
 *
 */