#include <Logger.h>

int main() {
    LOG.Run();
    LOG.SetFilePath("../../resources/log/log.txt");

    ERROR("Error test{}", 1234);
    WARNING("Warning test{}", 1234);
    TRACE("Trace test{}", 1234);
    DEBUG("Debug test{}", 1234);

    LOG.Stop();
    return 0;
}