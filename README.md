# TinyRemote - IR Remote Control based on CH32V003
TinyRemote is a very simple 5-button IR remote control based on the cost-effective CH32V003 RISC-V microcontroller powered by a CR2032 or LIR2032 coin cell battery.

- Project Video (YouTube, ATtiny13A version): https://youtu.be/ad3eyNCov9c

![IR_Remote_pic1.jpg](https://raw.githubusercontent.com/wagiminator/CH32V003-IR-Remote/main/documentation/IR_Remote_pic1.jpg)

For the ATtiny13A-based version of the IR remote control visit [TinyRemote](https://github.com/wagiminator/ATtiny13-TinyRemote). For the 12-button version of the ATtiny13A-based IR remote control see [TinyRemoteXL](https://github.com/wagiminator/ATtiny13-TinyRemoteXL).

# Hardware
## Schematic
![IR_Remote_wiring.png](https://raw.githubusercontent.com/wagiminator/CH32V003-IR-Remote/main/documentation/IR_Remote_wiring.png)

## The CH32V003 Family of 32-bit RISC-V Microcontrollers
The CH32V003 series is a collection of industrial-grade general-purpose microcontrollers that utilize the QingKe RISC-V2A core design supporting the RV32EC instruction set. These microcontrollers are equipped with various features such as a 48MHz system main frequency, 16KB flash, 2KB SRAM, wide voltage support, a single-wire serial debug interface, low power consumption, and an ultra-small package. Additionally, the CH32V003 series includes a built-in set of components including a DMA controller, a 10-bit ADC, op-amp comparators, multiple timers, and standard communication interfaces such as USART, I2C, and SPI.

# Software
There are various communication protocols for infrared remote controls, most of which use a carrier wave between 30kHz and 58kHz, depending on the protocol. This carrier wave is generated using pulse-width modulation (PWM) at the IR diode, helping the receiver distinguish the signal from noise. The IR signal is modulated onto the carrier wave using pulse code modulation (PCM) by turning the IR LED on and off in a specific pattern. The telegrams consist of a start frame, the receiver's device address, and the command for the key pressed. This IR Remote Control implements the four most widely used protocols:

|Protocol|Carrier Frequency|Encoding Method|Start Frame|Address|Command|
|:-|:-|:-|:-|:-|:-|
|NEC|38kHz|Pulse Distance|9ms burst / 4.5ms space|8/16 bits|8 bits|
|Samsung|38kHz|Pulse Distance|4.5ms burst / 4.5ms space|8 bits|8 bits|
|RC-5|36kHz|Manchester|Start bits|5 bits|6/7 bits|
|Sony SIRC|40kHz|Pulse Length|2.4ms burst / 0.6ms space|5/8/13 bits|7 bits|

## NEC Protocol
Timer1 generates a 38kHz carrier frequency with a 25% duty cycle on the output pin connected to the IR LED. The IR telegram is modulated by toggling the IR LED pin between output high and output alternate. Setting the pin to output alternate enables PWM on this pin, sending a burst of the carrier wave. Setting the pin to output high turns off the LED completely. 

The NEC protocol uses pulse distance encoding, where a data bit is defined by the time between bursts. A "0" bit consists of a 562.5µs burst (LED on: 38kHz PWM) followed by a 562.5µs space (LED off). A "1" bit consists of a 562.5µs burst followed by a 1687.5µs space.

An IR telegram starts with a 9ms leading burst followed by a 4.5ms space. Then, four data bytes are transmitted, with the least significant bit first. A final 562.5µs burst signifies the end of the transmission. The four data bytes are transmitted in the following order:
- the 8-bit address for the receiving device,
- the 8-bit logical inverse of the address,
- the 8-bit command, and
- the 8-bit logical inverse of the command.

![NEC_transmission.png](https://techdocs.altium.com/sites/default/files/wiki_attachments/296329/NECMessageFrame.png)

The Extended NEC protocol uses 16-bit addresses. Instead of sending an 8-bit address and its logical inverse, it sends the low byte first, followed by the high byte of the address.

If the remote control button is held down, a repeat code is sent. This code consists of a 9ms leading burst, a 2.25ms pause, and a 562.5µs burst to mark the end. The repeat code is sent every 108ms until the button is released.

![NEC_repeat.png](https://techdocs.altium.com/sites/default/files/wiki_attachments/296329/NECRepeatCodes.png)

## Samsung Protocol
The SAMSUNG protocol corresponds to the NEC protocol, except that the start pulse is 4.5ms long and the address byte is sent twice. The telegram is repeated every 108ms as long as the button is pressed.

## Philips RC-5 Protocol
The Philips RC-5 protocol uses Manchester encoding with a 36kHz carrier frequency. A "0" bit is an 889µs burst followed by an 889µs space, while a "1" bit is an 889µs space followed by an 889µs burst. 

An IR telegram starts with two start bits: the first bit is always "1", and the second bit is "1" in the original protocol or the inverted 7th bit of the command in the extended RC-5 protocol. The third bit toggles after each button release. The next five bits are the device address, and the last six bits are the command, all transmitted with the most significant bit (MSB) first.

![RC5_transmission.png](https://techdocs.altium.com/sites/default/files/wiki_attachments/296330/RC5MessageFrame.png)

As long as a key remains down the telegram will be repeated every 114ms without changing the toggle bit.

## Sony SIRC Protocol
The Sony SIRC protocol uses pulse length encoding with a carrier frequency of 40kHz. A "0" bit consists of a 600µs burst followed by a 600µs space, while a "1" bit consists of a 1200µs burst followed by a 600µs space. An IR telegram starts with a 2400µs leading burst followed by a 600µs space. The command and address bits are then transmitted, with the least significant bit (LSB) first. The specifics of the command and address bits depend on the protocol version as follows:
- 12-bit version: 7 command bits, 5 address bits
- 15-bit version: 7 command bits, 8 address bits
- 20-bit version: 7 command bits, 5 address bits, 8 extended bits

As long as a key remains down the telegram will be repeated every 45ms.

## Power Saving
The code uses the standby power-down function, waking up whenever a button is pressed, triggered by a pin falling edge event.

While no button is pressed, the CH32V003 stays in standby power-down mode, consuming about 9µA at 3V. A typical CR2032 battery has a capacity of 230mAh, resulting in a theoretical battery life of over 25,000 hours, or nearly 3 years. However, actual battery life will be shorter due to self-discharge. When a button is pressed, the current can spike up to 25mA. The diagram below shows the current consumption when a button is pressed and an NEC telegram is sent, measured with the [Power Profiler Kit II](https://www.nordicsemi.com/Products/Development-hardware/Power-Profiler-Kit-2):

![IR_Remote_current.png](https://raw.githubusercontent.com/wagiminator/CH32V003-IR-Remote/main/documentation/IR_Remote_current.png)

When sending a NEC telegram, the current consumption averages around 3mA for 71ms. In theory, a single battery could send almost 4 million telegrams. However, rechargeable LIR2032 batteries have a much lower capacity.

By the way, although 9µA in standby mode seems low, the [ATtiny13A](https://github.com/wagiminator/ATtiny13-TinyRemote) uses only about 150nA, which is 60 times less!

# Compiling and Uploading Firmware
## Defining the Key Commands
Before compiling and uploading the firmware, the desired IR commands must be assigned to the respective buttons. This is done by editing the *config.h* file. Multiple commands of different protocols can be assigned to a single button. These commands should be separated by semicolons. When the button is pressed, the commands will be executed sequentially.

## Programming and Debugging Device
To program the CH32V003 microcontroller, you will need a special programming device which utilizes the proprietary single-wire serial debug interface (SDI). The [WCH-LinkE](http://www.wch-ic.com/products/WCH-Link.html) (pay attention to the "E" in the name) is a suitable device for this purpose and can be purchased commercially for around $4. This debugging tool is not only compatible with the CH32V003 but also with other WCH RISC-V and ARM-based microcontrollers.

![CH32V003_wch-linke.jpg](https://raw.githubusercontent.com/wagiminator/Development-Boards/main/CH32V003F4P6_DevBoard/documentation/CH32V003_wch-linke.jpg)

To use the WCH-LinkE on Linux, you need to grant access permissions beforehand by executing the following commands:
```
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="8010", MODE="666"' | sudo tee /etc/udev/rules.d/99-WCH-LinkE.rules
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="8012", MODE="666"' | sudo tee -a /etc/udev/rules.d/99-WCH-LinkE.rules
sudo udevadm control --reload-rules
```

On Windows, if you need to you can install the WinUSB driver over the WCH interface 1 using the [Zadig](https://zadig.akeo.ie/) tool.

To upload the firmware, you need to ensure that the battery is removed. Then, you should make the following connections to the WCH-LinkE:

```
WCH-LinkE      IR Remote
+-------+      +-------+
|  SWDIO| <--> |DIO    |
|    GND| ---> |GND    |
|    3V3| ---> |3V3    |
+-------+      +-------+
```

If the blue LED on the WCH-LinkE remains illuminated once it is connected to the USB port, it means that the device is currently in ARM mode and must be switched to RISC-V mode initially. There are a few ways to accomplish this:
- You can utilize the Python tool *rvprog.py* (with *-v* option), which is provided with the firmware in the *tools* folder.
- Alternatively, you can select "WCH-LinkRV" in the software provided by WCH, such as MounRiver Studio or WCH-LinkUtility.
- Another option is to hold down the ModeS button on the device while plugging it into the USB port.

More information can be found in the [WCH-Link User Manual](http://www.wch-ic.com/downloads/WCH-LinkUserManual_PDF.html).

## Compiling and Uploading Firmware using the Makefile
### Linux
Install the toolchain (GCC compiler, Python3, and PyUSB):
```
sudo apt install build-essential libnewlib-dev gcc-riscv64-unknown-elf
sudo apt install python3 python3-pip
python3 -m pip install pyusb
```

Remove the battery from the device. Connect the IR Remote Control via the 3-pin PROG header to the WCH-LinkE programming device. Do not press any button on the remote control while the programmer is connected! Open a terminal and navigate to the folder with the makefile. Run the following command to compile and upload:
```
make flash
```

If you want to just upload the pre-compiled binary, run the following command instead:
```
python3 tools/rvprog.py -f bin/ir_remote.bin
```

### Other Operating Systems
Follow the instructions on [CNLohr's ch32v003fun page](https://github.com/cnlohr/ch32v003fun/wiki/Installation) to set up the toolchain on your respective operating system (for Windows, use WSL). Also, install [Python3](https://www.pythontutorial.net/getting-started/install-python/) and [pyusb](https://github.com/pyusb/pyusb). Compile and upload with "make flash". Note that I only have Debian-based Linux and have not tested it on other operating systems.

## Compiling and Uploading Firmware using PlatformIO
- Install [PlatformIO](https://platformio.org) and [platform-ch32v](https://github.com/Community-PIO-CH32V/platform-ch32v). Follow [these instructions](https://pio-ch32v.readthedocs.io/en/latest/installation.html) to do so. Linux/Mac users may also need to install [pyenv](https://realpython.com/intro-to-pyenv).
- Click on "Open Project" and select the firmware folder with the *platformio.ini* file.
- Remove the battery from the IR Remote Control. Connect the WCH-LinkE to the board. Do not press any button on the remote control while the programmer is connected! Then click "Upload".

## Uploading pre-compiled Firmware Binary
WCH offers the free but closed-source software [WCH-LinkUtility](https://www.wch.cn/downloads/WCH-LinkUtility_ZIP.html) to upload the precompiled hex-file with Windows. Select the "WCH-LinkRV" mode in the software, open the *ir_remote.hex* file in the *bin* folder and upload it to the microcontroller.

Alternatively, there is a platform-independent open-source tool called minichlink developed by Charles Lohr (CNLohr), which can be found [here](https://github.com/cnlohr/ch32v003fun/tree/master/minichlink). It can be used with Windows, Linux and Mac.

If you have installed [Python3](https://www.pythontutorial.net/getting-started/install-python/) and [pyusb](https://github.com/pyusb/pyusb) on your system, you can also use the included Python tool *rvprog.py*:
```
python3 tools/rvprog.py -f bin/ir_remote.bin
```

# References, Links and Notes
- [EasyEDA Design Files](https://oshwlab.com/wagiminator)
- [MCU Templates](https://github.com/wagiminator/MCU-Templates)
- [MCU Flash Tools](https://github.com/wagiminator/MCU-Flash-Tools)
- [WCH: CH32V003 Datasheet](http://www.wch-ic.com/products/CH32V003.html)
- [CNLohr: ch32v003fun](https://github.com/cnlohr/ch32v003fun)
- [San Bergmans: IR remote control explanations](https://www.sbprojects.net/knowledge/ir/index.php)
- [Christoph Niessen: IR remote control (german)](http://chris.cnie.de/avr/tcm231421.html)
- [David Johnson-Davies: IR remote control detective](http://www.technoblogy.com/show?24A9)
- [Altium: Infrared communication concepts](https://techdocs.altium.com/display/FPGA/Infrared+Communication+Concepts)
- [ATtiny13A NEC decoder](https://github.com/wagiminator/ATtiny13-TinyDecoder)
- [ATtiny13A TinyRemote](https://github.com/wagiminator/ATtiny13-TinyRemote)
- [ATtiny13A TinyRemote XL](https://github.com/wagiminator/ATtiny13-TinyRemoteXL)
- [ATtiny13A TinyRemote RF](https://github.com/wagiminator/ATtiny13-TinyRemoteRF)

![IR_Remote_pic2.jpg](https://raw.githubusercontent.com/wagiminator/CH32V003-IR-Remote/main/documentation/IR_Remote_pic2.jpg)
![IR_Remote_pic3.jpg](https://raw.githubusercontent.com/wagiminator/CH32V003-IR-Remote/main/documentation/IR_Remote_pic3.jpg)
![IR_Remote_pic4.jpg](https://raw.githubusercontent.com/wagiminator/CH32V003-IR-Remote/main/documentation/IR_Remote_pic4.jpg)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
