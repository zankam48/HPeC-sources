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
 * Author(s) :  Erwan Moréac, erwan.moreac@univ-ubs.fr (EM)
 * Created on: November, 2018
 *
 * Main file of Detection and Tracking ROS node. Use OpenTLD for the software version.
 * 
 *  1. Use roslaunch with .launch file to execte this app
 *  2. on another terminal, run the command 
 *     "rostopic pub -1 /detection_tracking_mgt_topic std_msgs/Int32 "0" or "1" or "2""
 *************************************************************************************/
#include "detection_tracking_node.hpp"


/************************************************************************
 * Global variables 
**********************************************************************/
time_t start, ends;
struct timeval beginning, current, end;

bool run = false;
int current_ver = 0;
int hardware = 0;
double altitude;

boost::shared_ptr<boost::thread> worker_thread;
boost::shared_ptr<ros::NodeHandle> workerHandle_ptr;
boost::shared_ptr<ros::Publisher> detect_track_pub;
/************************************************************************
 * Global variables 
**********************************************************************/


/*************************************************************
 * detection_tracking_hwsw
 * Author : EM 
 * @param workerHandle_ptr allow the function to get ROS 
 * master events, to handle topics for example
 * 
**************************************************************/
void detection_tracking_hwsw(const boost::shared_ptr<ros::NodeHandle> &workerHandle_ptr)
{
	ROS_INFO("[THREAD][RUNNING][HW]: detection_tracking HARDWARE VERSION \r\n");

	run = true;
	ros::CallbackQueue queue;
	workerHandle_ptr->setCallbackQueue(&queue);

	//EM 22/02/18, Topic Subscribtion (Input)
	ros::Subscriber gps_sub = workerHandle_ptr->subscribe("mavros/global_position/global", 1, gps_callback);

	/**********************************************************************
	* EM, Insert here Topic Publication and Subscription
	**********************************************************************/
	
	//EM, App parameters reading
	// /!\ Node activation rate is mandatory, add it in parameters folder
	double rate_double;
	if (!workerHandle_ptr->getParam("/node_activation_rates/detection_tracking", rate_double))
	{
		ROS_INFO("Could not read detection_tracking activation rate. Setting 1 Hz");
		rate_double = 1;
	}
	ros::Rate rate(rate_double);

	/**********************************************************************
	* EM, Insert here other parameters loading
	**********************************************************************/

	/* EM, create acquire function for Hardware version
	if (acquire() != 0) 
	{
		ROS_INFO("Could not INIT HARDWARE");
	}*/

	std_msgs::Float32 elapsed_time;
	while (workerHandle_ptr->ok()) //Main processing loop
	{
		//EM, This call checks if a somethubg from a topic has been received 
		//	  and run callback functions if yes.
		queue.callAvailable();

		//EM, Start app execution time
		start = clock();


		/**********************************************************************
		 * EM, Insert here some application functions
		**********************************************************************/
		std::cout << "HARD Hello World!" << std::endl;



		//EM, Compute execution time
		ends = clock();
		elapsed_time.data = ((double)(ends - start)) * 1000 / CLOCKS_PER_SEC;
		std::cout << "HARDWARE detection_tracking Processing time : " << elapsed_time.data << std::endl;

		rate.sleep(); //EM, sleep time computed to respect the node_activation_rate of the app
		
	}	 // end while
	run = false;
	ROS_INFO("[THREAD][STOPPED]");
}


/*************************************************************
 * detection_tracking_sw
 * Author : EM 
 * @param workerHandle_ptr allow the function to get ROS 
 * master events, to handle topics for example
 *
**************************************************************/
void detection_tracking_sw(const boost::shared_ptr<ros::NodeHandle> &workerHandle_ptr)
{
	ROS_INFO("[THREAD][RUNNING][SW]: detection_tracking SOFTWARE VERSION \r\n");

	run = true;
	ros::CallbackQueue queue;
	workerHandle_ptr->setCallbackQueue(&queue);

	//EM 22/02/18, Topic Subscribtion (Input) 
	ros::Subscriber gps_sub = workerHandle_ptr->subscribe("mavros/global_position/global", 1, gps_callback);
	
	//EM, App parameters reading
	//Node activation rate is mandatory
	double rate_double;
	if (!workerHandle_ptr->getParam("/node_activation_rates/detection_tracking", rate_double))
	{
		ROS_INFO("Could not read detection_tracking activation rate. Setting 1 Hz");
		rate_double = 1;
	}

	Main * main_node = new Main();

	main_node->hpec_process(workerHandle_ptr, rate_double);

	run = false;
	delete main_node;
	ROS_INFO("[THREAD][STOPPED]");
}


