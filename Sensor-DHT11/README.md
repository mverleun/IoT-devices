# Sensor Wemos D1 mini with DHT11

This firmware is meant to be used on a Wemos D1 Mini with a DHT-11 shield.
There are several shields available, make sure you get one that has the DHT-11 connected to D4 and *not* the I2C version.

## Connecting the hardware
Connect the hardware as follows: 

```
DHT11     Wemos D1 Mini
VCC ----- +3.3V
GND ----- GND
SIG ----- D4
```
##MQTT
If all is configured well you should receive measurements on the following topics:

```
<base_topic>/<device_id>/temperature/temperature
<base_topic>/<device_id>/temperature/json
<base_topic>/<device_id>/humidity/humidity
<base_topic>/<device_id>/humidity/json
```

Temperature and humidity are reported as floating point values.
The json output contains the name of the sensor, the name of the metric reported and the value of the measurement.