#ifndef TYPES_H
#define TYPES_H

#if defined(DEBUG) || defined(_DEBUG) || defined(NDEBUG)
#	define ENG_DEBUG
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <map>
#include <unordered_map>
#include <array>
#include <initializer_list>
#include <functional>
#include <tuple>
#include <algorithm>
#include <memory>
//#include "optional.hpp"
#include <experimental/optional>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;

using String = std::string;
template <typename E, u32 SZ> using Array = std::array<E, SZ>;
template <typename E> using Vector = std::vector<E>;
template <typename E> using IList = std::initializer_list<E>;
template <typename E> using Queue = std::queue<E>;
template <typename E> using Stack = std::stack<E>;
template <typename K, typename V> using Map = std::map<K, V>;
template <typename K, typename V> using UMap = std::unordered_map<K, V>;
template <typename F> using Fn = std::function<F>;

template <typename T> using opt = std::experimental::optional<T>;
template <typename T> using wptr = std::weak_ptr<T>;
template <typename T> using uptr = std::unique_ptr<T>;
template <typename T> using sptr = std::shared_ptr<T>;

#define mov(x) std::move(x)
#define tup(...) std::make_tuple(__VA_ARGS__)
#define bstr(x) ((u8*)x)

#ifdef USE_NAMESPACE
#	define NS_BEGIN namespace eng {
#	define NS_END }
#else
#	define NS_BEGIN
#	define NS_END
#endif

#include "json.hpp"
using JSON = nlohmann::json;

class Util {
public:
	static const String currentDateTime();
	static const String currentDateTimeNoFormat();
	static i32 random(i32 min, i32 max);
	static float random(float min, float max);
	static double getTime();
};

#endif // TYPES_H