/*************************************************************
 * stop
 * Author : Soguy, EM 
 * 
 * Stop the currently running version of the application
**************************************************************/
void stop()
{
	if (workerHandle_ptr == NULL && worker_thread == NULL)
		return;
	if (workerHandle_ptr != NULL)
		workerHandle_ptr->shutdown();
	if (worker_thread != NULL)
	{
		ROS_INFO("THREAD JOIN");
		worker_thread->timed_join(boost::posix_time::seconds(3));
		if (run)
		{
			ROS_INFO("THREAD CANCELLING");
			if(pthread_cancel(worker_thread->native_handle()) != 0)
			{
				perror("pthread_cancel() error");                                           
    			exit(3);
			}
			
			if (hardware == 1)
			{
				//release(); EM, TODO create a function to release mem allocated for the HW
				hardware = 0;
			}
			ROS_INFO("THREAD CANCELLING DONE");
		}
	}
	// RESET
	//workerHandle_ptr = NULL;
	//worker_thread = NULL;
}



/*******************************************************************
 * managing_controller_request
 * Author : Soguy, EM 
 * @param value, value of the message sent on the topic listened
 * 				0 stop the application
 * 				1 start the software version of the application
 * 				2 start the hardware version of the application
 * 
 * Callback function to receive orders from adaptation manager node
*******************************************************************/
void managing_controller_request(const std_msgs::Int32::ConstPtr &value)
{
	gettimeofday(&current, NULL);

	dbprintf("\n Hardware = %d\n",hardware);
	if(hardware==1){ 
		
		/**********************************************************************
		 * EM, Insert here soft reset on the IP before to switch to another 
		 * mode than HW (For Hardware version only)
		**********************************************************************/		
	}


	switch (value->data)
	{
	case 0: // stop the task
		dbprintf("wrapper_ver %.0f %d\n", ((double)time_micros(&current, &beginning)), value->data);
		stop();
		hardware = 0;
		break;

	case 1: //starting the Software version
		dbprintf("wrapper_ver %.0f %d\n", ((double)time_micros(&current, &beginning)), value->data);
		stop();
		hardware = 0;
		workerHandle_ptr = boost::make_shared<ros::NodeHandle>();
		worker_thread = boost::make_shared<boost::thread>(&detection_tracking_sw, workerHandle_ptr);
		break;
		
	case 2: //starting the HW version
		dbprintf("wrapper_ver %.0f %d\n", ((double)time_micros(&current, &beginning)), value->data);
		stop();
		hardware = 1;
		workerHandle_ptr = boost::make_shared<ros::NodeHandle>();
		worker_thread = boost::make_shared<boost::thread>(&detection_tracking_hwsw, workerHandle_ptr);
		break;

	default:
		break;
	}
}


/**##################################################################
 * MAIN
 * Author : Valentin, Soguy, EM 
 * @param argc, number of argument of the execution
 * @param argv, execution arguments
 * 
 * Launch the ROS node and subscribe to topics in order to wait 
 * for orders.
##################################################################**/
int main(int argc, char **argv)
{
	ros::init(argc, argv, "detection_tracking_node");
	ros::NodeHandle nh;
	
	gettimeofday(&beginning, NULL);
	ros::Subscriber mgt_topic;
	mgt_topic = nh.subscribe("detection_tracking_mgt_topic", 1, managing_controller_request);

	detect_track_pub = boost::make_shared<ros::Publisher>(
	nh.advertise<std_msgs::Float32>("/detection_tracking_notification_topic", 1));
	gettimeofday(&current, NULL);

	dbprintf("wrapper_ver %.0f 0\n", ((double)time_micros(&current, &beginning)));

	ROS_INFO("[TASK WRAPPER][RUNNING] TRACKING ONLINE");
	ros::Rate rate(20.0);
	
	// DEBUG code
	// hardware = 0;
	// workerHandle_ptr = boost::make_shared<ros::NodeHandle>();
	// worker_thread = boost::make_shared<boost::thread>(&detection_tracking_sw, workerHandle_ptr);
	
    while(ros::ok())
    {
        ros::spinOnce();
        rate.sleep();
    }

}


/*******************************************************************
 * gps_callback
 * Author : Valentin, EM 
 * @param position, UAV GPS position from topic listened
 * 
 * Callback function to get GPS position
 * Here, only altitude is used but there are other data on position
 * (?) Sample code, you can erase if useless
*******************************************************************/
void gps_callback(const sensor_msgs::NavSatFix::ConstPtr &position)
{
	altitude = position->altitude;
}


void termination_handler(int signum)
{
	ros::shutdown();
}
