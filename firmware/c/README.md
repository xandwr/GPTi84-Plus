# Pico W C firmware

Build with the [Pico SDK](https://github.com/raspberrypi/pico-sdk). Requires
`PICO_SDK_PATH` to be set; the user `.zshrc` already exports
`/usr/share/pico-sdk` (Arch package `pico-sdk`).

## Build

```sh
cd firmware/c/build
cmake ..
make -j
```

## Flash

Hold BOOTSEL while plugging the Pico W in (or short-press RESET if wired),
then either drag-drop `bringup.uf2` onto the `RPI-RP2` mass-storage device,
or:

```sh
picotool load -f bringup.uf2 && picotool reboot
```

## Observe

USB-CDC serial appears on `/dev/ttyACM0` (or `ttyACM1` if there's another
CDC device attached). Any baud — USB-CDC ignores it.

```sh
tio /dev/ttyACM0
```

## Targets

- **bringup** — GPIO loopback (GP2↔GP3 jumper), PIO pin-hold, PIO FIFO
  shift, CYW43 WiFi radio init + scan. C port of `src/test_pico_data_works.py`.
