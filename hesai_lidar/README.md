# OT128 PCAP to PCD Conversion Using Hesai SDK (Docker + Ubuntu 22.04)

This document provides instructions to:

- Run Ubuntu 22.04 in Docker
- Build the Hesai LiDAR SDK
- Modify `test.cc`
- Parse an OT128 `.pcap`
- Export a calibrated `.pcd` file

This method uses the official Hesai SDK (C++) for correct calibration and geometry.

---

# 1. Launch Ubuntu 22.04 in Docker

For macOS, run in terminal:

```bash
docker run -it --name hesai -v ~/Desktop:/workspace ubuntu:22.04 /bin/bash
```

If the container already exists:

```bash
docker start -ai hesai
```

Otherwise, run on Ubuntu 22.04

# 2. Install Build Dependencies (Inside Docker)

```bash
apt update
apt install -y build-essential cmake git
```

---

# 3. Clone Official Hesai SDK

Inside Docker, or Ubuntu, I clone this to my Desktop (/workspace is mounted to my Desktop):

```bash
cd /workspace
git clone https://github.com/HesaiTechnology/HesaiLidar_SDK_2.0.git
cd HesaiLidar_SDK_2.0
```

---

# 4. Build the SDK

```bash
mkdir build
cd build
cmake ..
make -j8
```

You should see:

```
[100%] Built target sample
```
# 5. Getting the pcap 

Use `tcpdump` on the Raspberry Pi to record raw Hesai LiDAR packets:

```bash
sudo tcpdump -i eth0 -w your_file.pcap
```
- sudo: required for packet capture permissions
- tcpdump: capture raw UDP packets from the network interface
- -i eth0: listen on the Ethernet interface connected to the LiDAR
- -w: write captured packets to specified file

Copy your file over from Raspberry Pi into your working directory for PC/laptop

# 6. Modify test.cc file

Replace the current SDK `test.cc` file with the updated `test.cc` file from the GitHub. 

## 6.1. Configuration Checklist
Before building and running, update the following fields in `test.cc`:

### 1. PCAP File Path
```cpp
param.input_param.pcap_path = "/workspace/PanoRadar/ORIN/hesai_capture_test_5.pcap";
```
Change to the path of your `.pcap` file inside the Docker container.

### 2. Angle Correction File
```cpp
param.input_param.correction_file_path = "/workspace/PanoRadar/OT128_Angle-Correction-File-1.csv";
```
Change to the path of your OT128 angle correction `.csv` file.

### 3. Firetime File
```cpp
param.input_param.firetimes_path = "/workspace/PanoRadar/Standard_Mode_firetime_1212.csv";
```
Change to the path of your OT128 firetime `.csv` file. Make sure this matches the firing mode your sensor was configured in during capture.

### 4. SBG UTC Reference File
```cpp
utc_offset = GetSBGStartUTC("/workspace/PanoRadar/time_alignment/csv_logs3/utcTime.csv") - frame.frame_start_timestamp;
```
Change to the path of the `utcTime.csv` generated from your SBG binary log. This file is used to anchor the LiDAR timestamps to true UTC.

### 5. Timestamp Output File
```cpp
ts_log.open("/workspace/frames/timestamps_2.csv");
```
Change the output filename if needed to avoid overwriting previous runs (e.g. `timestamps_3.csv`).


## 6.2. Timestamps

In order to later produce timestamps that can be interpreted in both UNIX and UTC you need to add/locate this line in the `test.cc` (if not already there). Ensure that you have this parameter `param.decoder_param.use_timestamp_type` set to = 1.

```cpp
  param.decoder_param.enable_packet_loss_tool = false;
  param.decoder_param.socket_buffer_size = 262144000;
  param.decoder_param.use_timestamp_type = 1;
  system("mkdir -p /workspace/frames");
  ts_log.open("/workspace/frames/timestamps.csv");
  ts_log << "frame_index,frame_start_timestamp,frame_end_timestamp\n";
  //init lidar with param
  sample.Init(param);
```
This is very important, as this parameter determines whether the LiDAR will use its global (UNIX/UTC) time for its frame timestamps or its own time from boot. The timestamp decoder parameter must be set to 1.

---

## 6.3 Resulting Output
After you build, make, and execute the decoder, you should be getting frame-by-frame .pcd files located in the newly made `frames` directory. Additionally, you should also get an accompanying `timestamps.csv` that has each frame's start and end time in UNIX (will later be converted to UTC).

# 7. Rebuild After Modifying Code

Inside Docker:

```bash
cd /workspace/HesaiLidar_SDK_2.0/build
make -j8
```

---

# 8. Run the Decoder

```bash
./sample
```

Expected output:

```
-------- Hesai Lidar SDK --------
...
Saved SDK frame to sdk_frame.pcd
```

Output files should be:
- frame_*.pcd (quantity depends on length of run)
- timestamps.csv (indexes every frame by timestamp with start and end time)

---

# 9. Interpreting the Data and Post-Processing

You are now left with the point cloud for each frame, as well as the corresponding timestamp for each frame (start to finish). Using the script `lidar_utc.py` you can convert the frame timestamps into UTC, which will output a new `timestamp.csv` file.

Your new `timestamps.csv` file will have the last two columns be the frame start and end time in UTC. You can now compare the LiDAR and IMU/GNSS data using time-aligned UTC data-points. 


