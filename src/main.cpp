#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <dht.h>
#include <Ticker.h>
#include <PubSubClient.h>

void measure();
void transmit();

static WiFiClient wificlient;
static PubSubClient pubsubclient(wificlient);
static Ticker measure_ticker(measure, 60 * 1000, 0, MILLIS);
static Ticker transmit_ticker(transmit, 60 * 60 * 1000, 0, MILLIS);
static dht dht;
static float humidity_sum;
static float temperature_sum;
static unsigned int measurements;
static String id = String(ESP.getChipId());

void setup()
{
  Serial.begin(115200);

  WiFiManager wifimanager;
  wifimanager.autoConnect();

  pubsubclient.setServer("konehuone", 1883);
  
  measure_ticker.start();
  transmit_ticker.start();
}

void loop()
{
  pubsubclient.loop();
  measure_ticker.update();
  transmit_ticker.update();
}

void measure()
{
  int status = dht.read(D2);
  
  if (status != DHTLIB_OK) {
    Serial.printf("measure: dht.read: status: %d\n", status);
  }
  else {
    humidity_sum += dht.humidity;
    temperature_sum += dht.temperature;
    measurements++;

    Serial.printf("measure: humidity: %.2f temperature: %.2f "
		  "measurements: %u\n",
		  dht.humidity, dht.temperature, measurements);
  }
}

void transmit()
{
  if (measurements > 0) {
    if (!pubsubclient.connected()) {
      if (!pubsubclient.connect(id.c_str())) {
	Serial.printf("transmit: pubsubclient.state: %d\n", pubsubclient.state());
	return;
      }
    }

    char topic[80];
    char payload[80];

    memset(topic, 0, sizeof topic);
    memset(payload, 0, sizeof payload);
    snprintf(topic, sizeof topic, "sensor/%s/humidity", id.c_str());
    snprintf(payload, sizeof payload, "%.2f", (float) humidity_sum / (float) measurements);
    pubsubclient.publish(topic, payload);

    memset(topic, 0, sizeof topic);
    memset(payload, 0, sizeof payload);
    sprintf(topic, "sensor/%s/temperature", id.c_str());
    sprintf(payload, "%.2f", (float) temperature_sum / (float) measurements);
    pubsubclient.publish(topic, payload);

    humidity_sum = 0.0;
    temperature_sum = 0.0;
    measurements = 0;
  }
}
