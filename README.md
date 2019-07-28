ManiaBoard (Osu! Keyboard)
==========================

A multi-functional gaming keyboard.

This has been for over a year before uploaded here(and everywhere else), and still WIP.

..For that reason, the code become really massive and finally filled up 99% of Pro Micro's storage. Ah, great.

Uses Arduino Pro Micro(Arduino Leonardo), WS2812B LED strip, Kalih Sockets.

![20190727_104836](https://user-images.githubusercontent.com/44800937/61988606-6c2fd800-b05e-11e9-9c30-4bb0c95662bd.png)

Arduino's usb socket become loose over time with my intense gameplay and caused occasional disconnection.
So I decided to use jumper wires instead until the next build. Yes, I'm lazy.

Features
--------

- 6 Keys + 2 Function keys
  - Simultaneous input up to 12 keys (Keyboard.h library modified)
  - 350Hz polling rate on average (Up to 400Hz)
  - Fast assembly-level pin reading
  - Fast debouncing
- Exchangeable Cherry keys (Kalih socket)
  - Easy to repair
- Custom Keycaps
  - Lightweight and durable
- Anti-Slip Pads
  - Works with cheap rubber pads
- LED for each 6 keys
  - Changes color by key press
  - Changes color by KPS(Keys Per Second)
  - 4 different LED modes
  - Fast data transfer code
- Multiple Keymaps
  - With distinct LED color
- Built-in CUI Menu
  - Change key mapping
  - Manage keymap slots
  - Multilingual menu (WIP)
  
Manual
------

1. Controls
```
          +----+----+
 (Arduino)| K1 | K2 | K7
+----+----+----+----+
| K3 | K4 | K5 | K6 | K8
+----+----+----+----+
```
- K1 - K6
  - Normal Keys
- K7 : Function Key 1
  - Short : K7
  - Long : Reset KPS display(K2 LED)
- K8 : Function Key 2
  - Short : K8
  - Long : Cycle keymaps
- K7 & K8
  - Short : Enter settings (open cmd or any text editor to print)
  - Long : Cycle LED mode

2. CUI Menu
```
          +----+----+
 (Arduino)| UP |    |
+----+----+----+----+
|    | BK | DN | OK |
+----+----+----+----+
```
 - UP : Up
   - Move up from current menu
   - Increase value
 - DN : Down
   - Move down from current menu
   - Decrease value
 - BK : Back
   - Exit from current menu
   - Cancel operation (WIP)
 - OK : Ok
   - Go into selected menu
   - Confirm

Hold UP or DN for fast control.

3. LED Modes

The below map indicates which LED mode is set when it's cycling.
e.g. K6 LED is lit = Mode 4
```
          +----+----+
 (Arduino)|    |    |
+----+----+----+----+
| M1 | M2 | M3 | M4 |
+----+----+----+----+
```
 - Mode 1 : Bright Mode
   - Shows current KPS if not pressed
   - Shows invert of current color if pressed
   - K2 LED as Max KPS indicator
 - Mode 2 : Dark Mode
   - Shows nothing if not pressed (except for K2)
   - Shows current KPS if pressed
   - K2 LED as Max KPS indicator
 - Mode 3 : Mood Mode
   - Shows nothing if pressed or not (except for K2)
   - K2 LED as Max KPS indicator, but without the flash effect
 - Mode 4 : Blackout Mode
   - Shows nothing if pressed or not
   - But Max KPS is still being calculated (displayed on the settings menu)
   
LED shows current keymap color when it's cycling keymaps, regardless of which LED mode is set.
   
Schematic
---------
```
Dxx = Arduino digital Pin number
[KS] = Kalih Socket
[PB] = Push Button
330R = 330ohm resistor
100uF = 100uF capacitor

        GND
         |
D02-[KS]-+  //K1
	 |
D03-[KS]-+  //K2
	 |
D04-[KS]-+  //K3
	 |
D05-[KS]-+  //K4
	 |
D06-[KS]-+  //K5
	 |
D07-[KS]-+  //K6
	 |
D14-[PB]-+  //K7
	 |
D15-[PB]-+  //K8
         |
	 |  ############################
         +--gnd                        #
D10-330R----din  WS2812B Strip (6LEDs) #
         +--vin                        #
	 |  ############################
         |
	+5V--100uF--GND
```

TODO
----

- Complete the menu
- Optimize everything

