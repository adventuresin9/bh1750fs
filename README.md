# bh1750fs
9Front / Plan9 file system for the BH1750 Ambient Light Sensor

https://www.adafruit.com/product/4681

By default, it posts a file to /srv called bh1750, and in the current namespace will mount a directory called bh1750 to /mnt.  In that directory will be a file called "lux", and reading that will give the current lux reading of the sensor.
