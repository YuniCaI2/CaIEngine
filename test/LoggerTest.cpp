#include <Logger.h>

int main() {
    LOG.Run();
    LOG.SetFilePath("../../resources/log/log.txt");

    LOG_ERROR("Error test {}", 1234);
    LOG_WARNING("Warning test{}", 1234);
    LOG_TRACE("Trace test{}", 1234);
    LOG_DEBUG("Debug test{}", 123344);

    LOG.Stop();
    return 0;
}