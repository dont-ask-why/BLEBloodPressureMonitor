## Blood Pressure Monitor WiFi Interface - ESP32

This is the Arduino code for a ESP32 used to add BLE capability to a Panasonic blood presure monitor.

Please check out the original project at http://www.edusteinhorst.com/hacking-a-blood-pressure-monitor/ where I got the idea. I checked most of the data and code with my own devices and modified the code for BLE (instead of WiFi) transmission.

## Disclaimer

This is a very rough initial version of the code. Don't expect reliability. Also, keep in mind you're modifying a medical device and that CAN interfere with it's accuracy and reliability. Remember that a WiFi (well in this case it's BLE) device emits RF which can affect other devices - and powering a microcontroller could also affect analog reference voltages. In general if you choose to modify it, don't use it as a medical device anymore - there are some regulations about modifying medical devices and using them but from my I learned by stuying this sort of stuff I would say they don't apply here. Use this code at your own risk. I cannot be held liable if, knock on wood, your house burns down our your dog dies (Disclaimer by d.a.w.: plz dont't use on dog but willing humans only)!

## Changes by the fork

I build a [small pulseoximeter w/ an app](https://github.com/dont-ask-why/Picoximeter) and thought: I could add a blood pressure monitor to that!
My connection uses BLE from an ESP32 so I can use fancy features like the integrated step down circuit and the deep sleep functionality.
Luckily I found the same device online for easy implementation.

So to conclude: my wired connections are basically the same but to another microcontroller and the code works the same way but with a different transmission standard. I also found some behaviour where some byte at 129 as well as 128 indicated an uneven value so I added that functionality as well.

## License

This project is licensed under the terms of the MIT license.
