# MODULE-IHSC
A thermal imaging module consisting of XIAO RP2040 &amp; MLX90640 for thermal camera and XIAO for the onboard processing 
which supports usb type c streaming and usb screen sharing extension via an 4 pin I2C extension

 <p align="center">
<img width="1232" height="874" alt="PCB-3D-FRONT" src="https://github.com/user-attachments/assets/d6a7bab1-9e3b-4110-b2c4-50942a6affa3" />
</p>

-------------------------------------------------------------------------------------------------
## Purpose 
The goal of this project was to design a low profile thermal camera module capable of streaming live
footage over to the screen via type c and I2C pins

things considerd in mind to make it happen
- PCB design
- CAD 
- Components
- Firmware modularization and easy modification
  
 <p align="center">
 <img width="1074" height="739" alt="Schematics" src="https://github.com/user-attachments/assets/3446310d-20e1-4f8c-a726-56d4cf608dc6" />
</p>

 <p align="center">
<img width="1906" height="1011" alt="Screenshot 2026-04-22 180021" src="https://github.com/user-attachments/assets/fff27ce3-d70f-4bf1-b2a5-b9fd4e196d83" />
</p>

-------------------------------------------------------------------------------------------------
## Features

 <p align="center">

</p>


*Core Hardware*

- XIAO RP2040
- MLX90640  
- 4 Pin I2C horizontal pins 
- buzzer for beeping
- tactile switch
- transistor
 
-------------------------------------------------------------------------------------------------
## Transport & Connectivity

 - 4 Pin I2C horizontal pins 
- USB-C connection
-------------------------------------------------------------------------------------------------
## Hardware
The PCB is designed using KiCad (open-source) and the case using Fusion 360.

Specifications

- 2-layer PCB
- XIAO RP2040 Proccessing
- MLX90640  heat Camera
- I²C  4 pin connector
- 
All PCB source files are available in `/hardware/kicad`.

<p align="center">
  <img width="699" height="651" alt="CAD-3" src="https://github.com/user-attachments/assets/d67ef8a7-6336-4066-8fc0-afc14124d56f" />
</p>

-------------------------------------------------------------------------------------------------

## Firmware


- Thermal Sensing camera

*Raw MLX90640 32×24 thermal array with 768 pixels, connected directly via I2C
with 110° wide field of view built into the camera
and have the abillity to measure −40 °C to +300 °C measurement range
Dual I2C bus architecture — sensor on I2C1 (GP6/GP7), display on I2C0 (GP4/GP5) to prevent bus conflicts*

- Rainbow Heatmap

*Full 7-stop rainbow colour mapping (violet → blue → cyan → green → yellow → orange → red)
Bicubic interpolation upscaling from 32×24 to full display resolution which will convert it into 
Real-time hotspot and coldspot crosshair overlays with temperature labels
Live temperature scale bar rendered on-screen*

<p align="center">
  <img width="812" height="612" alt="Rainbow heatmap" src="https://github.com/user-attachments/assets/878eed0c-2b86-4ca3-9ddf-7caa059d4dd2" />
</p>

- Dual Output System

*USB-CDC mode — binary frame stream over via a Type-C cable to a PC host viewer
I2C display mode — frames rendered directly onto an external OLED via the 4-pin PCB connector (SSD1306 128×64 or SH1107 128×128)
BOTH mode — simultaneous USB stream and I2C display output
Smart startup auto-detection: defaults to BOTH if display and USB host are both detected, I2C is used only if no PC is connected or USB if no display is found*

- Output Switching

*Short-press mode button cycles USB → I2C → BOTH → USB in real time
Long-press mode button saves current mode to flash (persists across reboots)
PC-side display_switch.py utility to switch modes remotely over USB without touching the device
Soft-reset trigger from PC so new mode activates immediately without unplugging* (made for future integration)

- PC Host Viewer (host_display.py)

*Live OpenCV window with rainbow heatmap whit support up to 30 fps
Bicubic interpolation for smooth upscaled display
Adjustable window scale factor
and it Cycle's through 5 colour maps: Rainbow, Inferno, Plasma, Bone, Jet
and the Temperature range lock/unlock (freeze min–max for consistent colour mapping)
and shows Hotspot and coldspot crosshairs with live °C labels and it has a 
Full-screen toggle
Live FPS counter and frame index overlay*

- PC Switch Utility (display_switch.py)

*Query current output mode from the device which 
Switchs to the output mode (USB / I2C / BOTH) via command line and
Scan the 4-pin I2C connector bus to detect connected display hardware and then
 the Live frame monitor — prints per-frame FPS, Tmin, Tmax, and checksum status and it also
Auto-detects RP2040 serial port (no --port flag needed in most cases)*

- Performance

*~30 fps live thermal stream at 32 Hz sensor refresh rate in the MLX90640 camera
-USB-CDC frame size: 1562 bytes per frame (~47 KB/s, well within USB bandwidth)
 and uses Binary framing protocol with magic header and checksum validation for data integrity
and Frame index tracking for dropped frame detection*

- Hardware Controls

