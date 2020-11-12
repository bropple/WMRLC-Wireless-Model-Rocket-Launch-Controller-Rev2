# Wireless Model Rocket Launch Controller (WMRLC)

The WMRLC is an ESP8266-based, Wi-Fi enabled embedded system designed to launch model rockets safely from a distance without needing to lay down long lengths of wire. The Wi-Fi connection has been tested to work for at least 30ft with line of sight. The main goals of the project are as follows:

  - Ensure safety of the user
  - Simplify design as much as possible to lower costs and make constructing the device easier for beginners
  - Use a Wi-Fi client device for the controls, allowing almost any Wi-Fi enabled device to be used

Instructions for software installation, and a short tutorial for the hardware construction for those who need it, are in the following sections. A circuit schematic and BOM are available in the Resources folder. Datasheets for the major components are also included in the Resources folder, I recommend reading them to double check the pin connections for the diodes, relay, and the 2N3904 transistor.

Feel free to fork this project and modify the software and circuit schematic to fit your needs at your own risk. Test everything thoroughly before launching an actual rocket!

Note: when launching rockets, keep the system away from the rocket's exhaust. If a battery is hit directly, it could cause a thermal runaway and the battery will start a fire.

---

# How to install the software on the NodeMCU
This program was made using the Arduino IDE to simplify programming the device. There are some things you have to do in order to compile and upload the program:

 * Download and Install the Arduino IDE if needed, and follow the tutorial [here](https://create.arduino.cc/projecthub/najad/using-arduino-ide-to-program-nodemcu-33e899) to get the NodeMCU working with the Arduino IDE
 * Install the following libraries in Sketch > Include library >  Add .ZIP Library:
   * [MCP342x by Steve Marple](https://github.com/stevemarple/MCP342x) - for using the I2C ADC
   * [ESPAsyncWebServer by me-no-dev](https://github.com/me-no-dev/ESPAsyncWebServer) - Asynchronous HTTP and WebSocket server
 * Open the WirelessRocketLaunchCtrl.ino file in the Arduino IDE
 * Select "NodeMCU 1.0" under Tools > Board
 * Match settings to this image. Make sure you select the port your NodeMCU is on (COM port for Windows or tty for Linux/OSX)
 ![](/img/idemenu.png "Arduino IDE NodeMCU Settings")
 * Upload!
---

# Using the WMRLC

 After you upload the code, plug the NodeMCU into the circuit, and turn the power switch ON, a blue LED will turn on if the webserver starts successfully. Go to your phone or computer's WiFi settings and connect to the "BEN-CO_Rocket_Launch_Ctrlr" network, or the SSID you set. "ROCKETMAN" is the default password for the network. Upon login, you should get a notification to log in to the network. Depending on the device, you will either be automatically redirected to the launch control page or you will have to click/tap on the notification. If you get neither of these notifications, go on your device's web browser and type "8.8.8.8" or "http://8.8.8.8" into the address bar and the launch controls will be shown.
 


Desktop            |  Mobile
:-------------------------:|:-------------------------:
![](/img/desktop.png)  |  ![](/img/mobile.jpg)

 
 To launch the rocket, you must first check the continuity of the circuit by pressing the top button. The launch button is disabled at this point and it won't do anything if it's clicked. The website will update accordingly if the continuity check passes or fails:
 
Continuity PASS           |  Continuity FAIL
:-------------------------:|:-------------------------:
![](/img/pass.jpg)  |  ![](/img/fail.jpg)
 
 If the continuity check fails, the reason why it failed will be shown under the System Messages section and the launch button will remain inactive. If it succeeded, the launch button will be active. When you press the launch button, the continuity is quickly and automatically checked two more times, just in case something somehow happens to the leads in between the manual check and the launch command.
 
 After launching a rocket successfully, the site buttons reset to their default functions, once again not allowing a launch to occur until a manual continuity check is performed. At this point, it is safe to set up a new rocket.
 
 One thing to note is that it is still possible to have a bad igniter that doesn't launch the rocket successfully. If this happens, the procedure is the same as any other launch controller: wait 60 seconds to replace the igniter after a failed launch.
 
 If the system reports a FAILED status erroneously, you may need to adjust the ADC calibration values. This will be shown along with other errors in the Troubleshooting section.
 
 A list of devices I have tested with the launch controller is shown below:
 
 Device          |  Site Access at 8.8.8.8   | Does auto login work? | Notes
:-------------------------:|:-------------------------:|:----------------------:|:-------------------:
Android 8.1 Phone |  Yes | Yes | Automatically pops up when Wi-Fi is connected.
Android 9.0 Tablet | Yes | Yes | Tap on notification to be brought to page.
iOS 12 Phone | Yes | No | Automatic redirect failed. I think Apple uses a different DNS?
Manjaro Linux Laptop | Yes | Yes | Seems to have no problems with the DNS. |
Windows 10 Laptop | Yes | No | Probably the DNS again. |
Windows Phone | Yes | No | Non-Google DNS? |


---

# How to construct the hardware
This section has a few tips and general advice for making the hardware for those who are less experienced with electronics. Device datasheets are included in the Resources/Datasheets folder if you need them, they are helpful for finding the transistor and ADC pin designations.

There are some basic tools you'll need as a prerequisite for this project. They don't need to be top of the line, but they are needed for assembly, verification, and troubleshooting:

 - Soldering Iron w/ conical tip and bevel tip
 - Rosin-core solder
 - A multimeter for measuring voltages and checking continuity between nodes
 - Solder wick or desoldering pump, for fixing soldering mistakes

If you are new to soldering and/or using a multimeter, I highly recommend watching YouTube videos to learn how to use the tools safely and correctly.

 * A bill of materials (BOM) was created to assist in finding the right parts for the project. Take it as a recommendation and do some research of your own - especially for parts marked with an asterisk. Better deals may be found on sites like Mouser, Ebay, Amazon, etc. It may also help to order multiple ADCs and NodeMCU's in case they are accidentally damaged.

* There are several different versions of the NodeMCU available. I recommend using a "NodeMCU Lolin V3" board so the pin assignments on the schematic match.

* When soldering the I2C ADC to the adapter, use the fine solder tip and be careful not to "bridge" the pins. Use the multimeter and ensure all the pins are where you expect them, and check each pin using continuity mode for short circuits. You can use the bevel tip for the rest, as it dissipates heat better for larger components.

* For nodes where the ignition battery voltage will be present at some point (the launch circuit, battery connector) a lower gauge wire should be used. I designed the circuits so 15V is the maximum safe ignition voltage, so 18 gauge wire should be used as the highest gauge for this part. For the 5V battery, you can use a higher gauge wire (< 28 gauge). For other connections to ADCs and general routing, 30 gauge wire will be OK as these are lower current. Use [this link](https://www.powerstream.com/Wire_Size.htm) as a reference for wire gauges and current rating (under "chassis wiring").

I'm leaving the enclosure options up to you, and your desired solderboard size. It's easier to make this on a larger solder board, but this makes the device less compact. To give you an idea of how I laid things out for my project, some photos from the previous revision are shown below. I used a crayon box from Wal-Mart as a simple enclosure and used a Dremel to cut holes where I needed them. I was able to fit everything on a 5x7cm solder board. I experimented with using N-channel MOSFETs to switch the ADC inputs, but this isn't necessary and only increases cost with no real benefit.

 ![](/img/inside.jpg "Inside the enclosure")
 To secure everything inside this enclosure, I used a lot of hot glue. It's ugly, but it's non-conductive, cheap, and can also help seal and insulate the circuitry. Just make sure everything's wired correctly before gluing it in, or it will be harder to remove and fix. I used a simple barrel jack for my ignition battery connector, but I recommend using another terminal block and then attaching the proper connector for your battery to that.

 ![](/img/underboard.jpg "Underneath the NodeMCU")
 I put the I2C ADC and the voltage dividers underneath the NodeMCU. The female header pins elevate the microcontroller board up off the solderboard, which allows for more compact design. This also allows the NodeMCU to be removed and used for something else. If you don't need to remove it, you can just solder the NodeMCU directly to the solderboard, but you will probably need something bigger than 5x7cm.
 
![](/img/front.jpg "The front side of the launch controller")
I used a heat sink on the power diode, this isn't strictly necessary since the high current only flows through it for a short time. The terminal block shown here is where I connect my alligator clip wires and the igniter. The switch shown is the system power switch. This old version doesn't have a safety switch, since I figured the relay would be enough, but the manual safety switch is a good addition in case the microcontroller has an issue, or the relay somehow gets stuck in the open position.

It is important that the internal battery (connected to Vin) is 5V. This can be accomplished with the 18650 battery and shield listed in the BOM, as this takes care of boosting the output to 5V and recharging the battery. Other batteries can be used with DC switching converters to obtain the 5V output. This can also be changed, however, if the 5V voltage divider values are changed so the battery voltage is reduced to ~1V. Some ESP8266 development boards include a voltage divider that works up to 3.3V and some do not, so I included the voltage divider anyway. This would only mean you have to change the 5V ADC calibration value in the program.

For the launch battery, I recommend using batteries with a voltage greater than 6V and a high discharge rating (>2A). Lipo batteries, commonly used for RC cars, would work great for this purpose. Ni-Mh Airsoft batteries or even a car battery would also work well. I have found that standard 9V batteries only cause the igniter to smoke slightly (possibly due to a high internal resistance).

Be careful wiring the launch battery, as there is no reverse polarity protection! If your launch battery comes with a special connector (Deans connector, Tamiya connector, etc.), I recommend getting the same connector with the opposite gender to ensure the polarity is correct every time you plug it in.

---

# Troubleshooting
It is highly recommended to test the system using a power resistor, DC lightbulb, or an igniter before using it to actually launch rockets. You can test the continuity of an igniter without destroying it (don't press the launch button).

## To have the ADC voltage values displayed continuously in the serial monitor
* Call the Continuty_Check(); function inside the loop() function so it is called over and over again. Add in delay(5000); to add a 5 second delay between the messages. Connect the NodeMCU to the PC and upload the code again with this modification. Leave the NodeMCU connected via USB and turn on the serial monitor to view the debug messages.

## Software Errors
```
ADC Conversion Failure!
```
* Something went wrong with the I2C ADC voltage measurement. Make sure each channel of the ADC is connected to the correct place as shown in the schematic. Double-check the ADC SDA and SCL wiring and make sure those are connected in the right order.

```
Please connect a 6-12V ignition battery!
```
* The ignition battery is not present. Attach a launch battery of the proper voltage. 

```
Ignition battery voltage is too low!
```
* The ignition battery is under 6V, which is the rated minimum voltage for a standard Estes igniter.

```
Internal battery voltage is too low! Is the switch on?
```
* Usually only happens when you're running the NodeMCU off USB power and forget to turn the power switch on. If not, the internal battery is under 4V and is severely low, and needs to be increased to 5V.

```
Short circuit! Check igniter connection!
```
* The circuit is shorted, because the differential voltage is near zero, indicating the nodes across the igniter port are connected.

```
Open circuit or High Resistance! Check igniter connection!
```
* There is no igniter attached or a lead has fallen off, because the differential voltage is large.
```
An unknown error has occurred!
```
* An unexpected error has occurred. This can only happen if the Continuity_Check() function somehow returns a value outside the range of -1 - 5.

## Other errors

### The system's continuity check fails even though everything is attached correctly
* Don't forget to turn on the system power if your NodeMCU is connected to your computer over USB.
* If the voltage readings are incorrect, adjust the ADC calibration values on lines 58 and 59 until they are close to the actual voltage readings on your multimeter. 
* If it still isn't passing and the voltages are correct, try altering the conditions in the Continuity_Check() function. You shouldn't have to lower the short circuit value any more.

### The blue LED doesn't turn on or blinks when the power switch is turned on
* Double-check the wiring of the internal battery. Make sure it's connected to the VIN pin.
* The ADC isn't initializing. A message will appear in the serial monitor to confirm this. Check the wiring of the I2C ADC. If it is correct, upload this [I2C Scanner](http://www.esp8266learning.com/i2c-scanner.php) to find the I2C address of the ADC. It may be different for some IC batches.

### The system's continuity check passes when it shouldn't
* Double-check your wiring. This isn't likely to happen if everything is connected properly.
* If everything is correct, check the voltage levels on the serial monitor. you can increase the short circuit threshold voltage in the Continuity_Check() function. The differential voltage should be very close to zero if there is a shorted circuit. You could also increase the open circuit voltage threshold.

### Cannot upload program to the NodeMCU
* Make sure none of the pins on the NodeMCU are bridged with solder, this can sometimes happen.
---

# Future Software Revisions
I plan on updating the software for this version of the device at least a few times. I plan to add the following features:
* Measure differential voltage on client login to make sure the ignition voltage isn't present on the ignition line before flipping the safety switch. There's a low probablility of the relay failing like this, but still good to check.
* Try and get auto-login to work on iOS
* Visual updates to the web interface
* Find and fix software bugs

---

# Future Hardware Revisions
While not immediately possible for me, I do plan on making future revisions of the hardware someday:
 * Make a custom PCB using the ESP8266 that doesn't rely on a development board
 * Use more surface-mount components to further reduce system size
 * Increase accuracy of the ADC
 * Add reverse polarity protection for the launch battery
 * Add more LEDs to indicate system status on the hardware itself
 * Increase Wi-Fi range beyond 30 ft using an external antenna
 * Add a piezoelectric buzzer to make a sound for continuity, and also use it for a software countdown sequence
 * Add more ports for more rockets to be connected at once, with continuity checking available for all of them. Still only allow for one rocket to be launched at any time, and add a software timer to disallow rapid firing.
 * Add the ability to switch the ADC inputs using multiplexing or a different method in order to measure more node voltages than is normally possible with a 2-channel ADC
 * Internal battery recharging circuitry for a low-profile lithium-ion battery
 * PCB desined to fit a custom-designed 3D-printed enclosure

While these revisions will make it more difficult for beginners to assemble, it will make the system more reliable and versatile, and perhaps more attractive for model rocket clubs with many members.
