# Building the firmware

## ESP32 firmware

Before building the RP2040 firmware, you need to build the ESP32 firmware (since it's included in the RP2040 firmware). 
The ESP32 firmware uses ESPHome. A build script has been provided, which partially uses Docker. It does however also 
require the following CLI tools to be available in `$PATH`:

* xxd 
* crc32
* md5
* sed
* [bin2header](https://github.com/AntumDeluge/bin2header)

### Building

The checked-in version of `smart-lcc.yaml` references a `secrets.yaml`, for Wi-fi SSID and password, and API Key. It is 
possible to use a captive portal or other Wi-fi provisioning option from ESPHome, but for 
simplicity, adding these to a `secrets.yaml` is recommended.

Here's an example:

```yaml
wifi_ssid: "Your SSID"
wifi_password: "a-super-secret-password"

# This is a 32-byte base64 encoded string, generate one on your own, or get one on https://esphome.io/components/api.html
api_key: "XUK5d9/mV4m2GlCiV1/j9/vy+lR2bcU2R71D84OY4g0="
```

Once you have a `secrets.yaml`, run the following command in your favorite shell:

```bash
$ esphome/compile.sh
```

## RP2040 firmware

The RP2040 firmware is entirely custom. It is built using the standard pico-sdk toolchain, which
is built on `cmake`. It uses `PICO_SDK_FETCH_FROM_GIT`, so you don't need to have a system-wide install of pico-sdk, 
but you do need an install of the GCC ARM Embedded toolchain. On a Mac, the easiest way to install it, is to use
homebrew: 

```bash
$ brew install gcc-arm-embedded
```

Once `cmake` and `gcc-arm-embedded` are installed, use cmake to build the firmware:

```bash
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

The project is compatible with Jetbrains CLion, and can be built using it.

If you have `picotool` installed on the `$PATH`, you can use the `Upload-Picotool` target to build and upload (over USB). 
This is the recommended way to upload the firmware over USB.

If you have a Mac and SEGGER J-Link (and have soldered an SWD interface onto your Arduino RP2040 Connect), you can use the 
`Upload-JLink` target to build and upload over J-Link. 

In other case, the build produces a `.uf2` file, which you can upload using the RP2040 USB Bootloader.