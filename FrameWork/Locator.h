//
// Created by 51092 on 25-6-1.
//

#ifndef LOCATOR_H
#define LOCATOR_H
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <iostream>

//TODO: 线程安全

//注意服务定位器是初始化的第一位，为了防止所用的服务正确注册
namespace FrameWork {
    //为全局访问设置一个统一的入口点
    class Locator {
        inline static std::unordered_map<std::type_index, std::shared_ptr<void>> services;
    public:
        Locator(){};
        template<typename T>
        inline static void RegisterService(std::shared_ptr<void> service) {
            services[std::type_index(typeid(T))] = service;
        }

        //这是一个不好的设计
        template<typename T>
        inline static std::shared_ptr<T> GetService() {
            auto it = services.find(std::type_index(typeid(T)));
            if (it != services.end()) {
                // 这里的static_pointer_cast是智能指针库中的函数，为了保证转换的有效和保证引用计数的一致性
                return std::static_pointer_cast<T>(it->second);
            }else {
                std::cerr << "the service you want didn't register" << std::endl;
                exit(-1);
            }
        }
    };
}

#endif //LOCATOR_H
