#include "CppDemo/03_shader/shader.h"
#include "Demo/01_createwindow/createwindow.h"
#include "Demo/02_createVulkanInstance/createVulkanInstance.h"
#include "Demo/03_physicalDeviceAndQueue/physicalDeviceAndQueue.h"

#include "CppDemo/01_context/context.h"
#include "Util/fileutil.h"
#include <boost/filesystem/path.hpp>
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "launch args must specify path of Resources" << std::endl;
        return -1;
    }

    boost::filesystem::path exePath = argv[0];
    boost::filesystem::path resourcesPath = argv[1];
#ifndef NDEBUG
    std::cout << "exePath: " << exePath << std::endl;
    std::cout << "resourcesPath: " << resourcesPath << std::endl;
#endif
    //return _01::createWindow();
    //return _02::createVulkanInstance();
    //return _03::physicalDeviceAndQueue();
    //return cpp_demo::contextDemo();
    return cpp_demo::shaderDemo(resourcesPath);
}