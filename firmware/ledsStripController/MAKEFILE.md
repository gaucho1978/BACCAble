# BACCAble Makefile

Base makefile to build and flash without STM32 IDE so you can use your favorite IDE

## Building

Firmware builds with GCC. Specifically, you will need gcc-arm-none-eabi, which
is packaged for Windows, OS X, and Linux on
[Launchpad](https://launchpad.net/gcc-arm-embedded/+download). Download for your
system and add the `bin` folder to your PATH.

Your Linux distribution may also have a prebuilt package for `arm-none-eabi-gcc`, check your distro's repositories to see if a build exists.

Just you can compile using `make`. If succesfull you'll get a summary of the build.

## Building default flavors

For BH bus
```
$ CFLAGS="-DRELEASE_FLAVOR=BH_FLAVOR -DBH_FLAVOR=1" make clean all
```

For Diesel C1 bus
```
$ CFLAGS="-DRELEASE_FLAVOR=C1_FLAVOR -DC1_FLAVOR=1" make clean all
```

For Gasoline C1 bus
```
$ CFLAGS="-DRELEASE_FLAVOR=C1_FLAVOR -DIS_GASOLINE -DC1_FLAVOR=1" make clean all
```

For C2 bus
```
$ CFLAGS="-DRELEASE_FLAVOR=C2_FLAVOR -DC2_FLAVOR=1" make clean all
```

```
$ CFLAGS="-DRELEASE_FLAVOR=CAN_FLAVOR -DCAN_FLAVOR=1" make clean all
```


## Flashing with the Bootloader

Simply plug in your CANable with the BOOT jumper enabled (or depress the boot button on the CANable Pro while plugging in). Next, type `make flash` and your CANable will be updated to the latest firwmare. Unplug/replug the device after moving the boot jumper back, and your CANable will be up and running.

Please note that the flashed binary will be `./build/baccable-$(GIT_VERSION).bin`

## Contributors

- [Ethan Zonca](https://github.com/normaldotcom) - This makefile is just an adaption of the one crafted by Ethan for his SLCAN firmware (https://github.com/normaldotcom/canable-fw)
