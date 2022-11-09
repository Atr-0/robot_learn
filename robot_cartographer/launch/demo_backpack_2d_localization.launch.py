"""
  Copyright 2018 The Cartographer Authors
  Copyright 2022 Wyca Robotics (for the ros2 conversion)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
"""

from email.policy import default
from http.server import executable
from importlib.resources import Package
from platform import node

from sympy import true
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, ExecuteProcess
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node, SetRemap
from launch_ros.substitutions import FindPackageShare
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
from launch.actions import Shutdown
import os

def generate_launch_description():
    ## ***** File paths ******
    package_name = 'robot_cartographer'
    urdf_name = "fishbot_gazebo.urdf"
    pkg_share = FindPackageShare(package=package_name).find(package_name) 
    # pkg_share_this = FindPackageShare('robot_cartographer').find('robot_cartographer') 

    # urdf_file = os.path.join(FindPackageShare('robot_description').find('robot_description') , f'urdf/{urdf_name}')
    # with open(urdf_file, 'r') as infp:
    #     robot_desc = infp.read()
    # 配置文件夹路径
    configuration_directory = LaunchConfiguration('configuration_directory',default= os.path.join(pkg_share, 'config') )
    # 配置文件
    configuration_basename = LaunchConfiguration('configuration_basename', default='backpack_2d_localization.lua')
    # 地图文件
    load_state_filename =  LaunchConfiguration('load_state_filename', default=os.path.join(pkg_share, 'maps','oiu.pbstream'))

    nav2_bringup_dir = get_package_share_directory('nav2_bringup')
    map_yaml_file = os.path.join(pkg_share, 'maps', 'lll_map.yaml')
    ## ***** Nodes **
    # ***
    # robot_state_publisher_node = Node(
    #     package = 'robot_state_publisher',
    #     executable = 'robot_state_publisher',
    #     parameters=[
    #         {'robot_description': robot_desc},
    #         {'use_sim_time': False}],
    #     output = 'screen'
    #     )

    cartographer_node = Node(
        package='cartographer_ros',
        executable='cartographer_node',
        name='cartographer_node',
        output='screen',
        parameters = [{'use_sim_time': False}],
        arguments = [
            '-configuration_directory', configuration_directory,
            '-configuration_basename', configuration_basename,
            '-load_state_filename', load_state_filename],
        remappings = [
            ('echoes', 'scan')],
        )

    cartographer_occupancy_grid_node = Node(
        package='cartographer_ros',
        executable='cartographer_occupancy_grid_node',
        name='cartographer_occupancy_grid_node',
        output = 'screen',
        parameters = [
            {'use_sim_time': False},
            # {'-pure_localization': True},
            {'resolution': 0.05}],
        )

    rviz_node = Node(
        package = 'rviz2',
        executable = 'rviz2',
        on_exit = Shutdown(),
        arguments = ['-d', pkg_share + '/rviz/LL.rviz'],
        parameters 
        = [{'use_sim_time': False}],
    )


    # serial_port_front = LaunchConfiguration('serial_port', default='/dev/ttyUSB0')
    # frame_idF_front = LaunchConfiguration('frame_id', default='laser_1')
    # scan_front = LaunchConfiguration('lidar_scan', default='/scan')
    # delta_lidar_front = Node(
    #     package='lidar',
    #     executable='delta_lidar_node',
    #     output='screen',
    #     parameters=[{'serial_port':serial_port_front},{'frame_id':frame_idF_front},{'lidar_scan':scan_front}]
    #     )

    # serial_port_behind= LaunchConfiguration('serial_port', default='/dev/ttyUSB1')
    # frame_id_behind = LaunchConfiguration('frame_id', default='laser_2')
    # scan_behind = LaunchConfiguration('lidar_scan', default='/scan_2')
    # delta_lidar_behind = Node(
    #     package='lidar',
    #     executable='delta_lidar_node',
    #     output='screen',
    #     parameters=[{'serial_port':serial_port_behind},{'frame_id':frame_id_behind},{'lidar_scan':scan_behind}]
    #     )

    
    
    return LaunchDescription([
        # (delta_lidar_front),
        # (delta_lidar_behind),   
        cartographer_node,
        cartographer_occupancy_grid_node,
        # rviz_node,
    ])
