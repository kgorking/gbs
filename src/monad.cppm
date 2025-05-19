export module monad;
import std;


template<typename T>
concept mutable_monad = !std::is_const_v<std::remove_reference_t<T>>;

export template<typename V>
class monad : public std::ranges::view_interface<monad<V>> {
	V view_;
	using Base = std::remove_reference_t<decltype(*view_.begin())>;

public:
	monad() = delete;

	// NOTE: this makes a copy of the initializer list
	template<typename T>
	constexpr monad(std::initializer_list<T> il) : view_(il) {}

	template<std::ranges::viewable_range R>
	explicit constexpr monad(R&& cont) noexcept : view_(std::forward<R>(cont)) {}


	constexpr auto begin() { return view_.begin(); }
	constexpr auto end()   { return view_.end(); }

	constexpr auto as_rvalue() {
		return ::monad(
			std::ranges::views::as_rvalue(std::move(view_))
		);
	}

	template<typename Fn, mutable_monad Self>
	constexpr auto filter(this Self&& self, Fn&& fn) {
		return ::monad(
			std::ranges::views::filter(std::move(std::forward<Self>(self).view_), std::forward<Fn>(fn))
		);
	}

	template<typename Fn>
	constexpr auto transform(Fn&& fn) {
		return ::monad(
			std::ranges::views::transform(std::move(view_), std::forward<Fn>(fn))
		);
	}

	constexpr auto take(std::integral auto i) {
		return ::monad(
			std::ranges::views::take(std::move(view_), i)
		);
	}

	template<typename Fn>
	constexpr auto take_while(Fn&& fn) {
		return ::monad(
			std::ranges::views::take_while(std::move(view_), std::forward<Fn>(fn))
		);
	}

	constexpr auto drop(std::integral auto i) {
		return ::monad(
			std::ranges::views::drop(std::move(view_), i)
		);
	}

	template<typename Fn>
	constexpr auto drop_while(Fn&& fn) {
		return ::monad(
			std::ranges::views::drop_while(std::move(view_), std::forward<Fn>(fn))
		);
	}

	constexpr auto join() {
		static_assert(std::ranges::input_range<Base>, "'join' must be a view of ranges");
		return ::monad(
			// Use a 'views::join' to be able to do a double join
			std::ranges::views::join(std::move(view_))
		);
	}

	constexpr auto join_with(auto&& pattern) {
		return ::monad(
			std::ranges::views::join_with(std::move(view_), std::move(pattern))
		);
	}

	constexpr auto lazy_split(auto&& pattern) {
		return ::monad(
			std::ranges::views::lazy_split(std::move(view_), std::move(pattern))
		);
	}

	constexpr auto split(auto&& pattern) {
		return ::monad(
			std::ranges::views::split(std::move(view_), std::move(pattern))
		);
	}

#ifdef __cpp_lib_ranges_concat
	constexpr auto concat(auto&& ...stuff) {
		return ::monad(
			std::ranges::views::concat(view_, stuff...)
		);
	}
#endif

	constexpr auto reverse() {
		return ::monad(
			std::ranges::views::reverse(std::move(view_))
		);
	}

	constexpr auto as_const() {
		return ::monad(
			std::ranges::views::as_const(std::move(view_))
		);
	}

	template<int I>
		requires (I >= 0 && I < std::tuple_size_v<Base>)
	constexpr auto elements() {
		return ::monad(
			std::ranges::views::elements<I>(std::move(view_))
		);
	}

	constexpr auto keys() 
		requires (std::tuple_size_v<Base> >= 1) {
		return ::monad(
			std::ranges::views::keys(std::move(view_))
		);
	}

	constexpr auto values() 
		requires (std::tuple_size_v<Base> >= 2) {
		return ::monad(
			std::ranges::views::values(std::move(view_))
		);
	}

	constexpr auto enumerate() {
		return ::monad(
			std::ranges::views::enumerate(std::move(view_))
		);
	}

	template<std::ranges::viewable_range... Rs >
	constexpr auto zip(Rs&& ...rs) {
		return ::monad(
			std::ranges::views::zip(std::move(view_), std::forward<Rs>(rs)...)
		);
	}

	template<typename F, std::ranges::viewable_range... Rs>
	constexpr auto zip_transform(F &&f, Rs&& ...rs) {
		return ::monad(
			std::ranges::views::zip_transform(std::forward<F>(f), std::move(view_), std::forward<Rs>(rs)...)
		);
	}

	template<int I>
		requires (I > 0)
	constexpr auto adjacent() {
		return ::monad(
			std::ranges::views::adjacent<I>(std::move(view_))
		);
	}

	template<int I, typename F>
		requires (I > 0)
	constexpr auto adjacent_transform(F&& f) {
		return ::monad(
			std::ranges::views::adjacent_transform<I>(std::move(view_), std::forward<F>(f))
		);
	}

	constexpr auto chunk(int n) {
		return ::monad(
			std::ranges::views::chunk(std::move(view_), n)
		);
	}

	constexpr auto slide(int n) {
		return ::monad(
			std::ranges::views::slide(std::move(view_), n)
		);
	}

	template<typename Pred>
	constexpr auto chunk_by(Pred&& pred) {
		return ::monad(
			std::ranges::views::chunk_by(std::move(view_), std::forward<Pred>(pred))
		);
	}

	constexpr auto stride(int n) {
		return ::monad(
			std::ranges::views::stride(std::move(view_), n)
		);
	}

	template<std::ranges::viewable_range... Rs >
	constexpr auto cartesian_product(Rs&& ...rs) {
		return ::monad(
			std::ranges::views::cartesian_product(std::move(view_), std::forward<Rs>(rs)...)
		);
	}

#ifdef __cpp_lib_ranges_cache_latest
	constexpr auto cache_latest() {
		return ::monad(
			std::ranges::views::cache_latest(std::move(view_))
		);
	}
#endif

#ifdef __cpp_lib_ranges_to_input
	constexpr auto to_input() {
		return ::monad(
			std::ranges::views::to_input(std::move(view_))
		);
	}
#endif

	//
	// Wrap some ranges stuff 
	//

	constexpr auto distance() {
		return std::ranges::distance(view_);
	}

	template<typename T>
	constexpr bool equal(std::initializer_list<T> other) {
		return std::ranges::equal(view_, other);
	}

	constexpr bool equal(std::ranges::input_range auto&& other) {
		return std::ranges::equal(view_, std::forward<decltype(other)>(other));
	}

	template<template <typename...> typename To>
	constexpr auto to() const {
		return std::ranges::to<To>(view_);
	}

	template<typename To>
	constexpr auto to() const {
		return std::ranges::to<To>(view_);
	}
};

template<typename T>
monad(std::initializer_list<T>) -> monad<std::ranges::views::all_t<std::vector<T>>>;

template<std::ranges::viewable_range R>
monad(R&&) -> monad<std::ranges::views::all_t<R>>;
