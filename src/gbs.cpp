import std;
import compiler;
import context;

import cmd_build;
import cmd_get_cl;
import cmd_enum_cl;
import cmd_clean;
import cmd_run;
import cmd_cl;

template<typename T>
requires ("test", !std::same_as<T, void>)
struct task {
	struct promise_type {
		T value;

		task get_return_object() {
			return task{ std::coroutine_handle<promise_type>::from_promise(*this) };
		}

		std::suspend_always initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }

		void return_value(T&& from) { value = std::forward<T>(from); }

		std::suspend_always yield_value(T&& from) { value = std::forward<T>(from); return {}; }

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

task<int> computeSquare(int x) {
	co_return x * x;
}

task<std::string> asyncMessage() {
	co_return "Hello from coroutine!";
}

void test() {
	int sq = computeSquare(5).get();
	task<std::string> messageTask = asyncMessage();

	std::string s = messageTask.get();
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
