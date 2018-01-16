#include <b0/node.h>
#include <b0/publisher.h>
#include <b0/subscriber.h>
#include <stdio.h>
#include <stdlib.h>
#include <boost/lexical_cast.hpp>
#ifdef _WIN32
    #include <Windows.h>
#endif

// Global variables (also modified by the topic subscriber):
int sensorTrigger=0;
int pauseFlag=0;
unsigned int currentTime_updatedByTopicSubscriber=0;
float simulationTime=0.0;
b0::Node* node=NULL;

#ifdef _WIN32
    #define SLEEP_MS(x) Sleep(x)
#else
    #define SLEEP_MS(X) usleep(x*1000)
#endif

// Topic subscriber callbacks:
void sensorCallback(std::string sensTrigger_packedInt)
{
    sensorTrigger=((int*)sensTrigger_packedInt.c_str())[0];
}

void simulationTimeCallback(std::string simTime_packedFloat)
{
    simulationTime=((float*)simTime_packedFloat.c_str())[0];
    currentTime_updatedByTopicSubscriber=int(node->timeUSec()/1000000);
}

void pauseCallback(std::string pauseFlag_packedInt)
{
    pauseFlag=((int*)pauseFlag_packedInt.c_str())[0];;
    currentTime_updatedByTopicSubscriber=int(node->timeUSec()/1000000);
}

// Main code:
int main(int argc,char* argv[])
{
    std::string leftMotorTopic;
    std::string rightMotorTopic;
    std::string sensorTopic;
    std::string simTimeTopic;
    std::string pauseTopic;

    if (argc>=6)
    {
        leftMotorTopic=argv[1];
        rightMotorTopic=argv[2];
        sensorTopic=argv[3];
        simTimeTopic=argv[4];
        pauseTopic=argv[5];
    }
    else
    {
        printf("Indicate following arguments: 'leftMotorTopic rightMotorTopic sensorTopic simTimeTopic pauseFlagTopic'!\n");
        SLEEP_MS(5000);
        return 0;
    }

    // Create a B0 node.
    node=new b0::Node("b0_bubbleRob");

    // 1. Let's subscribe to the sensor, simulation time and pause flag stream:
    b0::Subscriber<std::string> sub_sensor(node,sensorTopic.c_str(),&sensorCallback);
    b0::Subscriber<std::string> sub_simTime(node,simTimeTopic.c_str(),&simulationTimeCallback);
    b0::Subscriber<std::string> sub_pause(node,pauseTopic.c_str(),&pauseCallback);

    // 2. Let's prepare publishers for the motor speeds:
    b0::Publisher<std::string> pub_leftMotor(node,leftMotorTopic.c_str());
    b0::Publisher<std::string> pub_rightMotor(node,rightMotorTopic.c_str());

    node->init();

    // 3. Finally we have the control loop:
    float driveBackStartTime=-99.0f;
    unsigned int currentTime;

    currentTime_updatedByTopicSubscriber=int(node->timeUSec()/1000000);
    currentTime=currentTime_updatedByTopicSubscriber;

    while (!node->shutdownRequested())
    { // this is the control loop (very simple, just as an example)
        currentTime=int(node->timeUSec()/1000000);

        if (pauseFlag==0)
        { // simulation not paused
            if (currentTime-currentTime_updatedByTopicSubscriber>9)
                break; // we didn't receive any sensor information for quite a while... we leave
            float desiredLeftMotorSpeed;
            float desiredRightMotorSpeed;
            if (simulationTime-driveBackStartTime<3.0f)
            { // driving backwards while slightly turning:
                desiredLeftMotorSpeed=-3.1415f*0.5;
                desiredRightMotorSpeed=-3.1415f*0.25;
            }
            else
            { // going forward:
                desiredLeftMotorSpeed=3.1415f;
                desiredRightMotorSpeed=3.1415f;
                if (sensorTrigger>0)
                    driveBackStartTime=simulationTime; // We detected something, and start the backward mode
                sensorTrigger=0;
            }

            // publish the motor speeds:
            std::string buff;
            buff=(char*)(&desiredLeftMotorSpeed);
            pub_leftMotor.publish(buff);
            buff=(char*)(&desiredRightMotorSpeed);
            pub_rightMotor.publish(buff);
        }

        // handle B0 messages:
        node->spinOnce();

        // sleep a bit:
        SLEEP_MS(20);
    }
    node->cleanup();
    delete node;
    printf("b0_bubbleRob just ended!\n");
    return(0);
}

