/* 
 * This file is part of the HPeC distribution (https://github.com/Kamiwan/HPeC-sources).
 * Copyright (c) 2018 Lab-STICC Laboratory.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
/*************************************************************************************
 * File   : navigation_node.h, file to create easily ros nodes for HPeC
 * Copyright (C) 2018 Lab-STICC Laboratory
 * Author(s) :  Erwan Moréac, erwan.moreac@univ-ubs.fr (EM)
 * Created on: February 21, 2019
 * 
 * You can test this program:
 * 	1. run on a terminal "roscore" to get a ROS master
 *  2. on another terminal, run the command "rosrun navigation navigation_node_node"
 *************************************************************************************/

#ifndef NAVIGATION_NODE_HPP
#define NAVIGATION_NODE_HPP

#include <ros/ros.h>
#include <ros/callback_queue.h>
#include <geometry_msgs/PoseStamped.h>

#include <iostream>
#include <math.h>   /* atan2 */

#include "std_msgs/Int32.h"
#include "std_msgs/Float32.h"
#include "std_msgs/Float64.h"
#include "std_msgs/String.h"
#include "geometry_msgs/TwistStamped.h"
#include <sensor_msgs/NavSatFix.h>
#include <sensor_msgs/image_encodings.h>
#include "sensor_msgs/Imu.h"
#include <tf/transform_datatypes.h>

#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/CommandTOL.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <mavros_msgs/GlobalPositionTarget.h>

#include "communication/nav_control.h"


#define PI          3.14159265
#define RAD_360     6.28319
#define DEG_TO_RAD  0.0174532925
#define MASK_IGNORE_VXYZ_AFXYZ_F 1016

#define MASK_IGNORE_LLA_YAW_RATE 3079
#define DEFAULT_HEIGHT 603.450

enum NavOrder {
    LAND = 0,
    TAKEOFF,
    GPS_MOVE,
    VELOCITY_MOVE,
    OBS_AVOIDANCE_MOVE,
    TRACKING_MOVE,
    INVALID_ORDER
};

bool   flying;

double current_altitude;
double current_latitude;
double current_longitude;

double destination_altitude;
double destination_latitude;
double destination_longitude;

mavros_msgs::State current_state;
std_msgs::Float64 current_heading;
mavros_msgs::SetMode guided_set_mode;

mavros_msgs::CommandBool arm_cmd;
mavros_msgs::CommandTOL takeoff_cmd;
mavros_msgs::CommandTOL landing_cmd;


NavOrder ResolveNavOrder(std::string input);

void StateCallback(const mavros_msgs::State::ConstPtr& msg);
void CompassCallback(const std_msgs::Float64::ConstPtr& msg);
void ImuCallback(const sensor_msgs::Imu::ConstPtr& imu_msg);
void GpsCallback(const sensor_msgs::NavSatFix::ConstPtr& position);
void ControlCallback(const communication::nav_control::ConstPtr& next_order);

#endif