export module cmd_version;
import std;
import context;

export bool cmd_version(context& /*ctx*/, std::string_view /*args*/){
	std::println("<gbs> Gorking build system v0.17.0 - Now with unittests!");
	return true;
}