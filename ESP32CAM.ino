// library
#include "esp_camera.h"
#include <Arduino.h>
#include "base64.h"

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

FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

Servo mainServo;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

void initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = 5;
    config.pin_d1 = 18;
    config.pin_d2 = 19;
    config.pin_d3 = 21;
    config.pin_d4 = 36;
    config.pin_d5 = 39;
    config.pin_d6 = 34;
    config.pin_d7 = 35;
    config.pin_xclk = 0;
    config.pin_pclk = 22;
    config.pin_vsync = 25;
    config.pin_href = 23;
    config.pin_sscb_sda = 26;
    config.pin_sscb_scl = 27;
    config.pin_pwdn = 32;
    config.pin_reset = -1;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12; 
    config.fb_count = 1;

    // Initialize the camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera Init Failed");
        return;
    }
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

void setup()
{
  Serial.begin(115200);

  initCamera();

  establishWifiConnection();

  establishFirebaseConnection();
}

String captureImageBase64() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return "";
    }

    String base64Image = base64::encode(fb->buf, fb->len);
    esp_camera_fb_return(fb);

    return base64Image;
}

void loop()
{
  
  String base64Image = captureImageBase64();
  if (base64Image != "" && Firebase.ready()) {
      if (Firebase.setString(fbdo,FPSTR("CAM"), base64Image)){
        Serial.println("Image uploaded Successfully!");
      }
      else {
        Serial.println("Failed to upload image");
        Serial.println(fbdo.errorReason());
      }
  }
}