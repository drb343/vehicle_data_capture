#include "hesai_lidar_sdk.hpp"
#include <sstream>
#include <ctime>

// #define LIDAR_PARSER_TEST
// #define SERIAL_PARSER_TEST
#define PCAP_PARSER_TEST //only have this one defined
// #define EXTERNAL_INPUT_PARSER_TEST
// #define LIDAR_PARSER_TCP_TEST

double pcap_start_utc = 0.0;
double lidar_tow_anchor = 0.0;
bool anchor_set = false;

uint32_t last_frame_time = 0;
uint32_t cur_frame_time = 0;
bool running = true;
std::ofstream ts_log;

double utc_offset = 0.0;
bool offset_set = false;

double GetSBGStartUTC(const std::string& sbg_utc_path) {
  std::ifstream f(sbg_utc_path);
  std::string line;
  std::getline(f, line); // skip header
  std::getline(f, line); // skip units row
  std::getline(f, line); // first data row

  std::stringstream ss(line);
  std::string tok;
  std::vector<std::string> cols;
  while (std::getline(ss, tok, ',')) cols.push_back(tok);

  struct tm t{};
  t.tm_year = std::stoi(cols[3]) - 1900;
  t.tm_mon  = std::stoi(cols[4]) - 1;
  t.tm_mday = std::stoi(cols[5]);
  t.tm_hour = std::stoi(cols[6]);
  t.tm_min  = std::stoi(cols[7]);
  t.tm_sec  = std::stoi(cols[8]);
  return (double)timegm(&t) + std::stol(cols[9]) / 1e9;
}

//log info, display frame message
void lidarCallback(const LidarDecodedFrame<LidarPointXYZICRT> &frame) {
  static bool saved = false;
  
  //time alignment with the IMU/GNSS
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
  if (!offset_set) {
    utc_offset = GetSBGStartUTC("/workspace/PanoRadar/time_alignment/csv_logs3/utcTime.csv") - frame.frame_start_timestamp;
    offset_set = true;
  }
  double utc_start = frame.frame_start_timestamp + utc_offset;
  double utc_end   = frame.frame_end_timestamp   + utc_offset;

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
}

void faultMessageCallback(const FaultMessageInfo& fault_message_info) {
  // Use fault message messages to make some judgments
  // fault_message_info.Print();
  return;
}

// Determines whether the PCAP is finished playing
bool IsPlayEnded(HesaiLidarSdk<LidarPointXYZICRT>& sdk)
{
  return sdk.lidar_ptr_->IsPlayEnded();
}

void packetProducer(HesaiLidarSdk<LidarPointXYZICRT>& sample, const UdpFrame_t& udp_packet_frame) {
  size_t index = 0;
  while (running && index < udp_packet_frame.size()) {
    if (sample.OriginPacketIsBufferFull()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }
    sample.PushOriginPacketBuffer(udp_packet_frame[index].buffer, udp_packet_frame[index].packet_len, udp_packet_frame[index].recv_timestamp);
    index++;
  }
  running = false;
}

int main(int argc, char *argv[])
{
#ifndef _MSC_VER
  if (system("sudo sh -c \"echo 562144000 > /proc/sys/net/core/rmem_max\"") == -1) {
    printf("Command execution failed!\n");
  }
#endif
  HesaiLidarSdk<LidarPointXYZICRT> sample;
  DriverParam param;

  param.use_gpu = (argc > 1);
  // assign param
#ifdef LIDAR_PARSER_TEST
  param.input_param.source_type = DATA_FROM_LIDAR;
  param.input_param.device_ip_address = "192.168.1.201";  // lidar ip
  param.input_param.ptc_port = 9347; // lidar ptc port
  param.input_param.udp_port = 2368; // point cloud destination port
  param.input_param.multicast_ip_address = "";

  param.input_param.ptc_mode = PtcMode::tcp;
  param.input_param.use_ptc_connected = true;  // true: use PTC connected, false: recv correction from local file
  param.input_param.correction_file_path = "Your correction file path";
  param.input_param.firetimes_path = "Your firetime file path";

  param.input_param.host_ip_address = ""; // point cloud destination ip, local ip
  param.input_param.fault_message_port = 0; // fault message destination port, 0: not use
  // PtcMode::tcp_ssl use
  param.input_param.certFile = "";
  param.input_param.privateKeyFile = "";
  param.input_param.caFile = "";


#elif defined (SERIAL_PARSER_TEST)
  param.input_param.source_type = DATA_FROM_SERIAL;
  param.input_param.rs485_com = "Your serial port name for receiving point cloud";
  param.input_param.rs232_com = "Your serial port name for sending cmd";
  param.input_param.point_cloud_baudrate = 3125000;
  param.input_param.correction_save_path = "";
  param.input_param.correction_file_path = "Your correction file path";
  
#elif defined (PCAP_PARSER_TEST)
  param.input_param.source_type = DATA_FROM_PCAP;
  param.input_param.pcap_path = "/workspace/PanoRadar/ORIN/hesai_capture_test_5.pcap";
  param.input_param.correction_file_path = "/workspace/PanoRadar/OT128_Angle-Correction-File-1.csv";
  param.input_param.firetimes_path =
    "/workspace/PanoRadar/Standard_Mode_firetime_1212.csv";
;


#elif defined (EXTERNAL_INPUT_PARSER_TEST)
  param.input_param.source_type = DATA_FROM_ROS_PACKET;
  param.input_param.correction_file_path = "Your correction file path";
  param.input_param.firetimes_path = "Your firetime file path";

#elif defined (LIDAR_PARSER_TCP_TEST)
  param.input_param.source_type = DATA_FROM_LIDAR_TCP;
  param.input_param.device_ip_address = "192.168.1.201";  // lidar ip
  param.input_param.device_tcp_src_port = 5121; // lidar pointcloud tcp port
  param.input_param.ptc_port = 9347; // lidar ptc port
  param.input_param.use_ptc_connected = true;  // true: use PTC connected, false: recv correction from local file
  param.input_param.correction_file_path = "Your correction file path";
  param.input_param.firetimes_path = "Your firetime file path";
#endif

  param.decoder_param.enable_packet_loss_tool = false;
  param.decoder_param.socket_buffer_size = 262144000;
  param.decoder_param.use_timestamp_type = 1;
  system("mkdir -p /workspace/frames");
  ts_log.open("/workspace/frames/timestamps_2.csv");
  ts_log << "frame_index,frame_start_timestamp,frame_end_timestamp\n";
  //init lidar with param
  sample.Init(param);

#ifdef EXTERNAL_INPUT_PARSER_TEST
  UdpFrame_t udp_packet_frame;
  std::thread producer_thread(packetProducer, std::ref(sample), udp_packet_frame);
#endif

  //assign callback fuction
  sample.RegRecvCallback(lidarCallback);
  sample.RegRecvCallback(faultMessageCallback);

  sample.Start();
  if (sample.lidar_ptr_->GetInitFinish(FailInit)) {
#ifdef EXTERNAL_INPUT_PARSER_TEST
    running = false;
    if (producer_thread.joinable()) {
        producer_thread.join();
    }
#endif
    sample.Stop();
    return -1;
  }

  while (!IsPlayEnded(sample) || GetMicroTickCount() - last_frame_time < 1000000)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
#ifdef EXTERNAL_INPUT_PARSER_TEST
    if (running == false) break;
#endif
  }

#ifdef EXTERNAL_INPUT_PARSER_TEST
  if (producer_thread.joinable()) {
      producer_thread.join();
  }
#endif
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  ts_log.close();
  printf("The PCAP file has been parsed and we will exit the program.\n");
  return 0;
}
