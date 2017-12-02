# Water Level Indicator

This project consists of two Wemos D1 mini units, which are ESP8266 based MCUs. These units are programmed using the Arduino IDE. 

One of the two Wemos units is connected to an LED array. We'll call this the "display Wemos", and it will be near the user, displaying the water level.

The other Wemos unit will be connected to an Ultrasound Sensor, hence called the "sensor Wemos", and will be installed at the water tank, above the maximum water level.

The code (to be edited and flashed using the Arduino IDE) consists of two files - **wemos-display.ino** and **wemos-sensor.ino**.

As the filename suggests, wemos-display.ino needs to be flashed on to the display Wemos and wemos-sensor.ino needs to go onto the sensor Wemos.

