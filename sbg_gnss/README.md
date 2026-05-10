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

## 2. Launch Ubuntu 22.04 in Docker

This SDK requires Ubuntu 22.04. Please follow the steps on the "hesai_lidar/README.md" for further instructions

## 2.1 Clone the sbgECom repository

```bash
cd /workspace
git clone https://github.com/SBG-Systems/sbgECom.git
cd sbgECom
```

## 3. Converting the SBF file

Once you have the SDK in your local machine, navigate to `sbgECom/tools/sbgEComApi/src/main.c`. 

From there, you will want to make the build dir and then make the file:

```bash
mkdir build
cd build
cmake .. -DBUILD_TOOLS=ON
make -j4
```
Then to run the main.cc enter:

```bash
./sbgBasicLogger -i /workspace/your_sbf_file.sbf -w -o /workspace/csv_logs3
```

This will create ten .txt files in the file specified at the end of the command (in my case `csv_logs3`).

## 4. Converting the files to CSV 

The ten files you generate in `csv_logs3` are in .txt format, but they need to be converted to .csv format. You can do this with the script `txt_to_csv.py`. 

## 5. Timestamping the Data

You will notice that the current .csv files are timestamped in the first column of every row. This timestamp is native to Septentrio systems and cannot be used to accurately compare to the LiDAR (LiDAR does not use this time). Thus you need to add another column to these files that has each data point in UTC time. You can do this by leveraging the `utcTime.csv` file provided in the initial ten file generation.

To convert to UTC, run this python script `csv_to_utc.py` which adds a UTC column to every one of your rows for the ten files (excluding utcTime.csv). It will save these new files in a `converted` folder. Of course, you can change this file path to whatever works best for you.

## 5. Interpreting the Data

At this point, your `converted` folder should have 9 files with all of the relevant IMU/GNSS sensor data. Each file should have the last column be `utc_timestamp_str`. You can now compare your UTC time aligned data to the LiDAR data. Note, the LiDAR and IMU/GNSS have a ~1 second startup delay, which can be attributed to the startup time required for the sensors to collect and then transmit data.
