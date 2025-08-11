#ifndef SPARSESET_H
#define SPARSESET_H

#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

template <class T, size_t PageSize, class = std::enable_if_t<std::is_integral_v<T>>>
class SparseSet final {
public:
    using value_type = T;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    
    SparseSet() = default;
    ~SparseSet() = default;
    
    // 禁止拷贝（因为包含unique_ptr）
    SparseSet(const SparseSet&) = delete;
    SparseSet& operator=(const SparseSet&) = delete;
    
    // 允许移动
    SparseSet(SparseSet&&) noexcept = default;
    SparseSet& operator=(SparseSet&&) noexcept = default;

    void Add(T t) {
        assert(t != null && "Cannot add null value");
        
        if (Contain(t)) return;
        
        densityArray.push_back(t);
        index(t) = static_cast<T>(densityArray.size() - 1);
    }

    bool Contain(T t) const {
        if (t == null) return false;
        
        const auto p = page(t);
        if (p >= sparseArray.size()) return false;
        
        const auto off = offset(t);
        return sparseArray[p] && sparseArray[p]->at(off) != null;
    }

    void Remove(T t) {
        assert(t != null && "Cannot remove null value");
        if (!Contain(t)) return;
        
        // 将要删除的元素与最后一个元素交换
        const T idx = index(t);
        const T last = densityArray.back();
        
        densityArray[idx] = last;
        index(last) = idx;
        
        densityArray.pop_back();
        index(t) = null;  // 标记为已删除
    }

    void Clear() {
        densityArray.clear();
        sparseArray.clear();
    }

    size_t Size() const { return densityArray.size(); }
    bool Empty() const { return densityArray.empty(); }
    
    // 迭代器支持
    iterator begin() { return densityArray.begin(); }
    iterator end() { return densityArray.end(); }
    const_iterator begin() const { return densityArray.begin(); }
    const_iterator end() const { return densityArray.end(); }
    const_iterator cbegin() const { return densityArray.cbegin(); }
    const_iterator cend() const { return densityArray.cend(); }

private:
    std::vector<T> densityArray;
    std::vector<std::unique_ptr<std::array<T, PageSize>>> sparseArray;

    static constexpr T null = std::numeric_limits<T>::max();

    size_t page(T t) const { return t / PageSize; }
    size_t offset(T t) const { return t % PageSize; }

    T& index(T t) {
        const auto p = page(t);
        const auto off = offset(t);
        
        // 确保有足够的页
        if (p >= sparseArray.size()) {
            sparseArray.resize(p + 1); //创建空间，这里的SparseSet中间可能有空指针
        }
        
        // 如果页不存在则创建
        if (!sparseArray[p]) {
            sparseArray[p] = std::make_unique<std::array<T, PageSize>>();
            sparseArray[p]->fill(null);  // 初始化所有元素为null
        }
        
        return sparseArray[p]->at(off);
    }
    
    const T& index(T t) const {
        const auto p = page(t);
        const auto off = offset(t);
        
        assert(p < sparseArray.size() && "Page out of range");
        assert(sparseArray[p] && "Page not allocated");
        
        return sparseArray[p]->at(off);
    }
};

#endif // SPARSESET_H