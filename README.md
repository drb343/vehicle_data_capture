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

---

# 5. Modify test.cc file

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

# 5.1. Modify lidarCallback() to export .pcd

Locate:

```cpp
void lidarCallback(const LidarDecodedFrame<LidarPointXYZICRT> &frame)
```

Add the following inside the function:

```cpp
static bool saved = false;
if (saved) return;

if (!saved && frame.points_num == 460800)
{
    std::ofstream pcd("/workspace/sdk_frame.pcd");

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

    for (uint32_t i = 0; i < frame.points_num; ++i)
    {
        const auto &pt = frame.points[i];
        pcd << pt.x << " "
            << pt.y << " "
            << pt.z << " "
            << pt.intensity << "\n";
    }

    pcd.close();
    saved = true;
    printf("Saved SDK frame to sdk_frame.pcd\n");
}
```

This ensures:
- Partial frame 0 is ignored
- Only a full 460,800-point rotation is saved
- Output file is written to Desktop

Save the file.

---

# 6. Rebuild After Modifying Code

Inside Docker:

```bash
cd /workspace/HesaiLidar_SDK_2.0/build
make -j8
```

---

# 7. Run the Decoder

```bash
./sample
```

Expected output:

```
-------- Hesai Lidar SDK --------
...
Saved SDK frame to sdk_frame.pcd
```

---




