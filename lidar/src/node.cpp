/*
*  3iRoboticsLIDAR System II
*  Driver Interface
*
*  Copyright 2017 3iRobotics
*  All rights reserved.
*
*	Author: 3iRobotics, Data:2017-09-15
*
*/

#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/laser_scan.hpp"

#include "C3iroboticsLidar.h"
#include "../sdk/include/CSerialConnection.h"

#define DEG2RAD(x) ((x)*M_PI/180.)

using namespace std;

typedef struct _rslidar_data
{
    _rslidar_data()
    {
        signal = 0;
        angle = 0.0;
        distance = 0.0;
    }
    uint8_t signal;
    float   angle;
    float   distance;
}RslidarDataComplete;


using namespace everest::hwdrivers;
sensor_msgs::msg::LaserScan scan_msg_;
void publish_scan(rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr *pub,
                  _rslidar_data *nodes,
                  size_t node_count, rclcpp::Time start,
                  double scan_time,
                  float angle_min, float angle_max,
                  std::string frame_id)
{
    //定义消息数据
    sensor_msgs::msg::LaserScan scan_msg;
    scan_msg.header.stamp = start;
    scan_msg.header.frame_id = frame_id;
    scan_msg.angle_min = angle_min;
    scan_msg.angle_max = angle_max;
    scan_msg.angle_increment = (scan_msg.angle_max - scan_msg.angle_min) / (360.0f - 1.0f);

    scan_msg.scan_time = scan_time;
    scan_msg.time_increment = scan_time / (double)(node_count-1);
    scan_msg.range_min = 0.3;
    scan_msg.range_max = 5.0;

    scan_msg.ranges.resize(360, std::numeric_limits<float>::infinity());
    scan_msg.intensities.resize(360, 0.0);

    //解压雷达数据
    for (size_t i = 0; i < node_count; i++)
    {
        size_t current_angle = floor(nodes[i].angle);
        if(current_angle > 360.0)
        {
            printf("Lidar angle is out of range %d\n", (int)current_angle);
            continue;
        }
        float read_value = (float) nodes[i].distance;
        
        if (read_value < scan_msg.range_min || read_value > scan_msg.range_max){
            scan_msg.ranges[360- 1- current_angle] = std::numeric_limits<float>::infinity();
        }
        else{
            scan_msg.ranges[360 -1- current_angle] = read_value;
        }
		float intensities = (float) nodes[i].signal;
		scan_msg.intensities[360 -1- current_angle] = intensities;

	}
    scan_msg_ = scan_msg;
}
int main(int argc, char * argv[])
{    
    //读取ROS参数
    rclcpp::init(argc, argv);
    auto node = std::make_shared<rclcpp::Node>("delta_2a_lidar_node");
    
	int    opt_com_baudrate = 230400;
	string opt_com_path;
    string frame_id;
    string lidar_scan;

    node->declare_parameter("serial_port",rclcpp::ParameterValue("/dev/ttyUSB0"));
    node->declare_parameter("frame_id",rclcpp::ParameterValue("scan_link"));
    node->declare_parameter("lidar_scan",rclcpp::ParameterValue("/scan"));
    node->get_parameter("serial_port", opt_com_path);
    node->get_parameter("frame_id", frame_id);
    node->get_parameter("lidar_scan", lidar_scan);
    rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr scan_pub = 
    node->create_publisher<sensor_msgs::msg::LaserScan>(lidar_scan, 1000);


    CSerialConnection serial_connect;
    C3iroboticsLidar robotics_lidar;

    
    serial_connect.setBaud(opt_com_baudrate);
    serial_connect.setPort(opt_com_path.c_str());
    if(serial_connect.openSimple())
    {
        printf("[AuxCtrl] Open serail port sucessful!\n");
    }
    else
    {
        printf("[AuxCtrl] Open serail port %s failed! \n", opt_com_path.c_str());
        return -1;
    }

    printf("3iRoboticsLidar connected\n");

    robotics_lidar.initilize(&serial_connect);

    rclcpp::Time start_scan_time;
    rclcpp::Time end_scan_time;
    double scan_duration;
    start_scan_time = node->get_clock()->now();
    while (rclcpp::ok())
    {
		TLidarGrabResult result = robotics_lidar.getScanData();
        switch(result)
        {
            case LIDAR_GRAB_ING:
            {
                break;
            }
            case LIDAR_GRAB_SUCESS:
            {
                TLidarScan lidar_scan = robotics_lidar.getLidarScan();
                size_t lidar_scan_size = lidar_scan.getSize();
                std::vector<RslidarDataComplete> send_lidar_scan_data;
                send_lidar_scan_data.resize(lidar_scan_size);
                RslidarDataComplete one_lidar_data;
                for(size_t i = 0; i < lidar_scan_size; i++)
                {
                    one_lidar_data.signal = lidar_scan.signal[i];
                    one_lidar_data.angle = lidar_scan.angle[i];
                    one_lidar_data.distance = lidar_scan.distance[i];
                    send_lidar_scan_data[i] = one_lidar_data;
                }

            	float angle_min = DEG2RAD(0.0f);
            	float angle_max = DEG2RAD(359.0f);


				end_scan_time = node->get_clock()->now();
				scan_duration = (end_scan_time - start_scan_time).seconds() * 1e-3;
                // printf("Receive Lidar count %u!\n", lidar_scan_size);

                //如果成功，发布激光雷达扫描
                int start_node = 0, end_node = 359;
                publish_scan(&scan_pub, &send_lidar_scan_data[0], lidar_scan_size,
                         start_scan_time, scan_duration,
                         angle_min, angle_max,
                         frame_id);
                scan_pub->publish(scan_msg_);

				start_scan_time = end_scan_time;

                break;
            }
            case LIDAR_GRAB_ERRO:
            {
                printf("[Main] erro!\n");
                break;
            }
            case LIDAR_GRAB_ELSE:
            {
                printf("[Main] LIDAR_GRAB_ELSE!\n");
                break;
            }
        }
        usleep(50);
        // rclcpp::spin(node);
        rclcpp::spin_some(node);
    }
    rclcpp::shutdown();
    return 0;
}
