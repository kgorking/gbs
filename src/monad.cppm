module;
#include <version>
export module monad;
import std;

#ifdef __cpp_deleted_function
#define REASON(x) (x)
#else
#define REASON(x)
#endif

template<typename T> concept range_like = requires(T rng) { std::ranges::begin(rng); std::ranges::end(rng); };
template<typename T> concept optional_like = requires(T const& opt) { opt.has_value(); opt.value(); };
template<typename T> concept expected_like = optional_like<T> && requires(T const& exp) { exp.error(); };
template<typename Fn> concept function_like = std::invocable<Fn, decltype([]<typename T>(T const&) -> bool { return true; })>;

template<typename T>
using in = std::conditional_t<std::is_trivially_copyable_v<T> && sizeof(T) <= 2 * sizeof(void*), T const, T const&>;

template<typename T>
struct alignas(std::hardware_destructive_interference_size) task {
	T data;
	bool return_value = true;
	std::binary_semaphore sema{ 0 };
	std::future<void> future;
};

template<typename Container>
	requires requires { typename Container::value_type;  }
//constexpr void add_to_container(Container& c, in<typename Container::value_type> v) {
constexpr void add_to_container(Container& c, auto const& v) {
	if constexpr (requires { c.emplace_back(v); })
		c.emplace_back(v);
	else if constexpr (requires { c.push_back(v); })
		c.push_back(v);
	else if constexpr (requires { c.emplace(v); })
		c.emplace(v);
	else if constexpr (requires { c.insert(c.end(), v); })
		c.insert(c.end(), v);
	else
		static_assert(false, "Container does not support adding elements.");
}

template<typename T>
constexpr bool has_value(T const& v) noexcept {
	if constexpr (optional_like<T>) {
		return v.has_value();
	}
	else {
		return true;
	}
}

template<typename T>
constexpr auto const& unwrap(T const& opt) noexcept {
	if constexpr (optional_like<T>) {
		return opt.value();
	}
	else {
		return opt;
	}
}

template<auto OrValue, typename T>
constexpr auto unbox_or(T const& v) noexcept {
	if constexpr (optional_like<T>) {
		return v.value_or(OrValue);
	}
	else {
		return v;
	}
}

template<typename T>
using unwrapped_t = std::remove_cvref_t<decltype(unwrap(std::declval<T>()))>;

template<typename T>
constexpr auto make_one(T const& val) {
	return [val](auto dst) {
		if(has_value(val))
			return dst(unwrap(val));
		return true;
		};
}

template<range_like T>
constexpr auto make_fn(T const& rng) {
	return [&](auto dst) {
		for (in<typename T::value_type> v : rng) {
			if(has_value(v))
				if (!dst(unwrap(v)))
					return false;
		}

		return true;
		};
}

export
template<typename T, function_like Fn>
class monad {
	Fn fn;

	// Allow acces to private constructor
	template<typename, function_like>
	friend class monad;

	constexpr explicit monad(Fn&& fn) : fn(std::forward<Fn>(fn)) {}
public:
	constexpr explicit monad(range_like auto const& rng) : fn(make_fn(rng)) {}
	constexpr explicit monad(auto const& val) : fn(make_one(val)) {}

	// This function does nothing
	constexpr auto identity() const {
		auto f = [&](auto dst) {

			bool const retval = fn([&](in<T> v) {
				if (has_value(v)) {
					return dst(unwrap(v));
				}
				return true;
				});

			return retval;
			};
		return monad<unwrapped_t<T>, decltype(f)>{std::move(f)};
	}

