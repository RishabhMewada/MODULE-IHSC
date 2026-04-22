# MODULE-IHSC
A thermal imaging module consisting of XIAO RP2040 &amp; WAVESHARE MLX90640 for thermal camera and XIAO for the onboard processing 
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
 <img width="1024" height="1536" alt="Flowchart" src="https://github.com/user-attachments/assets/062e7571-7452-4109-9c1e-cc33dfc627fd" />
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


- Thermal Sensing

*Raw MLX90640 32×24 thermal array (768 pixels) connected directly via I2C
110° wide field of view
−40 °C to +300 °C measurement range
Dual I2C bus architecture — sensor on I2C1 (GP6/GP7), display on I2C0 (GP4/GP5) to prevent bus conflicts*

- Rainbow Heatmap

*Full 7-stop rainbow colour mapping (violet → blue → cyan → green → yellow → orange → red)
Bicubic interpolation upscaling from 32×24 to full display resolution
Real-time hotspot and coldspot crosshair overlays with temperature labels
Live temperature scale bar rendered on-screen*

<p align="center">
  <img width="812" height="612" alt="Rainbow heatmap" src="https://github.com/user-attachments/assets/878eed0c-2b86-4ca3-9ddf-7caa059d4dd2" />
</p>

- Dual Output System

*USB-CDC mode — binary frame stream over Type-C cable to a PC host viewer
I2C display mode — frames rendered directly onto an external OLED via the 4-pin PCB connector (SSD1306 128×64 or SH1107 128×128)
BOTH mode — simultaneous USB stream and I2C display output
Smart startup auto-detection: defaults to BOTH if display and USB host are both detected, I2C-only if no PC is connected, USB if no display is found*

- Output Switching

*Short-press mode button cycles USB → I2C → BOTH → USB in real time
Long-press mode button saves current mode to flash (persists across reboots)
PC-side display_switch.py utility to switch modes remotely over USB without touching the device
Soft-reset trigger from PC so new mode activates immediately without unplugging* (made for future integration)

- PC Host Viewer (host_display.py)

*Live OpenCV window with rainbow heatmap at up to 30 fps
Bicubic interpolation for smooth upscaled display
Adjustable window scale factor
Cycle through 5 colour maps: Rainbow, Inferno, Plasma, Bone, Jet
Temperature range lock/unlock (freeze min–max for consistent colour mapping)
Hotspot and coldspot crosshairs with live °C labels
PNG snapshot saving with timestamp
Full-screen toggle
Live FPS counter and frame index overlay*

- PC Switch Utility (display_switch.py)

*Query current output mode from the device
Switch output mode (USB / I2C / BOTH) via command line
Scan the 4-pin I2C connector bus to detect connected display hardware
Live frame monitor — prints per-frame FPS, Tmin, Tmax, and checksum status
Auto-detects RP2040 serial port (no --port flag needed in most cases)*

- Performance

*~30 fps live thermal stream at 32 Hz sensor refresh rate
USB-CDC frame size: 1562 bytes per frame (~47 KB/s, well within USB bandwidth)
Binary framing protocol with magic header and checksum validation for data integrity
Frame index tracking for dropped frame detection*

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
<img width="893" height="674" alt="PCB" src="https://github.com/user-attachments/assets/68275bd9-c720-4739-8690-97e84e6b5463" />

<img width="651" height="761" alt="PCB-4" src="https://github.com/user-attachments/assets/3ad33133-2df3-48a8-bab6-9f5a8320f6ff" />

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
  
-------------------------------------------------------------------------------------------------
## License
This project is open-source hardware and software and is released under the **MIT License**.
