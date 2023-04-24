#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <math.h>

// Setting DHT Configuration
#define DHTPIN 4          // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11     // DHT 11
DHT dht(DHTPIN, DHTTYPE); // Initialing DHT Class7
#define MQ135_THRESHOLD_1 1000 // Fresh Air threshold
#define MQ135PIN 34
#define KY037PIN 35

// WiFi Network Configuration
const char* ssid     = "Noah_ Exa internet";
const char* password = "*********";
const char *hostname = "esp32";

//Helix IP Address
const char *orionAddressPath = "18.234.188.169:1026/v2";

// Device ID (example: urn:ngsi-ld:entity:001)
const char *deviceID = "espRU";

HTTPClient http;
#define LED_BUILTIN 2

void setup()
{
    Serial.begin(115200);
    Serial.println("\nBootting...");
    pinMode(LED_BUILTIN, OUTPUT);
    dht.begin();
    setupWiFi();
}

void setupWiFi()
{
    Serial.print("Connecting to ");
    Serial.println(ssid);

  
    //WiFi.mode(WIFI_STA);
    //WiFi.setHostname(hostname);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
    float totalTemperature = 0;
    float totalHumidity = 0;
    int noise_dB = 0;
    int MQ135_data = analogRead(MQ135PIN);
    float noise = analogRead(KY037PIN);
    noise_dB = map(noise, 150, 2500, 48, 66);
    if(MQ135_data < MQ135_THRESHOLD_1){
      Serial.print("Fresh Air:\n");
    } else {
       Serial.print("Poor Air:\n"); 
    }
    Serial.print(MQ135_data); // analog data
    Serial.print(noise); // analog data
    float actualTemperature = dht.readTemperature(false);
        // Wait a few seconds between measurements
    delay(10);
    float actualHumidity = dht.readHumidity();
    
    
    


    

   
    char msgHumidity[10];
    char msgTemperature[10];
    char msgNoise[10];
    char msgAir_quality[10];
    sprintf(msgHumidity, "%2.f", actualHumidity);
    sprintf(msgTemperature, "%2.f", actualTemperature);
    sprintf(msgNoise, "%d", noise_dB);
    sprintf(msgAir_quality, "%d", MQ135_data);

    // Update
    Serial.println("Updating data in orion...");
    orionUpdate(deviceID, msgTemperature, msgHumidity, msgNoise, msgAir_quality);

    // Luminous feedback
    digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(500);
    digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
    delay(500);
    Serial.println("Finished updating data in orion...");
}

// Request Helper
void httpRequest(String path, String data)
{
    String payload = makeRequest(path, data);

    if (!payload)
    {
        return;
    }

    Serial.println("##[RESULT]## ==> " + payload);
}

// Request Helper
String makeRequest(String path, String bodyRequest)
{
    String fullAddress = "http://" + String(orionAddressPath) + path;
    http.begin(fullAddress);
    Serial.println("Orion URI request: " + fullAddress);

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("fiware-service", "helixiot");
    http.addHeader("fiware-servicepath", "/");

    Serial.println(bodyRequest);
    int httpCode = http.POST(bodyRequest);

    //String response = http.getString();
  
    Serial.println("HTTP CODE");
    Serial.println(httpCode);
    if (httpCode < 0)
    {
        Serial.println("request error - " + httpCode);
        return "";
    }

    if (httpCode != HTTP_CODE_OK)
    {
        return "";
    }

    http.end();
    return "certo";    
}

// Creating the device in the Helix Sandbox (plug&play)
void orionCreateEntity()
{
    Serial.println("Creating " + String(deviceID) + " entity...");
    String bodyRequest = "{\"id\": \"" + String(deviceID) + "\", \"type\": \"iot\", \"temperature\": { \"value\": \"0\", \"type\": \"integer\"},\"humidity\": { \"value\": \"0\", \"type\": \"integer\"}}";
    httpRequest("/entities", bodyRequest);
}

// Update Values in the Helix Sandbox
void orionUpdate(String entityID, String temperature, String humidity, String noise, String air_quality)
{   
    delay(1000);
    Serial.println(noise);
    String bodyRequest = "{\"temperature\": { \"value\": \"" + temperature + "\", \"type\": \"float\"}, \"humidity\": { \"value\": \"" + humidity + "\", \"type\": \"float\"},\"noise\": { \"value\": \"" + noise + "\", \"type\": \"float\"}, \"air_quality\": { \"value\": \"" + air_quality + "\", \"type\": \"int\"}}";
    String pathRequest = "/entities/" + entityID + "/attrs?options=forcedUpdate";
    httpRequest(pathRequest, bodyRequest);
}
