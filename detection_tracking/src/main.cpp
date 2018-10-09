/*  Copyright 2012 UdeS University of Sherbrooke
 *
 *   This file is part of ROS_OpenTLD.
 *
 *   ROS_OpenTLD is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   ROS_OpenTLD is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with ROS_OpenTLD. If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*
 * main.cpp
 *
 *  Created on: June 8, 2012
 *      Author: Ronan Chauvin
 */

#include "main.hpp"
#include <ros/time.h>
#include <ros/duration.h>
#include <sensor_msgs/image_encodings.h>

namespace enc = sensor_msgs::image_encodings;

void Main::process()
{
    if(autoFaceDetection && !face_cascade.load(face_cascade_path))
    {
        ROS_FATAL("--(!)Error loading cascade detector\n"); 
        return;
    }
    while (ros::ok())
    {   
        switch (state)
        {
            case INIT:
                if(newImageReceived())
                {
                    if(showOutput)
                        sendTrackedObject(0, 0, 0, 0, 0.0);
                    getLastImageFromBuffer();
                    tld->detectorCascade->imgWidth = gray.cols;
                    tld->detectorCascade->imgHeight = gray.rows;
                    tld->detectorCascade->imgWidthStep = gray.step;

                    state = TRACKER_INIT;
                }
                break;
            case TRACKER_INIT:
                if(loadModel && !modelImportFile.empty())
                {
                    ROS_INFO("Loading model %s", modelImportFile.c_str());

                    tld->readFromFile(modelImportFile.c_str());
                    tld->learningEnabled = false;
                    state = TRACKING;
                }
                else if(autoFaceDetection || correctBB)
                {
                    if(autoFaceDetection)
                    {
                        target_image = gray;
                        target_bb = faceDetection();
                    }

                    sendTrackedObject(target_bb.x,target_bb.y,target_bb.width,target_bb.height,1.0);

                    ROS_INFO("Starting at %d %d %d %d\n", target_bb.x, target_bb.y, target_bb.width, target_bb.height);

                    tld->selectObject(target_image, &target_bb);
                    tld->learningEnabled = true;
                    state = TRACKING;
                }
                else
                {
                    ros::Duration(1.0).sleep();
                    ROS_INFO("Waiting for a BB");
                }
              break;
              case TRACKING:
                if(newImageReceived())
                {
                  ros::Time tic = ros::Time::now();

                  getLastImageFromBuffer();
                  tld->processImage(img);

                  ros::Duration toc = (ros::Time::now() - tic);
                  float fps = 1.0/toc.toSec();

                  std_msgs::Float32 msg_fps;
                  msg_fps.data = fps;
                  pub2.publish(msg_fps);
                  if(showOutput)
                  {
                    if(tld->currBB != NULL)
                    {
                      sendTrackedObject(tld->currBB->x,tld->currBB->y,tld->currBB->width,tld->currBB->height,tld->currConf);
                    }
                    else
                    {
                      sendTrackedObject(1, 1, 1, 1, 0.0);
                    }
                  }
                }
                break;
                case STOPPED:
                    ros::Duration(1.0).sleep();
                    ROS_INFO("Tracker stopped");
                break;
                  default:
                break;
                }
        }

        if(exportModelAfterRun)
        {
          tld->writeToFile(modelExportFile.c_str());
        }

        semaphore.unlock();
    }

  void Main::hpec_process(const boost::shared_ptr<ros::NodeHandle> &workerHandle_ptr, double rate_double)
  {
    ros::CallbackQueue queue;
	  workerHandle_ptr->setCallbackQueue(&queue);
    ros::Rate rate(rate_double);

    if(autoFaceDetection && !face_cascade.load(face_cascade_path))
    {
        ROS_FATAL("--(!)Error loading cascade detector\n"); 
        return;
    }

    std::cout << "RATE VALUE = " << rate_double << std::endl;  

    while (workerHandle_ptr->ok())
    {  
      rate.sleep(); //EM, sleep time computed to respect the node_activation_rate of the app
    	queue.callAvailable();
      //EM, This call checks if a something from a topic has been received 
		  // and run callback functions if yes.
        switch (state)
        {
            case INIT:
                //EM, Add a test to wait the first image callback
                if(get_first_image)
                {
                    if(showOutput)
                        sendTrackedObject(0, 0, 0, 0, 0.0);

                    #ifdef HIL //EM, useless if we are in HIL
                    #else
                      hpecGetLastImageFromBuffer();
                    #endif
                    tld->detectorCascade->imgWidth = gray.cols;
                    tld->detectorCascade->imgHeight = gray.rows;
                    tld->detectorCascade->imgWidthStep = gray.step;

                    state = TRACKER_INIT;
                }
                break;
            case TRACKER_INIT:
                if(loadModel && !modelImportFile.empty())
                {
                    ROS_INFO("Loading model %s", modelImportFile.c_str());

                    tld->readFromFile(modelImportFile.c_str());
                    tld->learningEnabled = false;
                    state = TRACKING;
                }
                else if(autoFaceDetection || correctBB)
                {
                    if(autoFaceDetection)
                    {
                        target_image = gray;
                        target_bb = faceDetection();
                    }

                    sendTrackedObject(target_bb.x,target_bb.y,target_bb.width,target_bb.height,1.0);

                    ROS_INFO("Starting at %d %d %d %d\n", target_bb.x, target_bb.y, target_bb.width, target_bb.height);

                    tld->selectObject(target_image, &target_bb);
                    tld->learningEnabled = true;
                    state = TRACKING;
                }
                else
                {
                    ros::Duration(1.0).sleep();
                    ROS_INFO("Waiting for a BB");
                }
              break;
              case TRACKING:
                //EM, Add a test in the case the camera topic is not activated
                if(img_buffer_ptr->image.rows != 0 && img_buffer_ptr->image.rows != 0)
                {
                  time_t start = clock();
                  ros::Time tic = ros::Time::now();

                  #ifdef HIL //EM, useless if we are in HIL
                  #else
                    hpecGetLastImageFromBuffer();
                  #endif

                  tld->processImage(img);

                  ros::Duration toc = (ros::Time::now() - tic);
                  float fps = 1.0/toc.toSec();
                  std_msgs::Float32 msg_fps;
                  msg_fps.data = fps;
                  pub2.publish(msg_fps);

                  if(showOutput)
                  {
                    if(tld->currBB != NULL)
                    {
                      sendTrackedObject(tld->currBB->x,tld->currBB->y,tld->currBB->width,tld->currBB->height,tld->currConf);
                    }
                    else
                    {
                      sendTrackedObject(1, 1, 1, 1, 0.0);
                    }
                  }
                  time_t ends = clock();
                  std_msgs::Float32 elapsed_time;
                  elapsed_time.data = ((double)(ends - start)) * 1000 / CLOCKS_PER_SEC;
			            std::cout << "SOFTWARE TLD processing time : " << elapsed_time.data << std::endl;
			  	        elapsed_time.data = ((double)(imcpy_end - imcpy_start)) * 1000 / CLOCKS_PER_SEC;
				          std::cout << "SOFTWARE HIL Image COPY time : " << elapsed_time.data << std::endl;
                  ROS_INFO("Detection and Tracking done!");

                  #ifdef HIL
                    if(img_acquired)
                    {
                      delete picture;
                      //img.release(); 	//EM, Free memory of the allocated current picture
                      img_acquired = false;
                    }
                  #endif
                  
                }
                break;
                case STOPPED:
                    ros::Duration(1.0).sleep();
                    ROS_INFO("Tracker stopped");
                break;
                  default:
                break;
                }
      }

      if(exportModelAfterRun)
      {
        tld->writeToFile(modelExportFile.c_str());
      }
  }


    void Main::imageReceivedCB(const sensor_msgs::ImageConstPtr & msg)
    {
      bool empty = false;
      mutex.lock();

      if(img_buffer_ptr.get() == 0)
      {
        empty = true;
      }

      try
      {
        if (enc::isColor(msg->encoding))
          img_buffer_ptr = cv_bridge::toCvCopy(msg, enc::RGB8);
        else
        {
          img_buffer_ptr = cv_bridge::toCvCopy(msg, enc::MONO8);
          cv::cvtColor(img_buffer_ptr->image, img_buffer_ptr->image, CV_GRAY2BGR);
        }
      }
      catch (cv_bridge::Exception& e)
      {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return;
      }

      if(empty)
      {
        semaphore.unlock();
      }
      mutex.unlock();
    }



    void Main::hpecImageReceivedCB(const sensor_msgs::ImageConstPtr & msg)
    {
      get_first_image = true;

      #ifdef HIL //mandatory cloning to save image from a remote computer
        if(!img_acquired) //EM, condition to avoid useless multiple img captures
        {
          try
          {
            if (enc::isColor(msg->encoding))
              img_buffer_ptr = cv_bridge::toCvCopy(msg, enc::RGB8);
            else
            {
              img_buffer_ptr = cv_bridge::toCvCopy(msg, enc::MONO8);
              cv::cvtColor(img_buffer_ptr->image, img_buffer_ptr->image, CV_GRAY2BGR);
            }
            img_acquired = true; //EM, boolean used to know if a cv::Mat release is needed
            imcpy_start = clock();

            img_header = img_buffer_ptr->header;

            picture = new cv::Mat(img_buffer_ptr->image);
            img = *picture;
            //img = img_buffer_ptr->image.clone();
            //img_buffer_ptr->image.copyTo(img);
            cv::cvtColor(img, gray, CV_BGR2GRAY);
            
            imcpy_end = clock();
          }
          catch (cv_bridge::Exception& e)
          {
            ROS_ERROR("cv_bridge exception: %s", e.what());
            return;
          }
        }
        #else //Zero-copy transfer
          try
          {
            if (enc::isColor(msg->encoding))
              img_buffer_ptr = cv_bridge::toCvCopy(msg, enc::RGB8);
            else
            {
              img_buffer_ptr = cv_bridge::toCvCopy(msg, enc::MONO8);
              cv::cvtColor(img_buffer_ptr->image, img_buffer_ptr->image, CV_GRAY2BGR);
            }
          }
          catch (cv_bridge::Exception& e)
          {
            ROS_ERROR("cv_bridge exception: %s", e.what());
            return;
          }
        #endif
    }

    void Main::hpecGetLastImageFromBuffer()
    {

      imcpy_start = clock();

      img_header = img_buffer_ptr->header;
      img = img_buffer_ptr->image;
      cv::cvtColor(img, gray, CV_BGR2GRAY);

      imcpy_end = clock();

			elapsed_time.data = ((double)(imcpy_end - imcpy_start)) * 1000 / CLOCKS_PER_SEC;
			std::cout << "SOFTWARE Image COPY time : " << elapsed_time.data << std::endl;

      img_buffer_ptr.reset();
    }






    void Main::targetReceivedCB(const tld_msgs::TargetConstPtr & msg)
    {
      reset();
      ROS_ASSERT(msg->bb.x >= 0);
      ROS_ASSERT(msg->bb.y >= 0);
      ROS_ASSERT(msg->bb.width > 0);
      ROS_ASSERT(msg->bb.height > 0);
      ROS_INFO("Bounding Box received");

      target_bb.x = msg->bb.x;
      target_bb.y = msg->bb.y;
      target_bb.width = msg->bb.width;
      target_bb.height = msg->bb.height;

      try
      {
        target_image = cv_bridge::toCvCopy(msg->img, enc::MONO8)->image;
      }
      catch (cv_bridge::Exception& e)
      {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return;
      }

      correctBB = true;
    }

    void Main::cmdReceivedCB(const std_msgs::CharConstPtr & cmd)
    {
      switch (cmd->data)
      {
        case 'b':
          clearBackground();
          break;
        case 'c':
          stopTracking();
          break;
        case 'l':
          toggleLearning();
          break;
        case 'a':
          alternatingMode();
          break;
        case 'e':
          exportModel();
          break;
        case 'i':
          importModel();
          break;
        case 'r':
          reset();
          break;
        default:
          break;
      }
    }

    void Main::sendTrackedObject(int x, int y, int width, int height, float confidence)
    {
      tld_msgs::BoundingBox msg;
      msg.header = img_header; //Add the Header of the last image processed
      msg.x = x;
      msg.y = y;
      msg.width = width;
      msg.height = height;
      msg.confidence = confidence;            
      pub1.publish(msg);
    }

    bool Main::newImageReceived()
    {
      semaphore.lock();
      return true;
    }

    void Main::getLastImageFromBuffer()
    {
      mutex.lock();

      img_header = img_buffer_ptr->header;
      img = img_buffer_ptr->image;

      cv::cvtColor(img, gray, CV_BGR2GRAY);

      img_buffer_ptr.reset();
      mutex.unlock();
    }


    void Main::clearBackground()
    {
      tld::ForegroundDetector* fg = tld->detectorCascade->foregroundDetector;

      if(fg->bgImg.empty())
      {
        gray.copyTo(fg->bgImg);
      }
      else
      {
        fg->bgImg.release();
      }
    }

    void Main::stopTracking()
    {
      if(state == STOPPED)
        state = TRACKING;
      else
        state = STOPPED;
    }

    void Main::toggleLearning()
    {
      tld->learningEnabled = !tld->learningEnabled;
      ROS_INFO("LearningEnabled: %d\n", tld->learningEnabled);   
    }

    void Main::alternatingMode()
    {
      tld->alternating = !tld->alternating;
      ROS_INFO("Alternating: %d\n", tld->alternating);        
    }

    void Main::exportModel()
    {
      ros::NodeHandle np("~");
      np.getParam("modelExportFile", modelExportFile);
      //tld->learningEnabled = false;
      tld->writeToFile(modelExportFile.c_str());  
      ROS_INFO("Exporting model %s", modelExportFile.c_str());
    }

    void Main::importModel()
    {
      ros::NodeHandle np("~");
      np.getParam("modelImportFile", modelImportFile);
      loadModel = true;
      state = TRACKER_INIT;
    }

    void Main::reset()
    {
      correctBB = false;
      state = INIT;
    }

    cv::Rect Main::faceDetection()
    {
      std::vector<cv::Rect> faces;

      while(faces.empty())
      {
        if(newImageReceived())
          getLastImageFromBuffer();

        cv::equalizeHist(gray, gray);   
        face_cascade.detectMultiScale(gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(30, 30));
      }

      return faces[0];
    }


    /*************************************************** 
     *  EM, Attempt to use pthread with mutexes by using
     *  pthread cleanup and push in hpec_process()
     * 
     * 	static void cleanup_handler(void *arg); (in main.hpp)
     * 
     *  pthread_cleanup_push(cleanup_handler,this);
     *  pthread_cleanup_pop(1);
     * 
		void Main::cleanup_handler(void *arg)
		{
    	printf("Called clean-up handlers\n");

      Main * my_main;
      my_main = (Main *) arg;

      printf("Here we go\n");
    	my_main->mutex.unlock();
      printf("CRASH SEMAPHORE\n");
			my_main->semaphore.unlock();
      printf("CRASH MUTEX\n");

      //delete my_main;
		}
    ***************************************************/
