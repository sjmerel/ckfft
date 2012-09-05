#include <string>

using namespace std;

void getInputDir(string& dir)
{
    NSString* resourceDir = [[NSBundle mainBundle] resourcePath];
    dir = [resourceDir UTF8String];
}

void getOutputDir(string& dir)
{
    NSString* docDir = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    dir = [docDir UTF8String];
}
