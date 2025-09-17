#include <FrameGraph/ThreadPool.h>



int main(){
    LOG.Run();
    ThreadPool threadPool(4);

    auto future1 = threadPool.Enqueue([](int a, int b){
        return a + b;
    }, 1, 2);
    auto future2 = threadPool.Enqueue([](int a, int b){
        return a * b;
    }, 3, 4);
    LOG_TRACE("future1 : {}", future1.get());
    LOG_TRACE("future2 : {}", future2.get());
    LOG.Stop();
    return 0;
}