#pragma once

#include <memory>

// std::unique_ptr
template <class T, class U = std::default_delete<T>>
using Ptr = std::unique_ptr<T, U>;

template <class T, class U = std::default_delete<T>, typename... Args>
constexpr Ptr<T, U> MakePtr(Args&&... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

// std::shared_ptr
template <class T>
using Ref = std::shared_ptr<T>;

template <typename T, typename... Args>
constexpr Ref<T> MakeRef(Args&&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

// std::weak_ptr
template <class T>
using Weak = std::weak_ptr<T>;
