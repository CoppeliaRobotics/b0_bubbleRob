#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
    #include <Windows.h>
#endif

#ifdef CPP_VERSION
    #include <b0/node.h>
    #include <b0/publisher.h>
    #include <b0/subscriber.h>
    b0::Node* node=nullptr;
#else
    #include <string>
    extern "C" {
    #include <c.h>
    }
    b0_node* node=nullptr;
#endif

// Global variables (also modified by the topic subscriber):
int sensorTrigger=0;
int pauseFlag=0;
long currentTime_updatedByTopicSubscriber=0;
float simulationTime=0.0;

#ifdef _WIN32
    #define SLEEP_MS(x) Sleep(x)
#else
    #define SLEEP_MS(x) usleep(x*1000)
#endif

// Topic subscriber callbacks:
#ifdef CPP_VERSION
void sensorCallback(const std::string &sensTrigger_packedInt)
{
    sensorTrigger=((int*)sensTrigger_packedInt.c_str())[0];
}

void simulationTimeCallback(const std::string &simTime_packedFloat)
{
    simulationTime=((float*)simTime_packedFloat.c_str())[0];
    currentTime_updatedByTopicSubscriber=(long)node->hardwareTimeUSec()/1000;
}

void pauseCallback(const std::string &pauseFlag_packedInt)
{
    pauseFlag=((int*)pauseFlag_packedInt.c_str())[0];;
    currentTime_updatedByTopicSubscriber=(long)node->hardwareTimeUSec()/1000;
}
#else
void sensorCallback(const void* data,size_t dataLength)
{
    std::string sensTrigger_packedInt((char*)data,(char*)data+dataLength);
    sensorTrigger=((int*)sensTrigger_packedInt.c_str())[0];
}

void simulationTimeCallback(const void* data,size_t dataLength)
{
    std::string simTime_packedFloat((char*)data,(char*)data+dataLength);
    simulationTime=((float*)simTime_packedFloat.c_str())[0];
    currentTime_updatedByTopicSubscriber=(long)b0_node_hardware_time_usec(node)/1000;
}

void pauseCallback(const void* data,size_t dataLength)
{
    std::string pauseFlag_packedInt((char*)data,(char*)data+dataLength);
    pauseFlag=((int*)pauseFlag_packedInt.c_str())[0];;
    currentTime_updatedByTopicSubscriber=(long)b0_node_hardware_time_usec(node)/1000;
}
#endif

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

    float driveBackStartTime=-99.0f;
    long currentTime;
#ifdef CPP_VERSION
    // Create a B0 node.
    b0::init();
    b0::Node _node("b0_bubbleRob");
    node=&_node;

    // 1. Let's subscribe to the sensor, simulation time and pause flag stream:
    b0::Subscriber sub_sensor(node,sensorTopic.c_str(),&sensorCallback);
    b0::Subscriber sub_simTime(node,simTimeTopic.c_str(),&simulationTimeCallback);
    b0::Subscriber sub_pause(node,pauseTopic.c_str(),&pauseCallback);

    // 2. Let's prepare publishers for the motor speeds:
    b0::Publisher pub_leftMotor(node,leftMotorTopic.c_str());
    b0::Publisher pub_rightMotor(node,rightMotorTopic.c_str());

    node->init();

    currentTime_updatedByTopicSubscriber=(long)node->hardwareTimeUSec()/1000;
#else
    // Create a B0 node.
    int arg1=1;
    const char* arg2="b0C";
    b0_init(&arg1,(char**)&arg2);
    node=b0_node_new("b0_bubbleRob");

    // 1. Let's subscribe to the sensor, simulation time and pause flag stream:
    b0_subscriber* sub_sensor=b0_subscriber_new(node,sensorTopic.c_str(),&sensorCallback);
    b0_subscriber* sub_simTime=b0_subscriber_new(node,simTimeTopic.c_str(),&simulationTimeCallback);
    b0_subscriber* sub_pause=b0_subscriber_new(node,pauseTopic.c_str(),&pauseCallback);

    // 2. Let's prepare publishers for the motor speeds:
    b0_publisher* pub_leftMotor=b0_publisher_new(node,leftMotorTopic.c_str());
    b0_publisher* pub_rightMotor=b0_publisher_new(node,rightMotorTopic.c_str());

    b0_node_init(node);

    currentTime_updatedByTopicSubscriber=(long)b0_node_hardware_time_usec(node)/1000;
#endif
    currentTime=currentTime_updatedByTopicSubscriber;
    
    // 3. Finally we have the control loop (very simple, just as an example):
#ifdef CPP_VERSION
    while (!node->shutdownRequested())
    {
        currentTime=(long)node->hardwareTimeUSec()/1000;
#else
    while (b0_node_shutdown_requested(node)==0)
    {
        currentTime=(long)b0_node_hardware_time_usec(node)/1000;
#endif
        if (pauseFlag==0)
        { // simulation not paused
            if (currentTime-currentTime_updatedByTopicSubscriber>8000)
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
            std::string buff1=(char*)(&desiredLeftMotorSpeed);
            std::string buff2=(char*)(&desiredRightMotorSpeed);
#ifdef CPP_VERSION
            pub_leftMotor.publish(buff1);
            pub_rightMotor.publish(buff2);
        }

        // handle B0 messages:
        node->spinOnce();

        // sleep a bit:
        SLEEP_MS(20);
    }
    node->cleanup();
#else
            b0_publisher_publish(pub_leftMotor,buff1.c_str(),buff1.size());
            b0_publisher_publish(pub_rightMotor,buff2.c_str(),buff2.size());
        }

        // handle B0 messages:
        b0_node_spin_once(node);

        // sleep a bit:
        SLEEP_MS(20);
    }
    b0_node_cleanup(node);
    b0_subscriber_delete(sub_sensor);
    b0_subscriber_delete(sub_simTime);
    b0_subscriber_delete(sub_pause);
    b0_publisher_delete(pub_leftMotor);
    b0_publisher_delete(pub_rightMotor);
    b0_node_delete(node);
#endif
    printf("b0_bubbleRob just ended!\n");
    return(0);
}