	auto async(std::size_t num_threads = std::thread::hardware_concurrency()) const {
		if (num_threads > std::thread::hardware_concurrency())
			num_threads = std::thread::hardware_concurrency();

		auto f = [&, num_threads](auto dst) {
			auto tasks = std::vector<task<unwrapped_t<T>>>(num_threads);
			auto task_bitset = std::atomic_size_t{ -1ull };
			auto task_counter = std::atomic_size_t{ 0 };
			auto producer_completed = bool{ false };

			for (task<unwrapped_t<T>>& task : tasks)
				task.future = std::async(std::launch::async, [&] {
						// Get the task id
						std::size_t const id = task_counter++;

						// Wait for an initial signal. All threads park here until they are fed data.
						task.sema.acquire();

						// Process data while the producer is not done
						while (!producer_completed) {
							// Do the work
							task.return_value = dst(task.data);

							// Re-enable the task slot
							task_bitset ^= (1 << id);

							// Wait for signal
							task.sema.acquire();
						}
					});

			bool const retval = fn([&](in<T> v) {
				if (has_value(v)) {
					// Find available task slot
					int id = std::countr_zero(task_bitset.load());

					// If all tasks are busy, do the work in the current thread
					//if (id == num_threads) {
					//	return dst(unwrap(v));
					//}
					// Keep looking until one becomes available
					while (id >= num_threads) {
						id = std::countr_zero(task_bitset.load());
					}

					// Check last return value
					if (!tasks[id].return_value)
						return false;

					// Disable the task slot
					task_bitset ^= (1 << id);

					// Copy the data to the task slot
					tasks[id].data = unwrap(v);

					// Signal the task to process the data
					tasks[id].sema.release();
				}
				return true;
				});

			// Wait for remaining tasks to finish
			while (task_bitset != -1)
				;

			// Mark the producer as completed and stop all tasks
			producer_completed = true;
			for(auto& task : tasks)
				task.sema.release();

			return retval;
			};
		return monad<unwrapped_t<T>, decltype(f)>{std::move(f)};
	}

