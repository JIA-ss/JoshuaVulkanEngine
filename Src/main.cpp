#include "Demo/01_createwindow/createwindow.h"
#include "Demo/02_createVulkanInstance/createVulkanInstance.h"
#include "Demo/03_physicalDeviceAndQueue/physicalDeviceAndQueue.h"
int main()
{
    //return _01::createWindow();
    //return _02::createVulkanInstance();
    return _03::physicalDeviceAndQueue();
}