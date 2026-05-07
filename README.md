To run the hesai_lidar and sbg_gnss simulataneously (either inside the Jetson Orin Nano Terminal with Display or via ssh) you must first run:

```bash
sudo chmod 666 /dev/ttyUSB0
```

This sets R/W permissions for everyone on the serial port file. Then run:

```bash
sudo tcpdump -i enP8p1s0 -w hesai_capture_test.pcap & python3 sbg_data_read.py; sudo pkill tcpdump
```

enp8p1s0 is the new name for `eth0` (Hesai LIDAR communicates over Ethernet). Pressing Ctrl+C stops the data collection for both.

