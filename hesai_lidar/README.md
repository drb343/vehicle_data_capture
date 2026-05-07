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

Once your environment/Ubuntu 22.04 is set up, and you have cloned the repo, modify the test.cc file on your Desktop

It will be under HesaiLidar_SDK_2.0/test/test.cc if on Docker

Ensure this macro is enabled:

```cpp
#define PCAP_PARSER_TEST
```
Inside the `#elif defined (PCAP_PARSER_TEST)` section, input the paths to your .pcap file and angle correction .csv file inside the PCAP section:

```cpp
param.input_param.source_type = DATA_FROM_PCAP;
param.input_param.pcap_path = "your_pcap_path_here";
param.input_param.correction_file_path = "your_csv_path_here";
```

# 6.1. Modify lidarCallback() to export .pcd

Locate:

```cpp
void lidarCallback(const LidarDecodedFrame<LidarPointXYZICRT> &frame)
```

Remove its contents and copy and paste this into the function:

```cpp
static bool saved = false;

  cur_frame_time = GetMicroTickCount();
  if (last_frame_time == 0) last_frame_time = GetMicroTickCount();
  uint32_t diff = (frame.fParam.IsMultiFrameFrequency() == 0) ? kMaxTimeInterval : kMaxTimeInterval * frame.multi_rate;
  if (cur_frame_time - last_frame_time > diff) {
    printf("Time between last frame and cur frame is: %u us\n", (cur_frame_time - last_frame_time));
  }
  last_frame_time = cur_frame_time;

  if (frame.fParam.IsMultiFrameFrequency() == 0) {
    printf("%ld -> frame:%d points:%u packet:%u start time:%lf end time:%lf\n",
      GetMicroTimeU64(), frame.frame_index, frame.points_num, frame.packet_num,
      frame.frame_start_timestamp, frame.frame_end_timestamp);
  } else {
    printf("%ld -> frame:%d points:%u packet:%u start time:%lf end time:%lf\n",
      GetMicroTimeU64(), frame.multi_frame_index, frame.multi_points_num, frame.multi_packet_num,
      frame.multi_frame_start_timestamp, frame.multi_frame_end_timestamp);
  }

  // Log Timestamps
  double utc_start = frame.frame_start_timestamp;
  double utc_end   = frame.frame_end_timestamp;

  ts_log << frame.frame_index << "," << std::fixed << std::setprecision(6)
        << utc_start << "," << utc_end << "\n";
  ts_log.flush();
 


  // Save all frames as PCD
  std::string path = "/workspace/frames/frame_" + std::to_string(frame.frame_index) + ".pcd";
  std::ofstream pcd(path);
  pcd << "# .PCD v0.7 - Point Cloud Data file format\n";
  pcd << "VERSION 0.7\n";
  pcd << "FIELDS x y z intensity\n";
  pcd << "SIZE 4 4 4 4\n";
  pcd << "TYPE F F F F\n";
  pcd << "COUNT 1 1 1 1\n";
  pcd << "WIDTH " << frame.points_num << "\n";
  pcd << "HEIGHT 1\n";
  pcd << "VIEWPOINT 0 0 0 1 0 0 0\n";
  pcd << "POINTS " << frame.points_num << "\n";
  pcd << "DATA ascii\n";
  for (uint32_t i = 0; i < frame.points_num; ++i) {
    const auto &pt = frame.points[i];
    pcd << pt.x << " " << pt.y << " " << pt.z << " " << pt.intensity << "\n";
  }
  pcd.close();
  printf("Saved frame %d to %s\n", frame.frame_index, path.c_str());
```

This ensures:
- Every frame of the .pcap reading is converted into a .pcd
- Every frame is timestamped according to Sepentrio-based timestamps (which can be converted to UTC)

Save the file.

# 6.2. Timestamps

In order to later produce timestamps that can be interpreted in both Sepentrio and UTC you need to add/locate this line in the `test.cc`

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
This is very important, as this parameter determines whether the LiDAR will use its global (Sepentrio/UTC) time for its frame timestamps or its own time from boot. The timestamp decoder parameter must be set to 1.

---

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

You are now left with the point cloud for each frame, as well as the corresponding timestamp for each frame (start to finish). Using the script `lidar_utc.py` you can convert the frame timestamps into utc, which will output a new timestamp.csv file.


