import std;
import compiler;
import context;

import cmd_build;
import cmd_get_cl;
import cmd_enum_cl;
import cmd_clean;
import cmd_run;
import cmd_cl;

struct simple_awaiter {
	bool await_ready() const noexcept { return false; }
	static void await_suspend(std::coroutine_handle<>) noexcept {}
	static void await_resume() noexcept {}
};

struct thread_awaitable
{
	std::jthread* p_out;
	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> h)
	{
		std::jthread& out = *p_out;
		if (out.joinable())
			throw std::runtime_error("Output jthread parameter not empty");
		out = std::jthread([h] { h.resume(); });
	}
	void await_resume() {}
};

template<typename T = void>
struct task {
	struct promise_type {
		T value;

		task get_return_object() { return task{ std::coroutine_handle<promise_type>::from_promise(*this) }; }
		std::suspend_always initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }
		void return_value(T&& from) { value = std::forward<T>(from); }
		//std::suspend_always yield_value(T&& from) { value = std::forward<T>(from); return {}; }
		void unhandled_exception() { std::terminate(); }
	};

	using handle_type = std::coroutine_handle<promise_type>;

	explicit task(handle_type h) : handle(h) {}
	~task() { if (handle) handle.destroy(); }

	T get() {
		handle.resume();
		return handle.promise().value;
	}

private:
	handle_type handle;
};

template<>
struct task<void> {
	struct promise_type {
		task get_return_object() { return task{ std::coroutine_handle<promise_type>::from_promise(*this) }; }
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_never final_suspend() noexcept { return {}; }
		void return_void() { }
		void unhandled_exception() { std::terminate(); }
	};

	using handle_type = std::coroutine_handle<promise_type>;

	explicit task(handle_type h) : handle(h) {}
	~task() { if (handle) handle.destroy(); }

private:
	handle_type handle;
};

task<int> computeSquare(int x) {
	co_return x * x;
}

task<std::string> asyncMessage() {
	co_return "Hello from coroutine!";
}

task<> resuming_on_new_thread(std::jthread& out)
{
	std::println("Coroutine started on thread: {}", std::this_thread::get_id());
	co_await thread_awaitable{ &out };
	std::println("Coroutine resumed on thread: ", std::this_thread::get_id());
}

void test() {
	int sq = computeSquare(5).get();
	task<std::string> messageTask = asyncMessage();
	std::println("sq: {}", sq);

	std::jthread thread;
	resuming_on_new_thread(thread);

	std::string s = messageTask.get();
	std::println("Message: {}", s);
}

int main(int argc, char const* argv[]) {
	test();

	std::println("Gorking build system v0.11");

	context ctx;

	if (argc == 1) {
		fill_compiler_collection(ctx);
		ctx.select_first_compiler();
		return !cmd_build(ctx, "release");
	}

	static std::unordered_map<std::string_view, bool(*)(context&, std::string_view)> const commands = {
		{"enum_cl", cmd_enum_cl},
		{"get_cl", cmd_get_cl},
		{"cl", cmd_cl},
		{"clean", cmd_clean},
		{"build", cmd_build},
		{"run", cmd_run},
	};

	auto const args = std::span<char const*>(argv, argc);
	for (std::string_view arg : args | std::views::drop(1)) {
		std::string_view const left = arg.substr(0, arg.find('='));
		if (!commands.contains(left)) {
			std::println("<gbs> Unknown command '{}', aborting\n", left);
			return 1;
		}

		arg.remove_prefix(left.size());
		arg.remove_prefix(!arg.empty() && arg.front() == '=');
		if (!commands.at(left)(ctx, arg)) {
			std::println("<gbs> aborting due to command failure.");
			return 1;
		}
	}

	return 0;
}
