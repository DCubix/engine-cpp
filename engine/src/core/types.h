#ifndef TYPES_H
#define TYPES_H

#ifndef NDEBUG
#	ifndef ENG_DEBUG
#		define ENG_DEBUG
#	endif
#endif

#ifdef _MSVC_LANG
#	define MSVC_LANG  _MSVC_LANG
#else
#	define MSVC_LANG  0
#endif

#define CPP11             (__cplusplus == 201103L )
#define CPP11_OR_GREATER  (__cplusplus >= 201103L || MSVC_LANG >= 201103L )
#define CPP14_OR_GREATER  (__cplusplus >= 201402L || MSVC_LANG >= 201703L )
#define CPP17_OR_GREATER  (__cplusplus >= 201703L || MSVC_LANG >= 201703L )
#define CPP20_OR_GREATER  (__cplusplus >= 202000L || MSVC_LANG >= 202000L )

#if defined(__has_include)
#	define HAS_INCLUDE(arg)  __has_include(arg)
#else
#	define HAS_INCLUDE(arg)  0
#endif

#define HAVE_STD_OPTIONAL (CPP17_OR_GREATER)

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

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using String = std::string;
template <typename E, u32 SZ> using Array = std::array<E, SZ>;
template <typename E> using Vector = std::vector<E>;
template <typename E> using IList = std::initializer_list<E>;
template <typename E> using Queue = std::queue<E>;
template <typename E> using Stack = std::stack<E>;
template <typename K, typename V> using Map = std::map<K, V>;
template <typename K, typename V> using UMap = std::unordered_map<K, V>;
template <typename F> using Fn = std::function<F>;

template <typename T> using wptr = std::weak_ptr<T>;
template <typename T> using uptr = std::unique_ptr<T>;
template <typename T> using sptr = std::shared_ptr<T>;

#if HAVE_STD_OPTIONAL
#include <optional>
template <typename T> using opt = std::optional<T>;
#else
#include <experimental/optional>
template <typename T> using opt = std::experimental::optional<T>;
#endif

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
	static String replace(const String& str, const String& what, const String& by);
};

#define SAFE_RELEASE(x) if (x) { delete x; x = nullptr; }

#endif // TYPES_H
