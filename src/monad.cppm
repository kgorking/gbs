module;
#include <version>
export module monad;
import std;

#if 0//def __cpp_deleted_function
#define REASON(x) (x)
#else
#define REASON(x)
#endif

template<typename T> concept range_like = requires(T rng) { std::ranges::begin(rng); std::ranges::end(rng); };
template<typename T> concept optional_like = requires(T opt) { opt.has_value(); opt.value(); };
template<typename T> concept expected_like = optional_like<T> && requires(T exp) { exp.error(); };
//template<typename F> concept function_like = std::invocable < F, decltype([]<typename T>(T const&) { return true; }) > ;

#ifdef _MSC_VER
#pragma warning(disable : 4324)
#endif
template<typename T>
struct alignas(std::hardware_destructive_interference_size) thread_data {
	T data;
	std::size_t id = 0;
	std::binary_semaphore sema{ 0 };
	std::future<void> future;
	char _pad[std::hardware_destructive_interference_size - sizeof(std::size_t) - sizeof(bool) - sizeof(std::binary_semaphore) - sizeof(std::future<void>)]{};
};

template<typename Container, typename T>
	requires requires { typename Container::value_type;  }
constexpr void add_to_container(Container& c, T const& v) {
	if constexpr (std::constructible_from<typename Container::value_type, T>) {
		if constexpr (requires { c.emplace_back(v); })
			c.emplace_back(v);
		else if constexpr (requires { c.push_back(v); })
			c.push_back(v);
		else if constexpr (requires { c.insert(v); })
			c.insert(v);
		else if constexpr (requires { c.insert(c.end(), v); })
			c.insert(c.end(), v);
		else if constexpr (requires { c.insert_range(v); })
			c.insert_range(v);
		else if constexpr (requires { c.emplace(v); })
			c.emplace(v);
		else
			static_assert(false, "Container does not support adding elements.");
	}
	else if constexpr (range_like<T> && std::same_as<typename Container::value_type, std::ranges::range_value_t<T>>) {
		if constexpr (requires { c.append_range(v); })
			c.append_range(v);
		else if constexpr (requires { c.insert_range(v); })
			c.insert_range(v);
		else if constexpr (requires { c.insert_range(v.end(), v); })
			c.insert_range(v.end(), v);
		else if constexpr (requires { c.insert_range_after(v.end(), v); })
			c.insert_range_after(v.end(), v);
		else
			static_assert(false, "Container does not support adding ranges, or I forgot to add code that can.");
	}
	else {
		static_assert(false, "Not sure how to insert this type T into the container. Do the types match?");
	}
}


template<typename T>				struct unwrapped { using type = T; };
template<typename T>				struct unwrapped<std::optional<T>> { using type = T; };
template<typename T, typename E>	struct unwrapped<std::expected<T, E>> { using type = T; };
template<typename T>				using  unwrapped_t = typename unwrapped<T>::type;

template<typename T>
constexpr bool has_value(T const& v) noexcept {
	if constexpr (optional_like<T>)
		return v.has_value();
	else
		return true;
}
template<typename T>
constexpr decltype(auto) unwrap(T&& v) noexcept {
	if constexpr (optional_like<T>) {
		//[[assume(v.has_value())]];
		return std::forward<T>(v).value();
	}
	else
		return std::forward<T>(v);
}
template<typename T, typename Fn>
constexpr decltype(auto) unwrap_then(T&& v, Fn&& fn) noexcept {
	if constexpr (optional_like<T>) {
		if (v.has_value())
			std::forward<Fn>(fn)(std::forward<T>(v).value());
	}
	else
		std::forward<Fn>(fn)(std::forward<T>(v));
}
template<typename T>
constexpr decltype(auto) unwrap_or(T&& v, auto const val) noexcept {
	if constexpr (optional_like<T>)
		return std::forward<T>(v).value_or(val);
	else
		return std::forward<T>(v);
}

template<typename Fn, typename T, typename ...Args>
decltype(auto) call(Fn&& fn, T const& v, Args&& ...args) {
	if constexpr (requires {std::tuple_size<unwrapped_t<T>>::value == 0; })
		return std::apply(fn, std::tuple_cat(unwrap(v), std::make_tuple( std::forward<Args>(args)... )));
	else
		return std::invoke(fn, unwrap(v), std::forward<Args>(args)...);
}

