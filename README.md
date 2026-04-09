- [GL Thread Dev Board](#gl-thread-dev-board)
  - [HW Info](#hw-info)
  - [Pinout](#pinout)
  - [SW Development Environment](#sw-development-environment)
    - [Requriements](#Requriements)
    - [Install dependencies](#install-dependencies)
    - [Download gl-nrf-sdk](#download-gl-nrf-sdk)
    - [Buiding gl-dev-board-over-thread demo](#buiding-gl-dev-board-over-thread-demo)
      - [buiding](#buiding)
      - [Flashing](#flashing)
      - [test](#test)
        - [Set LED switch](#set-led-switch)
        - [Set LED color](#set-led-color)
        - [Set the GPIO level](#set-the-gpio-level)
        - [Read the GPIO status](#read-the-gpio-status)
        - [Read LED status](#read-led-status)
    - [Buiding  other demo](#buiding--other-demo)
      - [buiding](#buiding-1)
      - [Flashing](#2flashing-1)
      - [test](#test-1)
  - [Adding a New Board to Home Assistant](#adding-a-new-board-to-home-assistant)
    - [Step 1: Flash Updated Firmware via UART DFU](#step-1-flash-updated-firmware-via-uart-dfu)
    - [Step 2: Join the Thread Network](#step-2-join-the-thread-network)
    - [Step 3: Update Firmware via Thread OTA](#step-3-update-firmware-via-thread-ota)
    - [Step 4: Add the Board to Home Assistant](#step-4-add-the-board-to-home-assistant)
  - [Recovery: Restoring Boards After GL-S200 OTA Overwrote Custom Firmware](#recovery-restoring-boards-after-gl-s200-ota-overwrote-custom-firmware)
    - [What Happened](#what-happened)
    - [Permanent Fix: Disable the S200 OTA Service](#permanent-fix-disable-the-s200-ota-service)
    - [Recovery Procedure: Re-flash Custom Firmware via USB (Ubuntu)](#recovery-procedure-re-flash-custom-firmware-via-usb-ubuntu)
    - [Commissioning on the Thread Network](#commissioning-on-the-thread-network)


# GL Thread Dev Board

Thread Dev Board (TBD) is the end device part of the thread kit developed by GL-iNet. Developers can test thread-related features and develop their own features based on the TBD.

TBD currently implements the following functions:

- Join a thread network as a **Thread Router** / **Thread End Device** / **Thread Sleepy End Device**.
- Real-time collection of current environment information and uploading to S200 gateway via thread.
    - Temperature 
    - Humidity 
    - Air pressure
    - Pyroelectric infrared
    - Rotary encoder status
    - GPIO input
- Receive command and execute operations in real time via thread
    - RGB LED on/off
    - Change RGB LED color
    - GPIO output
- Firmware upgrade
    - DFU by UART
    - DFU by thread

## HW Info

| HW part                             | Info                          |
| ----------------------------------- | ----------------------------- |
| MCU                                 | NRF52840                      |
| Temperature and humidity sensor     | SHTC3                         |
| Light sensor                        | HX3203                        |
| Air pressure and temperature sensor | SPL0601                       |
| Pyroelectric infrared sensor        | XYC-PIR203B-S0                |
| Rotary encoder                      | EC1108                        |
| RGB LED (X2)                        | LC8812B                       |
| SW define button                    | x2                            |
| HW reset button                     | x1                            |
| SW define LED                       | x2                            |
| HW power LED                        | x1                            |
| Power supply                        | Battery base(CR2032) / Type-C |

## Pinout

<img src="./docs/img/gl_thread_dev_board_pinout.jpg" alt="TDB_pinout" style="zoom: 25%;" />

## Getting Started Guide

### Requriements

- Ubuntu 20.04 LTS or later

### Install dependencies

Please refer to [Developing with Zephyr](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies) for environment setup.

### Download gl-nrf-sdk

**Support versions**

- [v2.2.0-glinet](https://github.com/gl-inet/gl-nrf-sdk/tree/v2.2.0-glinet) 

```
west init -m https://github.com/gl-inet/gl-nrf-sdk --mr v2.2.0-glinet gl-nrf-sdk
cd gl-nrf-sdk/
west update
```

### Buiding gl-dev-board-over-thread demo 

gl-dev-board-over-thread demo is used as an example.

#### buiding

```
cd glinet/gl-dev-board-over-thread
west build -b gl_nrf52840_dev_board 
```

####    Flashing

- **Using an external [debug probe](https://docs.zephyrproject.org/latest/develop/flash_debug/probes.html#debug-probes)** 

Please power on the GL Thread DEV Board. Refer to the picture below, connect SWDCLK, SWDIO and GND of the GL Thread DEV Board to the same pin of J-LINK. Then connect J-LINK to ubuntu, and use the west command to flash the firmware to GL Thread DEV Board.

<img src="./docs/img/j-link-connection.png" alt="j-link-connection" style="zoom: 100%;" />



```
west flash --erase
```

- **DFU over Serial**

The GL Thread Dev Board is built-in with MCUboot bootloader. Please refer to [UART_DFU](uart_dfu/README.md).

- **DFU over IP**

The [GL.iNET S200](https://www.gl-inet.com/products/gl-s200/) built-in with mcumgr. After gl-dev-board-over-thread demo is connected to S200, you can refer to the following command to upgrade.

```
root@GL-S200:~# mcumgr --conntype udp --connstring=[fd71:12b6:e2d0:5814:2ae8:1016:db22:4944]:1337 image list
Images:
 image=0 slot=0
    version: 1.1.10
    bootable: true
    flags: active confirmed
    hash: 3ea9079ccfad7138d86c549a010463203c64915e711e38c45d1393569304f0f9
Split status: N/A (0)

# Upload the image
root@GL-S200:~# mcumgr --conntype udp --connstring=[fd71:12b6:e2d0:5814:2ae8:1016:db22:4944]:1337 image upload /tmp/MTD.bin 
 22.00 KiB / 379.04 KiB [=======>-------------------------]   5.80% 4.49 KiB/s 01m16s
 
root@GL-S200:~# mcumgr --conntype udp --connstring=[fd71:12b6:e2d0:5814:2ae8:1016:db22:4944]:1337 image list
Images:
 image=0 slot=0
    version: 1.1.10
    bootable: true
    flags: active confirmed
    hash: 3ea9079ccfad7138d86c549a010463203c64915e711e38c45d1393569304f0f9
 image=0 slot=1
    version: 1.1.11
    bootable: true
    flags: 
    hash: 93b3858e4630bc2daccfeeb80fc7b359da50f1cf3ffedbdc2c886f0cf211542f
Split status: N/A (0)

# Set the image for next boot
root@GL-S200:~# mcumgr --conntype udp --connstring=[fd71:12b6:e2d0:5814:2ae8:1016:db22:4944]:1337 image confirm 93b3858e4630bc2daccfeeb80fc7b359da50f1cf3ffedbdc2c886f0cf211542f
Images:
 image=0 slot=0
    version: 1.1.10
    bootable: true
    flags: active confirmed
    hash: 3ea9079ccfad7138d86c549a010463203c64915e711e38c45d1393569304f0f9
 image=0 slot=1
    version: 1.1.11
    bootable: true
    flags: pending permanent
    hash: 93b3858e4630bc2daccfeeb80fc7b359da50f1cf3ffedbdc2c886f0cf211542f
Split status: N/A (0)

# Boot
root@GL-S200:~# mcumgr --conntype udp --connstring=[fd71:12b6:e2d0:5814:2ae8:1016:db22:4944]:1337 reset
Done

root@GL-S200:~# mcumgr --conntype udp --connstring=[fd71:12b6:e2d0:5814:2ae8:1016:db22:4944]:1337 image list
Images:
 image=0 slot=0
    version: 1.1.11
    bootable: true
    flags: active confirmed
    hash: 93b3858e4630bc2daccfeeb80fc7b359da50f1cf3ffedbdc2c886f0cf211542f
 image=0 slot=1
    version: 1.1.10
    bootable: true
    flags: 
    hash: 3ea9079ccfad7138d86c549a010463203c64915e711e38c45d1393569304f0f9
Split status: N/A (0)
```

You can erase unuse image slot by command,

```
root@GL-S200:~# mcumgr -t 20 --conntype udp --connstring=[fd71:12b6:e2d0:5814:2ae8:1016:db22:4944]:1337 image erase
Done
root@GL-S200:~# mcumgr --conntype udp --connstring=[fd71:12b6:e2d0:5814:2ae8:1016:db22:4944]:1337 image list
Images:
 image=0 slot=0
    version: 1.1.11
    bootable: true
    flags: active confirmed
    hash: 93b3858e4630bc2daccfeeb80fc7b359da50f1cf3ffedbdc2c886f0cf211542f
Split status: N/A (0)
```

####    test

Reference https://docs.gl-inet.com/iot/en/iot_dev_board/ add TDB(GL Thread DEV Board) to the thread network. After successfully joining the network, you can view the collected data such as temperature reported by the OTB to gl-s200 on the web page, or run commands in the background of gl-s200 to control the OTB.

##### Set LED switch

Off

```shell
coap_cli -N -e "{\"cmd\":\"onoff\",\"obj\":\"all\",\"val\":0}" -m put coap://[fd11:22:0:0:240f:8ae:bbca:47c2]/cmd
{"err_code":0}
```

On

```shell
coap_cli -N -e "{\"cmd\":\"onoff\",\"obj\":\"all\",\"val\":1}" -m put coap://[fd11:22:0:0:240f:8ae:bbca:47c2]/cmd
{"err_code":0}
```

On and off with delay

```plaintext
coap_cli -N -e "{\"cmd\":\"onoff\",\"obj\":\"all\",\"val\":1,\"delay\":10}" -m put coap://[fd11:22:0:0:240f:8ae:bbca:47c2]/cmd
{"err_code":0}
```

toggle

```shell
coap_cli -N -e "{\"cmd\":\"onoff\",\"obj\":\"all\",\"val\":2}" -m put coap://[fd11:22:0:0:45a7:d69c:3f32:614a]/cmd
{"err_code":0}
```

##### Set LED color

Note: It is effective only when the LED is on

```shell
coap_cli -N -e "{\"cmd\":\"change_color\",\"obj\":\"all\",\"r\":230,\"g\":230,\"b\":230}" -m put coap://[fd11:22:0:0:256:d4d4:98e2:2bf0]/cmd
{"err_code":0}
```

##### Set the GPIO level

```shell
coap_cli -N -e "{\"cmd\":\"set_gpio\",\"obj\":\"0.15\",\"val\":false}" -m put coap://[fd11:22:0:0:26e6:c63b:4a02:49d6]/cmd
{"err_code":0}
```

```shell
coap_cli -N -e "{\"cmd\":\"set_gpio\",\"obj\":\"0.15\",\"val\":true}" -m put coap://[fd11:22:0:0:26e6:c63b:4a02:49d6]/cmd
{"err_code":0}
```

##### Read the GPIO status

```shell
root@GL-S200:~# coap_cli -N -e "{\"cmd\":\"get_gpio_status\"}" -m put coap://[fd11:22:0:0:12c7:ca49:90c5:d269]/cmd
{"gpio_status":[{"obj":"0.15","val":0},{"obj":"0.16","val":0},{"obj":"0.17","val":0},{"obj":"0.20","val":0}],"err_code":0}
```

##### Read LED status

```shell
root@GL-S200:~# coap_cli -N -e "{\"cmd\":\"get_led_status\"}" -m put coap://[fd11:22:0:0:12c7:ca49:90c5:d269]/cmd
{"led_strip_status":[{"obj":"led_left","on_off":0,"r":0,"g":0,"b":0},{"obj":"led_left","on_off":0,"r":0,"g":0,"b":0}],"err_code":0}
```

### Buiding other demo 

cli demo is used as an example.

#### buiding

Enter cli demo directory and building

```
cd gl-nrf-sdk/nrf/samples/openthread/cli
west build -b gl_nrf52840_dev_board
```

The following print appears if the compilation is successful

```
[664/672] Linking CXX executable zephyr/zephyr.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:      387604 B     495104 B     78.29%
             RAM:      106552 B       256 KB     40.65%
        IDT_LIST:          0 GB         2 KB      0.00%
[667/672] Generating ../../zephyr/app_update.bin
sign the payload
[669/672] Generating ../../zephyr/app_signed.hex
sign the payload
[670/672] Generating ../../zephyr/app_test_update.hex
sign the payload
[672/672] Generating zephyr/merged.hex

```

####   Flashing	

GL Thread DEV Board is connected to ubuntu by J-LINK burner, and flashing the firmware to GL Thread DEV Board.

```
west flash --erase
```

​	If the flashing success will appear the following print

```
[ #################### ]  20.244s | Erase file - Done erasing                                                          
[ #################### ]   5.791s | Program file - Done programming                                                    
[ #################### ]   6.453s | Verify file - Done verifying                                                       
Enabling pin reset.
Applying pin reset.
-- runners.nrfjprog: Board with serial number 59768885 flashed successfully.
```

####    test

Enter cli command in GL development board terminal to test.

```
uart:~$ ot channel 11
Done
uart:~$ ot panid 0xabcd
Done
uart:~$ ot networkkey 00112233445566778899aabbccddeeff
Done
uart:~$ ot ifconfig up
Done
uart:~$ ot thread start
Done
uart:~$ ot state
leader
Done
uart:~$ ot networkname
ot_zephyr
Done
uart:~$ ot ipaddr
fdde:ad00:beef:0:0:ff:fe00:fc00
fdde:ad00:beef:0:0:ff:fe00:2000
fdde:ad00:beef:0:6664:a2c4:e0c3:9772
fe80:0:0:0:100a:90e1:b44:8bc
Done
uart:~$ ot dataset active -x
0e080000000000000000000300000b35060004001fffe00208dead00beef00cafe0708fddead00beef0000051000112233445566778899aabbccddeeff03096f745f7a65706879720102abcd04104407e0cbde217a7be71da8da414913cd0c0402a0f7f8
Done
```



---

## Adding a New Board to Home Assistant

Use the included [provision_tdb.py](provision_tdb.py) script to automate the full process.
It handles firmware flashing, Thread OTA, and Home Assistant registration end-to-end.
The only manual step is **pressing Button2** on the board to trigger Thread joining.

```bash
# One-time setup — set credentials as env vars (avoids secrets in process list)
export S200_CRYPT_PW='$1$6J.vdguZ$tTwXvmrqY.tYbME5DZpGw/'   # from docker-compose.yml (unescape $$ → $)
export HA_TOKEN='eyJ...'   # HA → Settings → Profile → Long-Lived Access Tokens

python3 provision_tdb.py \
    --firmware /path/to/GL-Thread-Dev-Board-FTD-OTA-vX.X.X.bin \
    --port /dev/ttyUSB0 \
    --name "TDB 3"
```

The manual steps below describe what the script does if you need to run them individually.

### Step 1: Flash Updated Firmware via UART DFU

The board has MCUboot, so new firmware can be pushed over USB before joining Thread.

1. Connect the board to a Windows PC via USB (Type-C).
2. Note the COM port (Device Manager → Ports).
3. Download the latest `.bin` release from the [Releases](../release) folder.
4. Open CMD in the `uart_dfu/windows/` directory and run:

```shell
# 1. Verify current version
mcumgr.exe --conntype serial --connstring=COM3,baud=460800 image list

# 2. Upload new firmware
mcumgr.exe --conntype serial --connstring=COM3,baud=460800 image upload GL-Thread-Dev-Board-FTD-OTA-vX.X.X.bin

# 3. Confirm upload and note the hash of slot-1
mcumgr.exe --conntype serial --connstring=COM3,baud=460800 image list

# 4. Mark new firmware for next boot (replace <HASH> with the slot-1 hash)
mcumgr.exe --conntype serial --connstring=COM3,baud=460800 image test <HASH>

# 5. Reboot into new firmware
mcumgr.exe --conntype serial --connstring=COM3,baud=460800 reset
```

Replace `COM3` with your actual port number. See [uart_dfu/README.md](uart_dfu/README.md) for details and screenshots.

### Step 2: Join the Thread Network

1. Power on the board within range of the GL-S200 gateway.
2. **Short press Button2** — LED2 (green) starts blinking, indicating the board is scanning for the S200 Thread network.
3. Wait for **LED2 to stay solid green** — the board has successfully joined.
4. Confirm in the GL-S200 web UI under **Thread → Devices** that the new board is listed.

### Step 3: Update Firmware via Thread OTA

With the board on Thread, push the final firmware over-the-air from the S200 using `mcumgr` over UDP.

1. SSH into the S200: `ssh root@<S200_IP>`
2. Find the board's Thread IPv6 address in the S200 web UI, or run:

```shell
root@GL-S200:~# ubus call otbr-gateway get_devices '{}'
```

3. Upload and activate the firmware:

```shell
# Upload firmware (replace [IPv6] and filename)
root@GL-S200:~# mcumgr --conntype udp --connstring=[fd11:22:0:0:xxxx:xxxx:xxxx:xxxx]:1337 image upload /tmp/GL-Thread-Dev-Board-FTD-OTA-vX.X.X.bin

# Check slot-1 hash after upload
root@GL-S200:~# mcumgr --conntype udp --connstring=[fd11:22:0:0:xxxx:xxxx:xxxx:xxxx]:1337 image list

# Confirm new image (replace <HASH> with slot-1 hash)
root@GL-S200:~# mcumgr --conntype udp --connstring=[fd11:22:0:0:xxxx:xxxx:xxxx:xxxx]:1337 image confirm <HASH>

# Reboot
root@GL-S200:~# mcumgr --conntype udp --connstring=[fd11:22:0:0:xxxx:xxxx:xxxx:xxxx]:1337 reset
```

### Step 4: Add the Board to Home Assistant

The `s200-bridge` service automatically discovers all boards joined to the S200. No manual device registration is needed.

1. In Home Assistant go to **Settings → Devices & Services**.
2. Find **S200 TDB Boards** and click **Configure**.
3. Select **Add Device**.
4. Choose the new board from the dropdown (auto-discovered from the S200).
5. Optionally set a friendly name and a webhook ID for PIR motion events.
6. Click **Submit** — sensor entities (temperature, humidity, pressure, PIR, etc.) and the RGB LED light entity will be created automatically.

---

## Recovery: Restoring Boards After GL-S200 OTA Overwrote Custom Firmware

### What Happened

On April 9, 2026 all three TDB boards stopped working. The root cause was **not** corrupted firmware or hardware failure. The GL-S200 has a built-in OTA auto-upgrade service (`gl_dev_board_ota`) that silently watched for boards joining the Thread network and automatically pushed GL-iNet's stock firmware (v2.1.1) over-the-air, overwriting our custom v1.3.0 firmware on every board.

**v2.1.1 is GL-iNet's official firmware — it has no relation to our version numbering.** Our firmware tops out at v1.3.0. If you see v2.x on a board it has been overwritten by the S200 OTA service.

### Permanent Fix: Disable the S200 OTA Service

SSH into the S200 and stop the auto-upgrade service. This must be done before connecting any board to the Thread network:

```bash
ssh root@<S200_IP>
/etc/init.d/gl_dev_board_ota stop
/etc/init.d/gl_dev_board_ota disable
```

Verify it is gone:
```bash
ps | grep ota | grep -v grep
# should return nothing
```

### Recovery Procedure: Re-flash Custom Firmware via USB (Ubuntu)

Use this procedure any time a board is on the wrong firmware and needs to be restored. You need a Linux machine with USB and the `mcumgr` binary from `uart_dfu/mcumgr/ubuntu22.04/mcumgr`.

#### Prerequisites on Ubuntu

```bash
# Stop ModemManager — it grabs USB serial ports and blocks mcumgr
sudo systemctl stop ModemManager
sudo systemctl disable ModemManager

# Copy mcumgr from NAS (use -O flag — modern Ubuntu scp uses SFTP by default which the NAS blocks)
scp -O NAS2743@192.168.1.211:/volume1/repos/gl-thread-dev-board/uart_dfu/mcumgr/ubuntu22.04/mcumgr ~/
scp -O NAS2743@192.168.1.211:/volume1/repos/gl-thread-dev-board/app_update_v1.3.0.bin ~/
chmod +x ~/mcumgr
```

> **Note:** The NAS ssh server uses `ForceCommand` which intercepts connections.
> Always use `scp -O` (legacy SCP mode) when copying files from the NAS — modern Ubuntu's scp uses SFTP by default which the NAS does not support.

#### Flash Each Board

Plug in one board at a time via USB-C. The port is typically `/dev/ttyUSB0`.

Add `-t 10 -r 5` flags to all mcumgr commands — the board's CoAP/OpenThread log output can corrupt SMP frames and these flags add retries to work around it.

```bash
# 1. Check what is currently on the board
~/mcumgr --conntype serial --connstring=/dev/ttyUSB0,baud=460800 -t 10 -r 5 image list
```

**If slot 1 already contains v1.3.0** (hash `383bba481c5e2b47cdf25ba769f4ee0822201ae8dfad9726cb05a2dff58f067b`), skip the upload step and go straight to test+reset:

```bash
# Mark v1.3.0 for next boot and reboot
~/mcumgr --conntype serial --connstring=/dev/ttyUSB0,baud=460800 -t 10 -r 5 image test 383bba481c5e2b47cdf25ba769f4ee0822201ae8dfad9726cb05a2dff58f067b
~/mcumgr --conntype serial --connstring=/dev/ttyUSB0,baud=460800 -t 10 -r 5 reset
```

**If slot 1 does not contain v1.3.0**, upload it first:

```bash
# Upload (~60 seconds)
~/mcumgr --conntype serial --connstring=/dev/ttyUSB0,baud=460800 -t 10 -r 5 image upload ~/app_update_v1.3.0.bin

# Get the slot 1 hash
~/mcumgr --conntype serial --connstring=/dev/ttyUSB0,baud=460800 -t 10 -r 5 image list

# Mark for next boot and reboot
~/mcumgr --conntype serial --connstring=/dev/ttyUSB0,baud=460800 -t 10 -r 5 image test 383bba481c5e2b47cdf25ba769f4ee0822201ae8dfad9726cb05a2dff58f067b
~/mcumgr --conntype serial --connstring=/dev/ttyUSB0,baud=460800 -t 10 -r 5 reset
```

After ~15 seconds confirm slot 0 shows `version: 1.3.0  flags: active confirmed`:

```bash
~/mcumgr --conntype serial --connstring=/dev/ttyUSB0,baud=460800 -t 10 -r 5 image list
```

Repeat for each board one at a time.

#### Troubleshooting: mcumgr Returns "NMP timeout"

| Symptom | Cause | Fix |
|---|---|---|
| `Error: device or resource busy` | `screen`, `picocom`, or ModemManager holding the port | `killall screen picocom`; `sudo systemctl stop ModemManager` |
| `NMP timeout` at 460800 baud, LED solid green | App running but log output corrupting SMP frames | Add `-t 10 -r 5` flags |
| `NMP timeout` at 115200 baud, LED blinking | MCUboot recovery window (~3s) expired before command ran | Re-do SW1+Reset and run command immediately |
| Port not found | Board not yet enumerated | Wait 5 seconds after plugging in, then `ls /dev/ttyUSB*` |

**MCUboot serial recovery mode** (last resort if app is completely dead):
1. Hold **SW1** (left button, labeled SW1 on the board — this is `mcuboot-button0` in DTS)
2. While holding SW1, press and release **Reset**
3. Keep holding SW1 for 2 seconds — LED will blink
4. Immediately run mcumgr at **115200** baud (MCUboot uses 115200, not 460800):

```bash
~/mcumgr --conntype serial --connstring=/dev/ttyUSB0,baud=115200 image list
```

### Commissioning on the Thread Network

The firmware uses **standard OpenThread Joiner protocol** with PSKD `AAAAAA` and `CONFIG_OPENTHREAD_JOINER_AUTOSTART=y` — the board joins automatically with no button press required.

The **GL-S200 web UI commissioning interface does not work** with our firmware. It uses a proprietary GL-iNet flow. Always use `ot-ctl` on the S200 CLI instead:

```bash
ssh root@<S200_IP>
ot-ctl commissioner start
ot-ctl commissioner joiner add * AAAAAA
```

The board will join within 30 seconds. LED2 goes solid green when on the network.

To find a board's Thread IPv6 address after joining (needed for OTA updates):

```bash
# Ping the Thread mesh-local all-nodes multicast to populate the neighbour table
ping6 -c 2 -I wpan0 ff03::1
# Board's mesh-local address (fd...) will appear in the ping replies
```



