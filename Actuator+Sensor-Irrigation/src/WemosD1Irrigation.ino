/*
1.0.0  02-06-2018 Initial version
*/
#include <Homie.h>

#include <ArduinoJson.h>

StaticJsonBuffer<300> jsonBuffer;

// For DHT 11
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN                D4 // Pin which is connected to the DHT sensor.
#define MOISTURE_SENSOR_PIN   A0 // Pin where readings from sensor come in.
#define MOISTURE_SENSOR_PWR_PIN D7 // Used to switch sensor power on and off
#define RELAYPIN              D1 // Pin to which relay (pump) is connected.
// Uncomment the type of sensor in use:
#define DHTTYPE           DHT11     // DHT 11
//#define DHTTYPE           DHT22     // DHT 22 (AM2302)
//#define DHTTYPE           DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

uint32_t delayMS;  // Store the minimum wait time between readings

float current_temperature;
float previous_temperature = 0.0;
float current_humidity;
float previous_humidity = 0.0;
float current_moisture;
float previous_moisture = 0.0;
float desiredMoisture = 0.0;

boolean relayOn = false;

// The DHT-11 sensor provides two different readings.
// As such we can create two different nodes. One for temperature and one for humidiy
HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");
HomieNode moistureNode("moisture", "moisture");
HomieNode relayNode("relay", "relay");

float readMoisture() {
  float moisture;

  pinMode(MOISTURE_SENSOR_PWR_PIN, OUTPUT);
  digitalWrite(MOISTURE_SENSOR_PWR_PIN, HIGH); // Power sensor on
  delay(3000); // settle time
  moisture = 100 - analogRead(MOISTURE_SENSOR_PIN) / 10;
  Homie.getLogger() << "readMoisture: " << String(moisture) << " %" << endl;
  digitalWrite(MOISTURE_SENSOR_PWR_PIN, LOW);  // Power sensor off
  return moisture;
}

void setRelay(String value) {
  if (value == "on") {
    digitalWrite(RELAYPIN, HIGH);
    relayOn = true;
  }
  if (value == "off") {
    digitalWrite(RELAYPIN, LOW);
    relayOn = false;
  }
  relayNode.setProperty("status").send(value);
}

bool moistureHandler(HomieRange range, String value) {
  Serial << "Received desiredMoisture: " << String(value) << endl;
  desiredMoisture = value.toFloat();
  moistureNode.setProperty("desiredMoisture").send(value);
  return true;
}

void setupHandler() {
  temperatureNode.setProperty("unit").send("°C");
  humidityNode.setProperty("unit").send("%");
  moistureNode.setProperty("unit").send("%");
  moistureNode.setProperty("desiredMoisture").send("0");
  relayNode.setProperty("status").send("off");
  // Initialize device.
  dht.begin();

  delayMS = 50;
}

bool broadcastHandler(const String& level, const String& value) {
  Serial << "Received broadcast level " << level << ": " << value << endl;
  return true;
}

void loopHandler() {
  char JSONmessageBuffer[100];
  JsonObject& JSONroot = jsonBuffer.createObject();
  JSONroot["name"] = String(Homie.getConfiguration().name);

  // Get temperature event and publish its value.
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float current_moisture = readMoisture();

  if (isnan(temperature)) {
    Serial.println("Error reading temperature!");
  }
  else {
    current_temperature = temperature;
    // Only publish info if there is a change in Temperature
    if ( current_temperature != previous_temperature ) {
      Homie.getLogger() << "Temperature: " << current_temperature << " °C" << endl;
      // Publish value as float
      temperatureNode.setProperty("value").send(String(current_temperature));

      // Publish data in JSON format
      JSONroot["metric"] = "temperature";
      JSONroot["value"] = String(current_temperature);

      JSONroot.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      temperatureNode.setProperty("json").send(JSONmessageBuffer);

      previous_temperature = current_temperature;
    }
  }

  if (isnan(humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    current_humidity = humidity;
    // Only publish info if there is a change in Humidity
    if ( current_humidity != previous_humidity ) {
      Homie.getLogger() << "Humidity: " << current_humidity << " %" << endl;
      humidityNode.setProperty("value").send(String(current_humidity));

      JSONroot["metric"] = "humidity";
      JSONroot["value"] = String(current_humidity);

      JSONroot.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      humidityNode.setProperty("json").send(JSONmessageBuffer);

      previous_humidity = current_humidity;
    }
  }

  Homie.getLogger() << "Moisture: " << current_moisture << " %" << endl;
  // Only publish info if there is a change in Moisture and we have a valid
  // reading
  if (current_moisture != previous_moisture and current_moisture > 0) {
    Homie.getLogger() << "Moisture: " << current_humidity << " %" << endl;
    moistureNode.setProperty("value").send(String(current_moisture));

    JSONroot["metric"] = "moisture";
    JSONroot["value"] = String(current_moisture);

    JSONroot.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    moistureNode.setProperty("json").send(JSONmessageBuffer);

    previous_moisture = current_moisture;
  }
  // Check if we need to turn on the relay
  if (relayOn == false and current_moisture < desiredMoisture) {
    setRelay("on");
  }
  // Check if we need to turn off the relay
  if (relayOn == true and current_moisture > desiredMoisture) {
    setRelay("off");
  }
}

void setup() {
  Serial.begin(115200);
  Serial << endl << endl;
  Homie_setFirmware("wemos-d1-irrigation", "1.0.0");
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
  Homie.setBroadcastHandler(broadcastHandler); // before Homie.setup()
  temperatureNode.advertise("actual");
  humidityNode.advertise("actual");
  moistureNode.advertise("actual");
  moistureNode.advertise("setpoint").settable(moistureHandler);
  relayNode.advertise("actual");
  Homie.setup();
}

void loop() {
  Homie.loop();
}
