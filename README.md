# Water Level Indicator

This project consists of two Wemos D1 mini units ( https://wiki.wemos.cc/products:d1:d1_mini ), which are ESP8266 ( http://espressif.com/en/products/hardware/esp8266ex/overview ) based MCUs. For this project, these units are to be programmed using the Arduino IDE. 

One of the two Wemos units is connected to an LED array. We'll call this the "display Wemos", and it will be near the user, displaying the water level.

The other Wemos unit will be connected to an Ultrasound Sensor, hence called the "sensor Wemos", and will be installed at the water tank, above the maximum water level.

The code (to be edited and flashed using the Arduino IDE) consists of two files - **wemos-display.ino** and **wemos-sensor.ino**.

As the filename suggests, wemos-display.ino needs to be flashed on to the display Wemos and wemos-sensor.ino needs to go onto the sensor Wemos.

Once the units are flashed with the code and everything is connected and powered up, the two Wemos units communicate over a Wi-Fi network that is setup by one of the Wemos units (display wemos) themselves.

The sensor Wemos measures the distance to the water surface, calculates the depth of water in the tank, normalizes this data on a scale of 0-9 and sends it once every 5 seconds to the display Wemos by calling a http URL endpoint hosted on the display Wemos.

The display Wemos uses the received data (a number in the ange 0-9) to light up the requisite mumber on LEDs in the LED array.

The code also lights up a 10th LED as a signal for a water-pump motor to turn ON and OFF whenever the water level reaches the bottom and the top of the scale, espectively.  

## Sensor Wemos - wiring diagram
![alt text](https://github.com/ajithvasudevan/water-level-indicator/raw/master/Sensor%20Wemos%20-%20wiring%20diagram.png "Sensor Wemos - Wiring Diagram")

## Display Wemos - wiring diagram:
![alt text](https://github.com/ajithvasudevan/water-level-indicator/raw/master/Display%20Wemos%20-%20wiring%20diagram.png "Display Wemos - Wiring Diagram")
