# cavelock

DIY Smart Door Lock using Raspberry Pi with Apple HomeKit integration

## Getting Started
cavelock is a D.I.Y. smart door lock that is runing on Raspberry Pi. It opens and closes a simple storm door deadbolt using Apple Home app. It is simply created for my personal uses, hoping it is useful for anyone elses.
[![cavelock demo](https://img.youtube.com/vi/mZIwY49XvS4/0.jpg)](https://www.youtube.com/watch?v=mZIwY49XvS4)

## Prerequisites
* Raspberry Pi
* A servo (SPEKTRUM A5030) and a relay to connect in to the Raspberry Pi 
* Compiler (GCC C++)
* Visual Studio 2019 (for remote debugging)
* Some knowledge on HomeKit (HAP, SRP)

## Features
* Run on Raspberry Pi Zero W in console (or systemd)
* Control a deadbolt from 90 to zero degree with a servo
* A relay shuts down the current once the servo moved to the target angle (it saves power consumption)
* A push button overrides the control of deadbolt for manual operation 
* No Home Bridge needed as HomeKit Accessary Protocol (HAP), Secure Remote Password (SRP) are included, it can be paired directly with iOS Home app

## Related Projects 
* HAP implementation relies on [Gera Kazakov](https://github.com/gera-k/uHap). It is great inspiration and I cannot thank him enough for utilizing the clean implementaion to my project

## References
* [HomeKit Accessary Protocol](https://developer.apple.com/support/homekit-accessory-protocol/)
* [Secure Remote Password](https://en.wikipedia.org/wiki/Secure_Remote_Password_protocol)

## Other hardware 
* A 5V solar cell and rechargeable batteries
* AutoDesk Fusion 360 for 3D modeling of container

## Test
* Tested only with RPi Zero Wireless, and a SPEKTRUM A5030 servo

## License
* MIT License
