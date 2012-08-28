#include <string>

void makeBundlePath(std::string& path)
{
    NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
    path = std::string([resourcePath UTF8String]) + "/" + path;
}