	constexpr auto filter(std::predicate<T const&> auto pred) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (has_value(v)) {
					in<unwrapped_t<T>> uv = unwrap(v);
					if (std::invoke(pred, uv))
						return dst(uv);
				}
				return true;
				});
			};
		return monad<unwrapped_t<T>, decltype(f)>{std::move(f)};
	}

	template<typename TypeHack = std::conditional_t<std::is_class_v<T>, T, std::nullopt_t>>
		requires std::is_class_v<T>
	constexpr auto filter(bool (TypeHack::* pred)() const) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (has_value(v)) {
					in<unwrapped_t<T>> uv = unwrap(v);
					if (std::invoke(pred, uv))
						return dst(uv);
				}
				return true;
				});
			};
		return monad<unwrapped_t<T>, decltype(f)>{std::move(f)};
	}

	template<std::invocable<unwrapped_t<T>> MapFn>
	constexpr auto map(MapFn mf) const {
		using Result = std::invoke_result_t<MapFn, unwrapped_t<in<T>>>;

		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (has_value(v))
					return dst(std::invoke(mf, unwrap(v)));
				return true;
				});
			};
		return monad<Result, decltype(f)> {std::move(f)};
	}

	constexpr auto take(std::signed_integral auto n) const {
		auto f = [=, fn = fn](auto dst) {
			if (n <= 0)
				return true;

			decltype(n) count = 0;
			return fn([&](in<T> v) {
				if(has_value(v))
					return count++ < n && dst(unwrap(v));
				return true;
				});
			};
		return monad<unwrapped_t<T>, decltype(f)>{std::move(f)};
	}

	constexpr auto drop(std::signed_integral auto n) const {
		auto f = [=, fn = fn](auto dst) {
			if (n <= 0)
				return true;

			decltype(n) count = 0;
			return fn([&](in<T> v) {
				if(has_value(v))
					return count++ < n || dst(unwrap(v));
				return true;
				});
			};
		return monad<unwrapped_t<T>, decltype(f)>{std::move(f)};
	}

	template<range_like ...Rngs>
	constexpr auto concat(Rngs const&... rng) const
		requires (std::same_as<unwrapped_t<T>, unwrapped_t<typename Rngs::value_type>> && ...)
	{
		auto f = [fn = fn, ...fns = make_fn(rng)](auto dst) {
			return fn(dst) && (fns(dst) && ...);
			};
		return monad<T, decltype(f)>{std::move(f)};
	}

	template<typename OtherT, typename OtherFn>
		requires std::same_as<unwrapped_t<T>, unwrapped_t<OtherT>>
	constexpr auto concat(monad<OtherT, OtherFn> m) const {
		auto f = [fn = fn, m](auto dst) {
			return fn(dst) && m.fn(dst);
			};
		return monad<T, decltype(f)>{std::move(f)};
	}

	constexpr auto join() const requires range_like<unwrapped_t<T>> {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (has_value(v)) {
					using in_t_val = in<typename unwrapped_t<T>::value_type>;
					for (in_t_val p : unwrap(v)) {
						if (!dst(p))
							return false;
					}
				}
				return true;
				});
			};
		return monad<typename unwrapped_t<T>::value_type, decltype(f)>{std::move(f)};
	}

	template<typename P>
	constexpr auto join_with(P&& pattern) const {
		auto f = [=, fn = fn](auto dst) {
			auto send_to_dst = [&]<typename DstT = T>(in<DstT> l) {
				if constexpr (range_like<DstT>) {
					using in_dst_t = in<typename DstT::value_type>;
					for (in_dst_t p : l) {
						if(has_value(p))
							if (!dst(unwrap(p)))
								return false;
					}
					return true;
				}
				else {
					return dst(l);
				}
				};

			T last;

			bool first = true;
			bool const retval = fn([&](in<T> v) {
				if (first) {
					first = false;
					last = v;
					return true;
				}
				else {
					bool const cont = (send_to_dst(last) && send_to_dst.template operator() < P > (pattern));
					last = v;
					return cont;
				}
				});

			if (retval) {
				return send_to_dst(last);
			}
			return true;
			};
		if constexpr (range_like<unwrapped_t<T>>) {
			return monad<typename unwrapped_t<T>::value_type, decltype(f)>{std::move(f)};
		}
		else {
			return monad<unwrapped_t<T>, decltype(f)>{std::move(f)};
		}
	}

	template<int S>
	constexpr auto join_with(const char (&pattern)[S]) const {
		return join_with(std::string_view{ pattern });
	}

	constexpr auto split(auto delimiter) const {
		constexpr bool use_string_as_container = std::same_as<T, char>;
		using Container = std::conditional_t<use_string_as_container, std::basic_string<T>, std::vector<T>>;

		auto f = [=, fn = fn](auto dst) {
			Container part;

			bool const retval = fn([&](in<T> v) {
				if (has_value(v)) {
					in<unwrapped_t<T>> uv = unwrap(v);
					if (uv == delimiter) {
						if (!dst(part)) {
							return false;
						}
						part.clear();
					}
					else {
						add_to_container(part, uv);
					}
				}

				return true;
				});

			return retval && dst(part);
		};
		return monad<Container, decltype(f)>{std::move(f)};
	}

	template<int MaxSplitSize>
		requires (MaxSplitSize > 0)
	constexpr auto split_fast(auto delimiter) const {
		constexpr bool is_string_type = std::same_as<T, char>;
		using View = std::conditional_t<is_string_type, std::string_view, std::span<T>>;
		using Container = std::array<T, MaxSplitSize>;

		auto f = [=, fn = fn](auto dst) {
			Container part;
			std::size_t i = 0;

			bool const retval = fn([&](in<T> v) {
				if (has_value(v)) {
					in<unwrapped_t<T>> uv = unwrap(v);
					if (uv == delimiter) {
						if (!dst(View{ part.data(), i })) {
							return false;
						}
						i = 0;
					}
					else {
						if (i < part.size()) {
							part[i++] = uv;
						}
					}
				}

				return true;
				});

			return retval && dst(View{ part.data(), i });
			};
		return monad<View, decltype(f)>{std::move(f)};
	}

	// TODO use bloom filter
	//constexpr auto split(range_like auto delimiter) const {

	template<typename Cast>
		requires std::constructible_from<Cast, unwrapped_t<T>>
				|| std::constructible_from<Cast, std::from_range_t, unwrapped_t<T>>
	constexpr auto as() const {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (has_value(v)) {
					if constexpr (std::constructible_from<Cast, unwrapped_t<T>>) {
						return dst(Cast{ unwrap(v) });
					}
					else {
						return dst(Cast{ std::from_range, unwrap(v) });
					}
				}
				return true;
				});
			};
		return monad<Cast, decltype(f)>{std::move(f)};
	}

	template<typename ...Projs>
	constexpr auto as_tuple(Projs&& ...projs) const {
		using Tuple = std::tuple<std::invoke_result_t<Projs, unwrapped_t<T>>...>;

		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (has_value(v)) {
					return dst(std::tuple{ std::invoke(projs, unwrap(v))... });
				}
				return true;
				});
			};
		return monad<Tuple, decltype(f)>{std::move(f)};
	}

	template<typename Other>
	constexpr auto value_or(Other const& other) const requires optional_like<T> {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (!has_value(v))
					return dst(other);
				else
					return dst(unwrap(v));
				});
			};
		return monad<typename T::value_type, decltype(f)>{std::move(f)};
	}

	constexpr auto unexpected(auto err_handler) const requires expected_like<T> {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (!has_value(v))
					err_handler(v.error());
				else
					return dst(unwrap(v));
				return true;
				});
			};
		return monad<typename T::value_type, decltype(f)>{std::move(f)};
	}

	constexpr auto and_then(auto user_fn) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (has_value(v)) {
					in<unwrapped_t<T>> ub = unwrap(v);
					user_fn(ub);
					return dst(ub);
				}
				return true;
				});
			};
		return monad<T, decltype(f)>{std::move(f)};
	}

	constexpr auto unbox() const requires optional_like<T> {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (has_value(v)) {
					if (!dst(unwrap(v)))
						return false;
				}
				return true;
				});
			};
		return monad<unwrapped_t<T>, decltype(f)>{std::move(f)};
	}

	//
	// Terminal operations
	//

	template<typename UserFn>
	constexpr void then(UserFn&& user_fn) const {
		fn([&](in<T> v) {
			if (has_value(v))
				user_fn(unwrap(v));
			return true;
			});
	}

	template<typename I = std::int64_t>
	constexpr I sum(I init = 0) const {
		fn([&](in<T> v) {
			init += unbox_or<0>(v);
			return true;
			});
		return init;
	}

	constexpr std::int64_t count() const {
		std::int64_t c{ 0 };
		fn([&](in<T> v) {
			c += has_value(v);
			return true;
			});
		return c;
	}

	template<typename C>
		requires std::constructible_from<typename C::value_type, unwrapped_t<T>>
	constexpr auto to() const {
		C c;

		fn([&](in<T> v) {
			if (has_value(v))
				add_to_container(c, unwrap(v));
			return true;
			});

		return c;
	}

	template<template<class...> typename C>
	constexpr auto to() const {
		return to<C<unwrapped_t<T>>>();
	}

	template<template<class...> typename C, typename ...Projs>
		requires (sizeof...(Projs) > 0 && requires { C<std::remove_cvref_t<std::invoke_result_t<Projs, T>>...>{}; })
	constexpr auto to(Projs ...projs) const {
		C<std::remove_cvref_t<std::invoke_result_t<Projs, T>>...> c;

		fn([&](in<T> v) {
			if (has_value(v))
				add_to_container(c, std::tuple{ std::invoke(projs, unwrap(v))... });
			return true;
			});

		return c;
	}
};


export template<range_like T>
monad(T const& t) -> monad<typename T::value_type, decltype(make_fn(t))>;

export template<typename T>
	requires (!function_like<T>) && (!range_like<T>)
monad(T const& val)->monad<T, decltype(make_one(val))>;
