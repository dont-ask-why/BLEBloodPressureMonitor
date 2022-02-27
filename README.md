## Blood Pressure Monitor WiFi Interface - ESP35

This is the Arduino code for a ESP32 used to add BLE capability to a Panasonic blood presure monitor.

Please check out the original project at http://www.edusteinhorst.com/hacking-a-blood-pressure-monitor/

## Disclaimer

This is a very rough initial version of the code. Don't expect reliability. Also, keep in mind you're modifying a medical device and that CAN interfere with it's accuracy and reliability. Remeber that a WiFi device emits RF which can affect other devices. If you choose to modify it, don't use it as a medical device anymore. Use this code at your own risk. I cannot be held liable if, knock on wood, your house burns down our your dog dies!

## Changes by the fork

I build a [small pulseoximeter w/ an app](https://github.com/dont-ask-why/Picoximeter) and thought: I could add a blood pressure monitor to that!
My connection uses BLE from an ESP32 so I can use fancy features like the integrated step down circuit or the deep sleep functionality.
Luckily I found the same device online for easy implementation.

## License

This project is licensed under the terms of the MIT license.