*Power button (GP28): short press pauses/resumes streaming; long press enters light-sleep
Mode button (GP29): short press cycles output mode; long press saves mode to flash
Buzzer (GP27): distinct beep patterns for each event — power-on, mode switch (unique per mode), mode saved, sleep, and error*

- Buzzer Feedback

Single long beep on power-on
1 beep = USB mode selected
2 beeps = I2C display mode selected
3 beeps = BOTH mode / mode saved to flash
Double beep = entering sleep
5 rapid beeps = sensor not found (wiring error)

- Persistent Configuration

*Output mode saved to output_mode.json on the RP2040's onboard flash
Automatically restored on every reboot with no user intervention*

Firmware files are located in `/firmware`.

-------------------------------------------------------------------------------------------------
## Images

 <p align="center">
    <img width="893" height="674" alt="PCB" src="https://github.com/user-attachments/assets/68275bd9-c720-4739-8690-97e84e6b5463" />
</p>
 

 <p align="center">
    <img width="697" height="818" alt="Screenshot 2026-06-25 164149" src="https://github.com/user-attachments/assets/19e4a537-0c1e-4cde-9f96-2763d425d8b7" />
</p>

<p align="center">
     <img width="1487" height="808" alt="Screenshot 2026-06-25 164611" src="https://github.com/user-attachments/assets/c93d9f8e-5836-4b0d-9b41-769d4602c4ff" />
</p>


-------------------------------------------------------------------------------------------------
## BOM
The complete bill of materials (BOM), including purchase links, is available in  
`/hardware/bom.csv`.

Manufacturing & sourcing:
- PCB fabrication: **JLCPCB**
- Components (India): **Robu.in**
- Enclosure: Online 3D printing services (if needed)

  <p align="center">
   <img width="771" height="679" alt="Screenshot 2026-04-22 172205" src="https://github.com/user-attachments/assets/9aa9b383-118e-4b00-bc3a-61cb0a58a8b1" />
</p>


## Bill of Materials

| Name | Quantity | Total Cost (USD) | Distributor | Link |
|------|----------|-----------------|-------------|------|
| PCB | 5 | $8.00 | JLCPCB | [Buy](https://cart.jlcpcb.com/shopcart/cart/) |
| Soldering Flux | 1 | $0.27 | Robu.in | [Buy](https://robu.in/product/noel-flux-soldering-paste-10g) |
| MLX90640 | 1 | $37.48 | Digikey.in | [Buy](https://www.digikey.in/en/products/detail/melexis-technologies-nv/MLX90640ESF-BAA-000-SP/8638465) |
| XIAO RP2040 | 1 | $6.41 | Robocraze.in | [Buy](https://robocraze.com/products/seeed-studio-xiao-rp2040-development-board?variant=47742255562976&country=IN&currency=INR&campaignid=23145906364&adgroupid=182236965810&keyword=&device=c&gad_campaignid=23145906364) |
| Buzzer (BZ1) | 1 | $0.15 | Robu.in | [Buy](https://robu.in/product/3v-active-electromagnetic-buzzer-pack-of-5) |
| 1×4 Pin Header (J1) | 1 | $0.14 | Robu.in | [Buy](https://robu.in/product/ds1022-22-1x10ruf1-a3-0-b6-0-c6-0-b-connfly-1x10-pin-2-54mm-pin-header-r-a-type-h2-0mm) |
| Low Profile Tactile Switch SMD | 1 | $0.11 | Robu.in | [Buy](https://robu.in/product/tsc008a1718a-bzcn-4x4x1-7mm-50ma-round-button-50gf-12v-smd-tactile-switches-rohs) |
| Resistor 4.7 kΩ 0603 (R2) | 2 | $0.11 | Robu.in | [Buy](https://robu.in/product/4-7k-ohm-1-4w-0603-surface-mount-chip-resistor-pack-of-100) |
| Resistor 1 kΩ 0805 (R1) | 1 | $0.11 | Robu.in | [Buy](https://robu.in/product/1k-ohm-1-4w-0805-surface-mount-chip-resistor-pack-of-10) |
| Capacitor 100 nF 0805 (C1) | 1 | $0.12 | Robu.in | [Buy](https://robu.in/product/cs2012x7r106m100nre-samwha-10v-10uf-x7r-%c2%b120-0805-multilayer-ceramic-capacitors-mlcc-smd-smt-rohs) |
| Capacitor 1 µF 0603 (C3) | 1 | $0.11 | Robu.in | [Buy](https://robu.in/product/1uf-1000nf-50v-capacitor-0603-smd-package-pack-of-20) |
| Capacitor 10 µF 0805 (C2) | 1 | $0.11 | Robu.in | [Buy](https://robu.in/product/100nf-0805-surface-mount-multilayer-ceramic-capacitor-pack-of-40) |
| 2N2219 Transistor | 1 | $ | link |
| **Total** | | **$77.81** | | |


please note that most of the parts are from indian suppliers !

*note there's also a transistor in the project (2N2219) but it's not available anywhere online for me that's why it's not included in the BOM with price and link

-------------------------------------------------------------------------------------------------
## License
This project is open-source hardware and software and is released under the **MIT License**.
