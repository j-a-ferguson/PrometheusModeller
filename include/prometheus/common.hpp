#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <armadillo>

#include <functional>
#include <optional>

namespace prometheus {

using Vec2 = arma::vec2;
using Vec3 = arma::vec3;

template <typename T> using Ref = std::reference_wrapper<T>;
template <typename T> using OptRef = std::optional<Ref<T>>;
template <typename T> using ConstRef = Ref<const T>;
template <typename T> using ConstOptRef = OptRef<const T>;

} // namespace prometheus

#endif