template<typename Fn, typename T, typename ...Args>
concept callable = requires(Fn && fn, T const& v, Args&& ...args) {
	{ call(std::forward<Fn>(fn), unwrap(v), std::forward<Args>(args)...) };
};

template<typename Fn, typename T, typename ...Args>
using callable_result_t = std::remove_cvref_t<decltype(call(std::declval<Fn>(), std::declval<unwrapped_t<T>>(), std::declval<Args>()...))>;

template<typename UserFn, typename T, typename ...Args>
concept must_return_void = std::is_same_v<void, callable_result_t<UserFn, T, Args...>>;


export template<typename T, typename Fn>
class monad {
	Fn fn;

	// Allow access to private constructor of other monads
	template<typename, typename InFn>
	friend class monad;

	// Allow access to 'as_monad' function
	template<typename Mt> friend constexpr auto as_monad(Mt const&) noexcept;
	template<typename InitT>  friend constexpr auto as_monad(std::initializer_list<InitT> const) noexcept;

	// This function does nothing. Good for reference.
	constexpr auto identity() const {
		auto f = [=, fn = fn](auto dst) {
			bool const retval = fn([=](auto const& v) {
				unwrap_then(v, dst);
				});

			return retval;
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	constexpr explicit monad(Fn&& in_fn) : fn(std::forward<Fn>(in_fn)) {}
public:
	// Disable construction and assignment
	monad() = delete REASON("Use 'as_monad()'");
	monad(auto const& val) = delete REASON("Use 'as_monad()'");
	monad(monad const&) = delete REASON("No");
	monad(monad&&) = delete REASON("No");
	void operator=(monad const&) = delete REASON("No");
	void operator=(monad&&) = delete REASON("No");

	// Apply a filter predicate to the monad.
	// The predicate must be callable with the unwrapped type of T.
	template<typename Pred>
		requires callable<Pred, unwrapped_t<T>>&& std::is_same_v<bool, callable_result_t<Pred, unwrapped_t<T>>>
	constexpr auto filter(Pred pred) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](const auto& v) {
				if (has_value(v)) {
					const auto& uv = unwrap(v);
					if (call(pred, uv))
						dst(uv);
				}
				});
			};
		using F = decltype(f);
		using Filter = monad<unwrapped_t<T>, F>;
		return Filter{ std::move(f) };
	}

	template<typename TypeHack = std::conditional_t<std::is_class_v<T>, T, std::nullopt_t>>
		requires std::is_class_v<T>
	constexpr auto filter(bool (TypeHack::* predicate)() const) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](const auto& v) {
				if (has_value(v)) {
					unwrapped_t<T> const& uv = unwrap(v);
					if (call(predicate, uv))
						dst(uv);
				}
				});
			};
		using F = decltype(f);
		return ::monad<unwrapped_t<T>, F>{std::move(f)};
	}

	// Map the current type T to another type using the provided mapping function.
	// Additional arguments can be passed to the mapping function.
	template<typename MapFn, typename ...Args>
		requires callable<MapFn, unwrapped_t<T>, Args...>
	constexpr auto map(MapFn const& mf, Args&& ...args) const {
		auto f = [=, fn = fn, ...args = std::forward<Args>(args)](auto dst) {
			return fn([=](auto const& v) {
				if (has_value(v)) {
					dst(call(mf, unwrap(v), args...));
				}
				});
			};
		using F = decltype(f);
		return monad<callable_result_t<MapFn, unwrapped_t<T>, Args...>, F> {std::move(f)};
	}

	// Extract the N'th element from a tuple-like type.
	template<std::size_t N>
		requires (N < std::tuple_size_v<unwrapped_t<T>>)
	constexpr auto element() const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) {
				if (has_value(v)) {
					dst(std::get<N>(unwrap(v)));
				}
				});
			};
		using F = decltype(f);
		using ElementT = std::tuple_element_t<N, unwrapped_t<T>>;
		return monad<ElementT, F>{std::move(f)};
	}

	// If the type is a tuple of at least two elements, return the first element
	constexpr auto keys() const requires (std::tuple_size_v<unwrapped_t<T>> >= 2) {
		return element<0>();
	}

	// If the type is a tuple of at least two elements, return the second element
	constexpr auto values() const requires (std::tuple_size_v<unwrapped_t<T>> >= 2) {
		return element<1>();
	}

	// Concatenate multiple types into a single monad.
	template<typename ...Ts>
	constexpr auto concat(Ts const&... ts) const {
		auto make_fn = [](auto const& v) {
			return [&v](auto dst) {
				unwrap_then(v, dst);
				};
			};

		auto f = [fn = fn, ...fns = make_fn(ts)](auto dst) {
			fn(dst);
			(fns(dst), ...);
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	// Prefix types before the monad.
	template<typename ...Ts>
	constexpr auto prefix(Ts const&... ts) const {
		auto make_fn = [](auto const& v) {
			return [&v](auto dst) {
				unwrap_then(v, dst);
				};
			};

		auto f = [fn = fn, ...fns = make_fn(ts)](auto dst) {
			(fns(dst), ...);
			fn(dst);
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	// Link two monads together, so that both are executed in sequence.
	template<typename OtherT, typename OtherFn>
	constexpr auto link(monad<OtherT, OtherFn> const& m) const {
		auto f = [fn = fn, &m](auto dst) {
			fn(dst);
			m.fn(dst);
			};
		using F = decltype(f);
		return monad<T, F>{std::move(f)};
	}

	// ...
	template<typename OtherT, typename OtherFn>
	constexpr auto cartesian_product(monad<OtherT, OtherFn> const& m) const {
		auto f = [fn = fn, &m](auto dst) {
			m.fn([=](auto const& v_outer) {
				fn([=](auto const& v_inner) {
					if (has_value(v_inner) && has_value(v_outer)) {
						dst(std::make_tuple(unwrap(v_inner), unwrap(v_outer)));
					}
					});
				});
			};
		using F = decltype(f);
		return monad<T, F>{std::move(f)};
	}

	// Zip two monads together. Due to the architecture of this monad implementation, the zipping is done using a separate thread.
	template<typename OtherT, typename OtherFn>
	constexpr auto zip(monad<OtherT, OtherFn> const& m) const {
		auto f = [fn = fn, &m](auto dst) -> void {
			OtherT right{};
			std::binary_semaphore sema_left{ 0 };
			std::binary_semaphore sema_right{ 1 };
			std::atomic_bool done = false;

			std::jthread t{ [&]() {
				m.fn([&](OtherT const& v_right) {
					if (done) return;

					if (has_value(v_right)) {
						sema_right.acquire();
						right = v_right;
						sema_left.release();
					}
					});
				done = true;
				} };

			fn([&](auto const& v_left) {
				if (done) return;

				sema_left.acquire();
				if (has_value(v_left) /*&& has_value(right)*/) {
					dst(std::make_tuple(unwrap(v_left), unwrap(right)));
				}
				sema_right.release();
				});
			done = true;
			};
		using F = decltype(f);
		return monad<std::tuple<unwrapped_t<T>, unwrapped_t<OtherT>>, F>{std::move(f)};
	}

	// Flattens a contained range-like type into a sequence of its elements.
	constexpr auto join(std::int64_t const drop = 0, std::int64_t const take = std::numeric_limits<std::int64_t>::max()) const requires range_like<unwrapped_t<T>> {
		auto f = [=, fn = fn](auto dst) {
			fn([=](auto const& v) {
				if (has_value(v)) {
					auto const& uv = unwrap(v);
					std::int64_t const first = std::max(0ll, drop);
					auto it = std::next(std::ranges::begin(uv), first);
					auto const end = std::ranges::end(uv);

					for (std::int64_t i = 0; i < take && it != end; ++i, ++it) {
						dst(*it);
					}
				}
				});
			};
		using F = decltype(f);
		using VT = std::ranges::range_value_t<unwrapped_t<T>>;
		return monad<VT, F>{std::move(f)};
	}

	// Flattens a contained range-like type into a sequence of its elements, in parallel.
	constexpr auto join_par() const requires range_like<unwrapped_t<T>> {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) {
				if (has_value(v)) {
					auto const& uv = unwrap(v);
					std::for_each(std::execution::par, std::ranges::begin(uv), std::ranges::end(uv), [dst](auto const& item) {
						dst(item);
						});
				}
				});
			};
		using F = decltype(f);
		using VT = std::ranges::range_value_t<unwrapped_t<T>>;
		using MonadJoin = monad<VT, F>;
		return MonadJoin{ std::move(f) };
	}

	// Flattens a contained range-like type into a sequence of its elements, in parallel.
	constexpr auto join_par(std::int64_t const drop, std::int64_t const take) const requires range_like<unwrapped_t<T>>&& std::ranges::sized_range<unwrapped_t<T>> {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) {
				if (has_value(v)) {
					auto const& uv = unwrap(v);
					std::int64_t const begin = std::max(0ll, drop);
					std::int64_t const count = std::min(std::ranges::ssize(uv) - begin, take);
					//int const end = begin + count;

					auto it = uv.begin() + begin;
					std::for_each(std::execution::par, it, it + count, [&](auto const& item) {
						dst(item);
						});
				}
				});
			};
		using F = decltype(f);
		using VT = std::ranges::range_value_t<unwrapped_t<T>>;
		using MonadJoin = monad<VT, F>;
		return MonadJoin{ std::move(f) };
	}

	// Joins a contained range-like type into a sequence of its elements, separated by the provided pattern.
	template<typename P>
		requires range_like<unwrapped_t<T>>&& std::is_same_v<P, unwrapped_t<std::ranges::range_value_t<unwrapped_t<T>>>>
	constexpr auto join_with(P&& pattern, std::int64_t drop = 0, std::int64_t take = std::numeric_limits<std::int64_t>::max()) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) {
				if (has_value(v)) {
					auto const& uv = unwrap(v);
					if (uv.empty())
						return;

					std::int64_t const begin = std::max(0ll, drop);
					std::int64_t const count = std::min(std::ranges::ssize(uv) - begin, take);

					auto it = uv.begin() + begin;
					auto const end = it + count - 1;
					for(; it != end; ++it) {
						dst(*it);
						dst(pattern);
					}

					dst(uv.back());
				}
				});
			};

		using F = decltype(f);
		using VT = std::ranges::range_value_t<unwrapped_t<T>>;
		using JoinWith = monad<VT, F>;
		return JoinWith{ std::move(f) };
	}

	// Joins a contained range-like type into a sequence of its elements, separated by the provided pattern.
	template<std::size_t S>
	constexpr auto join_with(const char(&pattern)[S], std::int64_t drop = 0, std::int64_t take = std::numeric_limits<std::int64_t>::max()) const {
		return join_with(std::string_view{ pattern }, drop, take);
	}

	// Joins a contained range-like type into a sequence of its elements, while replacing a single element.
	template<typename P>
		requires range_like<unwrapped_t<T>>&& std::is_same_v<P, unwrapped_t<std::ranges::range_value_t<unwrapped_t<T>>>>
	constexpr auto replace(P const pattern, P const replace) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) {
				if (has_value(v)) {
					auto const& uv = unwrap(v);
					if (uv.empty())
						return;

					std::for_each(std::ranges::begin(uv), std::ranges::end(uv), [dst, pattern, replace](auto const& item) {
						if (item == pattern)
							dst(replace);
						else
							dst(item);
						});
				}
				});
			};

		using F = decltype(f);
		using VT = std::ranges::range_value_t<unwrapped_t<T>>;
		using JoinWith = monad<VT, F>;
		return JoinWith{ std::move(f) };
	}

	// Drop the first 'n' elements from a contained range-like type.
	constexpr auto drop(std::int64_t n) const requires range_like<unwrapped_t<T>> {
		return join(n);
	}

	// Take only the first 'n' elements from a contained range-like type.
	constexpr auto take(std::int64_t n) const requires range_like<unwrapped_t<T>> {
		return join(0, n);
	}

	// Split the incoming sequence into parts, separated by the provided delimiter.
	template<typename D>
	constexpr auto split(D const delimiter) const {
		static_assert(std::is_same<unwrapped_t<T>, D>::value, "Input type 'T' and delimiter type 'D' are not comparable; maybe call 'join()' before this function?");

		constexpr bool use_string_as_container = std::same_as<T, char>;
		using Container = std::conditional_t<use_string_as_container, std::basic_string<T>, std::vector<T>>;

		auto f = [=, fn = fn](auto dst) {
			Container part{};

			fn([=, &part](auto const& v) {
				if (has_value(v)) {
					unwrapped_t<T> const& uv = unwrap(v);

					if (uv == delimiter) {
						dst(part);
						part.clear();
					}
					else {
						add_to_container(part, uv);
					}
				}
				});

			dst(part);
			};
		using F = decltype(f);
		return monad<Container, F>{std::move(f)};
	}

	// Split the incoming sequence into parts, separated by the provided delimiter. Faster than 'split', but requires a maximum size for each part.
	template<std::size_t MaxSplitSize, typename D>
	constexpr auto split_fast(D const delimiter) const {
		static_assert(std::is_same_v<unwrapped_t<T>, D>, "Input type 'T' and delimiter type 'D' are not comparable; maybe call 'join()' before this function?");

		constexpr bool is_string_type = std::same_as<unwrapped_t<T>, char>;
		using View = std::conditional_t<is_string_type, std::string_view, std::span<unwrapped_t<T>>>;
		using Container = std::array<T, MaxSplitSize>;

		auto f = [delimiter, fn = fn](auto dst) {
			Container part{};
			std::size_t i = 0;

			fn([=, &part, &i](auto const& v) {
				if (has_value(v)) {
					unwrapped_t<T> const& uv = unwrap(v);
					if (uv == delimiter) {
						dst(View{ part.data(), i });
						i = 0;
					}
					else {
						part[i++] = uv;
						if (i == MaxSplitSize)
							throw;
					}
				}
				});

			dst(View{ part.data(), i });
			};
		using F = decltype(f);
		using MonadSplitFast = monad<View, F>;
		return MonadSplitFast{ std::move(f) };
	}

	// Repeat each element N times.
	constexpr auto repeat(int N) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) {
				int const n = N;
				if constexpr (optional_like<T>) {
					for (int i = 0; i < n; ++i) {
						unwrap_then(v, dst);
					}
				}
				else {
					for (int i = 0; i < n; ++i) {
						dst(v);
					}
				}
				});
			};
		return monad<T, decltype(f)>{std::move(f)};
	}

	// Repeat each element N times.
	template <int N>
	constexpr auto repeat() const {
		return repeat(N);
	}

	// Convert the contained type to another type, if possible.
	template<typename Cast, typename... Args>
		requires std::constructible_from<Cast, unwrapped_t<T>, Args...>
	|| std::constructible_from<Cast, std::from_range_t, unwrapped_t<T>, Args...>
		constexpr auto as(Args&& ...args) const {
		auto f = [=, fn = fn, ...args = std::forward<Args>(args)](auto dst) {
			return fn([=](auto const& v) {
				if (has_value(v)) {
					if constexpr (std::constructible_from<Cast, unwrapped_t<T>>) {
						dst(Cast{ unwrap(v), args... });
					}
					else {
						dst(Cast{ std::from_range, unwrap(v), args... });
					}
				}
				});
			};
		using F = decltype(f);
		return monad<Cast, F>{std::move(f)};
	}

	// Project values from a type into a tuple of values.
	template<typename ...Projs>
	constexpr auto project(Projs const ...projs) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) {
				if (has_value(v)) {
					dst(std::tuple{ std::invoke(projs, unwrap(v))... });
				}
				});
			};

		using Tuple = std::tuple<std::invoke_result_t<Projs, unwrapped_t<T>>...>;
		using F = decltype(f);
		return monad<Tuple, F>{std::move(f)};
	}

	// Extract the value, or return the provided default value if no value is present.
	template<typename Other>
	constexpr auto value_or(Other const& other) const requires optional_like<T> {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) {
				if (has_value(v))
					dst(unwrap(v));
				else
					dst(other);
				});
			};
		using F = decltype(f);
		return monad<typename T::value_type, F>{std::move(f)};
	}

	// If the type is an std::expected, call the provided error handler if no value is present.
	constexpr auto unexpected(auto err_handler) const requires expected_like<T> {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) {
				if (has_value(v))
					dst(unwrap(v));
				else
					err_handler(v.error());
				});
			};
		using F = decltype(f);
		return monad<typename T::value_type, F>{std::move(f)};
	}

	// Call the provided function with the unwrapped value, if a value is present. Does not change the type of the monad.
	template<typename UserFn, typename ...Args>
		//requires must_return_void<UserFn, unwrapped_t<T>, Args...>
	constexpr auto and_then(UserFn&& user_fn, Args&& ...args) const {
		auto f = [user_fn = std::forward<UserFn>(user_fn), ...args = std::forward<Args>(args), fn = fn](auto dst) {
			return fn([&](auto const& v) {
				if (has_value(v)) {
					unwrapped_t<T> const& ub = unwrap(v);
					call(user_fn, ub, args...);
					dst(ub);
				}
				});
			};
		using F = decltype(f);
		return monad<T, F>{std::move(f)};
	}

	// If the type is an optional-like type, unbox it to the contained type.
	constexpr auto unbox() const requires optional_like<T> {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) {
				unwrap_then(v, dst);
				});
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	// Guard the following function calls in a try/catch block.
	template<typename Exception = std::exception, typename ExceptionHandler>
		requires std::invocable<ExceptionHandler, Exception const&>
	constexpr auto guard(ExceptionHandler&& exception_handler) const {
		auto f = [=, fn = fn, eh = std::forward<ExceptionHandler>(exception_handler)](auto dst) {
			fn([=](auto const& v) {
				try {
					unwrap_then(v, dst);
				}
				catch (Exception const& e) {
					std::invoke(eh, e);
				}
				catch (...) {
#ifdef __cpp_lib_stacktrace
					std::print(std::cerr, "monad::guard - unhandled exception:\n{}", std::stacktrace::current(0));
#else
					std::println(std::cerr, "monad::guard - unhandled exception");
#endif
					std::terminate();
				}
				});
			};
		using F = decltype(f);
		return monad<T, F>{std::move(f)};
	}

	// This requires data that takes a while to process, in order to be worthwile.
	auto async(std::size_t num_threads = std::thread::hardware_concurrency()) const {
		if (num_threads > std::thread::hardware_concurrency())
			num_threads = std::thread::hardware_concurrency();

		auto f = [=, fn = fn](auto dst) {
			auto tasks = std::vector<thread_data<T>>(num_threads);
			auto task_bitset = std::atomic_size_t{ std::numeric_limits<std::size_t>::max() };
			auto producer_completed = bool{ false };

			auto receiver = [&](thread_data<T>* task) {
				// Enable the task slot
				task_bitset ^= (1ull << task->id);

				// Wait for an initial signal. All threads park here until they are fed data.
				task->sema.acquire();

				// Process data while the producer is not done
				while (!producer_completed) {
					// Do the work
					dst(unwrap(task->data));

					// Re-enable the task slot
					task_bitset ^= (1ull << task->id);

					// Wait for signal
					task->sema.acquire();
				}
				};

			// Start all the threads.
			// The task slot is initially disabled until thread setup is done
			for (std::size_t task_counter = 0; thread_data<T>& task : tasks) {
				task_bitset ^= (1ull << task_counter);
				task.id = task_counter;
				task.future = std::async(std::launch::async, receiver, &task);
				task_counter += 1;
			}

			// Process the data.
			// Each element is fed to an available task slot on its own thread.
			fn([&](auto const& v) {
				if (has_value(v)) {
					// Find available task slot
					std::size_t id = 0;
					do {
						id = static_cast<std::size_t>(std::countr_zero(task_bitset.load()));
					} while (id >= num_threads);

					auto* task = &tasks.at(id);

					// Disable the task slot
					task_bitset ^= (1ull << id);

					// Copy the data to the task slot
					task->data = v;

					// Signal the task to process the data
					task->sema.release();
				}
				});

			// Wait for tasks to finish
			while (task_bitset.load() != std::numeric_limits<std::size_t>::max())
				;

			// Mark the producer as completed and stop all tasks
			producer_completed = true;
			for (auto& task : tasks) {
				task.sema.release();
			}
			for (auto& task : tasks) {
				task.future.get();
			}
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	//
	// Terminal operations
	//

	// Runs the monad and finally calls the provided function.
	template<typename UserFn, typename ...Args>
		requires std::invocable<UserFn&&, unwrapped_t<T>, Args...>
	constexpr void then(UserFn&& user_fn, Args&& ...args) const {
		fn([&](const auto& v) {
			if (has_value(v))
				std::forward<UserFn>(user_fn)(unwrap(v), std::forward<Args>(args)...);
			});
	}

	// Runs the monad until the provided function returns false.
	template<typename UserFn, typename ...Args>
		requires std::invocable<UserFn&&, unwrapped_t<T>, Args...>
	constexpr bool until(UserFn&& user_fn, Args&& ...args) const {
		std::atomic_bool keep_running{ true };
		fn([&](const auto& v) {
			if (keep_running && has_value(v))
				keep_running = keep_running && std::forward<UserFn>(user_fn)(unwrap(v), std::forward<Args>(args)...);
			});

		return keep_running;
	}

	// Sums up the values in the monad.
	template<typename I = unwrapped_t<T>>
		requires (!std::ranges::range<unwrapped_t<T>>)
	constexpr I sum(I init = {}) const {
		fn([&](auto const& v) {
			init += unwrap_or(v, 0);
			});
		return init;
	}

	// Sums up the values in the monad.
	template <typename I = std::int64_t>
		requires std::ranges::range<unwrapped_t<T>>
	constexpr auto sum(I init = I{}) const {
		fn([&](auto const& v) {
			auto const& uv = unwrap(v);
			std::int64_t const size = std::ssize(uv);

			I sum = init;
			for (std::int64_t i = 0; i < size; ++i) {
				sum += unwrap_or(uv[i], 0ll);
			}
			init = sum;
			});
		return init;
	}

	// Counts the number of elements in the monad.
	constexpr std::int64_t count() const {
		std::int64_t c{ 0 };
		fn([&](auto const& v) {
			c += has_value(v);
			return true;
			});
		return c;
	}

	// Sends the final values to the provided container.
	template<typename C>
	constexpr void dest(C& c) const {
		fn([&](auto const& v) {
			static_assert(requires { add_to_container(c, unwrap(v)); }, "Unsupported type can not be added to container.");
			if (has_value(v))
				add_to_container(c, unwrap(v));
			});
	}

	// Sends the final values to the provided function.
	// TODO unneeded?
	template<typename C>
	constexpr auto to_dest(void (*user_fn)(C&, unwrapped_t<T> const&)) const {
		C c{};

		fn([&](auto const& v) {
			if (has_value(v))
				user_fn(c, unwrap(v));
			});

		return c;
	}

	// Sends the final values to the provided container and returns the container.
	template<typename C>
		requires requires (C c, unwrapped_t<T> t) { add_to_container(c, t); }
	constexpr auto to_dest(C& c) const {
		fn([&](T&& v) {
			if (has_value(v)) {
				add_to_container(c, unwrap(v));
			}
			});

		return c;
	}

	// Sends the final values to a new container of the provided type and returns the container.
	template<typename C>
		requires requires (C c, unwrapped_t<T> t) { add_to_container(c, t); }
	constexpr auto to() const {
		C c{};

		fn([&](auto const& v) {
			if (has_value(v))
				add_to_container(c, unwrap(v));
			});

		return c;
	}

	// Sends the final values to a new container of the provided template type and returns the container.
	template<template<class...> typename C>
	constexpr auto to() const {
		return to<C<unwrapped_t<T>>>();
	}

	// Sends the final projected values to a new container of the provided template type and returns the container.
	template<template<class...> typename C, typename ...Projs>
		requires (sizeof...(Projs) > 0 && requires { C<std::remove_cvref_t<std::invoke_result_t<Projs, unwrapped_t<T>>>...>{}; })
	constexpr auto to(Projs ...projs) const {
		C<std::remove_cvref_t<std::invoke_result_t<Projs, unwrapped_t<T>>>...> c;

		fn([&](auto const& v) {
			if (has_value(v))
				add_to_container(c, std::tuple{ std::invoke(projs, unwrap(v))... });
			});

		return c;
	}
};

// Wrap a value into a monad.
export template<typename T>
constexpr auto as_monad(T const& val) noexcept {
	auto f = [&val](auto dst) {
		unwrap_then(val, dst);
		};

	using Fn = decltype(f);
	return ::monad<T, Fn>{ std::move(f) };
}

export template<typename T>
constexpr auto as_monad(std::initializer_list<T> const vals) noexcept {
	auto f = [vals](auto dst) {
		for (auto const& val : vals)
			unwrap_then(val, dst);
		};

	using Fn = decltype(f);
	return ::monad<T, Fn>{ std::move(f) };
}
