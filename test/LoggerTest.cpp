#include <Logger.h>

int main() {
    LOG.Run();
    LOG.SetFilePath("../../resources/log/log.txt");

    LOG.Error("Error test : {}", 1234);
    LOG.Warn("Warning test : {}", 1234);
    LOG.DeBug("Debug test : {}", 1234);
    LOG.Trace("Trace test : {}", 1234);

    LOG.Stop();
    return 0;
}