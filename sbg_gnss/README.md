# Ellipse-D IMU/GNSS Raw Data (SBF/SBG Binary) to CSV Conversion (Docker + Ubuntu 22.04)

This document provides instructions to:

- Capture raw SBG ECom binary serial data from the Ellipse-D IMU/GNSS
- Process the binary log and convert it into CSV format using the Ellipse-D SDK

This method uses the official Ellipse-D SDK (C/C++) for correct decoding, calibration, and geometry handling.

---

## 1. Create Binary Capture on Raspberry Pi 4

To generate a binary log file for post-processing, capture the IMU/GNSS data using `sbg_data_read.py`:

```bash
python3 sbg_data_read.py
```

- `sbg_data_read.py`: Uses `pyserial` to establish a connection between the Ellipse-D and Raspberry Pi 4 via USB  
- `PORT`: `/dev/ttyUSB0` confirm this device appears on the Raspberry Pi prior to execution  
- `BAUD`: `115200` must match the configured baud rate of the Ellipse-D  
- `Ctrl+C` stops the capture  

The output of this script is a raw SBG binary log file (e.g., `.sbg` or `.bin`) that must be copied to your local machine

## 2. 
