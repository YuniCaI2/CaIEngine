//
// Created by 51092 on 2025/10/8.
//

#ifndef CAIENGINE_SERIALIZE_H
#define CAIENGINE_SERIALIZE_H
#include "variable_traits.h"
#include<string>
#include<glm/glm.hpp>
#include<nlohmann/json.hpp>

template<typename T>
struct FieldTrait{};

template<typename Class, typename T>
struct FieldTrait<T Class::*> : variable_traits<T Class::*> {
    constexpr FieldTrait(T Class::* pointer, std::string_view name) : pointer(pointer) {
        this->name = name.substr(name.find_last_of(':') + 1);
    } //萃取传入指针
    T Class::* pointer;
    std::string_view name;
};

template<typename Class, typename T>
FieldTrait(T Class::* pointer, std::string_view name)->FieldTrait<T Class::*>;


template<typename T>
struct TypeInfo {};

#define BEGIN_COMPONENT(X) \
    template<>  \
    struct TypeInfo<X> { \

#define VARIABLES(...) static constexpr auto variables = std::make_tuple(__VA_ARGS__);
#define VARI(V) FieldTrait{V, #V}

#define END_COMPONENT()     \
    }   \
    ;\




namespace nlohmann {

// -------- vec<L, T, Q> ----------
template<glm::length_t L, class T, glm::qualifier Q> // Q位精度控制以及内存布局
struct adl_serializer<glm::vec<L, T, Q>> {
    static void to_json(json& j, glm::vec<L, T, Q> const& v) {
        j = json::array();
        for (glm::length_t i = 0; i < L; ++i) j.push_back(v[i]);
    }
    static void from_json(json const& j, glm::vec<L, T, Q>& v) {
        if (!j.is_array() || j.size() != static_cast<size_t>(L))
            throw std::runtime_error("glm::vec: invalid JSON size");
        for (glm::length_t i = 0; i < L; ++i) v[i] = j[i].get<T>();
    }
};

// JSON: 行优先（R 个数组，每个数组长度 C）
template<glm::length_t C, glm::length_t R, class T, glm::qualifier Q>
struct nlohmann::adl_serializer<glm::mat<C, R, T, Q>> {
    static void to_json(nlohmann::json& j, const glm::mat<C, R, T, Q>& m) {
        j = nlohmann::json::array();
        for (glm::length_t r = 0; r < R; ++r) {
            nlohmann::json row = nlohmann::json::array();
            for (glm::length_t c = 0; c < C; ++c)
                row.push_back(m[c][r]); //很奇怪glm的索引是反的
            j.push_back(std::move(row));
        }
    }
    static void from_json(const nlohmann::json& j, glm::mat<C, R, T, Q>& m) {
        if (!j.is_array() || j.size() != static_cast<size_t>(R))
            throw std::runtime_error("glm::mat: invalid JSON row count");
        for (glm::length_t r = 0; r < R; ++r) {
            const auto& row = j[r];
            if (!row.is_array() || row.size() != static_cast<size_t>(C))
                throw std::runtime_error("glm::mat: invalid JSON column count");
            for (glm::length_t c = 0; c < C; ++c)
                m[c][r] = row[c].get<T>();
        }
    }
};


// -------- qua<T, Q>（四元数） ----------
template<class T, glm::qualifier Q>
struct adl_serializer<glm::qua<T, Q>> {
    static void to_json(json& j, glm::qua<T, Q> const& q) {
        // 约定顺序 [w, x, y, z]
        j = json::array({ q.w, q.x, q.y, q.z });
    }
    static void from_json(json const& j, glm::qua<T, Q>& q) {
        if (!j.is_array() || j.size() != 4)
            throw std::runtime_error("glm::quat: invalid JSON size");
        q.w = j[0].get<T>();
        q.x = j[1].get<T>();
        q.y = j[2].get<T>();
        q.z = j[3].get<T>();
    }


};


} // namespace nlohmann







#endif //CAIENGINE_SERIALIZE_H