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
bool sensorTrigger=false;
unsigned int currentTime_updatedByTopicSubscriber=0;
float simulationTime=0.0;

int timeInMs()
{
    #ifdef _WIN32
        unsigned int t=timeGetTime();
        static unsigned int startT=t;
        if (t>=startT)
            return((t-startT)&0x8fffffff);
        return((t+(0xffffffff-startT))&0x8fffffff);
    #else
        struct timeval now;
        gettimeofday(&now,NULL);
        static time_t initSec=now.tv_sec;
        static suseconds_t initUSec=now.tv_usec;
        return((now.tv_sec-initSec)*1000+(now.tv_usec-initUSec)/1000);
    #endif
}

#ifdef _WIN32
    #define SLEEP Sleep
#else
    #define SLEEP sleep
#endif

// Topic subscriber callbacks:
void sensorCallback(bool sensTrigger)
{
    currentTime_updatedByTopicSubscriber=timeInMs();
    sensorTrigger=sensTrigger;
}

void simulationTimeCallback(float simTime)
{
    simulationTime=simTime;
}

// Main code:
int main(int argc,char* argv[])
{
    std::string leftMotorTopic;
    std::string rightMotorTopic;
    std::string sensorTopic;
    std::string simTimeTopic;

    if (argc>=5)
    {
        leftMotorTopic=argv[1];
        rightMotorTopic=argv[2];
        sensorTopic=argv[3];
        simTimeTopic=argv[4];
    }
    else
    {
        printf("Indicate following arguments: 'leftMotorTopic rightMotorTopic sensorTopic simTimeTopic'!\n");
        SLEEP(5000);
        return 0;
    }

    // Create a B0 node. The name has a random component:
    currentTime_updatedByTopicSubscriber=timeInMs();
    std::string nodeName("b0_bubbleRob");
    std::string randId(boost::lexical_cast<std::string>(currentTime_updatedByTopicSubscriber+int(999999.0f*(rand()/(float)RAND_MAX))));
    nodeName+=randId;
    b0::Node node(nodeName.c_str());


    // 1. Let's subscribe to the sensor and simulation time stream
    b0::Subscriber<bool> sub_sensor(&node,sensorTopic.c_str(),&sensorCallback);
    b0::Subscriber<float> sub_simTime(&node,simTimeTopic.c_str(),&simulationTimeCallback);

    // 2. Let's prepare publishers for the motor speeds:
    b0::Publisher<float> pub_leftMotor(&node,leftMotorTopic.c_str());
    b0::Publisher<float> pub_rightMotor(&node,rightMotorTopic.c_str());

    node.init();

    // 3. Finally we have the control loop:
    float driveBackStartTime=-99.0f;
    unsigned int currentTime;

    currentTime_updatedByTopicSubscriber=timeInMs()/1000;
    currentTime=currentTime_updatedByTopicSubscriber;

    while (!node.shutdownRequested())
    { // this is the control loop (very simple, just as an example)
        currentTime=timeInMs()/1000;
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
            if (sensorTrigger)
                driveBackStartTime=simulationTime; // We detected something, and start the backward mode
            sensorTrigger=false;
        }

        // publish the motor speeds:
        pub_leftMotor.publish(desiredLeftMotorSpeed);
        pub_rightMotor.publish(desiredRightMotorSpeed);

        // handle ROS messages:
        node.spinOnce();

        // sleep a bit:
        SLEEP(5);
    }
    node.cleanup();
    printf("b0_bubbleRob just ended!\n");
    return(0);
}

