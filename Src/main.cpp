#include "Demo/01_createwindow/createwindow.h"
#include "Demo/02_createVulkanInstance/createVulkanInstance.h"
#include "Demo/03_physicalDeviceAndQueue/physicalDeviceAndQueue.h"

#include "CppDemo/01_context/context.h"
#include "Util/fileutil.h"
int main()
{
    //return _01::createWindow();
    //return _02::createVulkanInstance();
    //return _03::physicalDeviceAndQueue();
    std::string path = "G:/vscode_proj/JoshuaVulkanEngine/Shader/GLSL/shader.frag";
    assert(util::fileExist(path));
    assert(!util::fileExist(path + ".tmp"));
    std::string content;
    std::vector<char> b_content;
    assert(util::readFile(path, content));
    assert(!content.empty());
    assert(util::readFile(path, b_content));
    assert(!b_content.empty());

    assert(util::writeFile(path + ".tmp2", content));
    return cpp_demo::contextDemo();
}