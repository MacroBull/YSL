
#include <cmath>
#include <map>
#include <thread>
#include <vector>

#include "stl_emitter.hpp"
#include "ysl.hpp"

int main()
{
	// setup glog
	FLAGS_logtostderr = true;
	FLAGS_minloglevel = 0;
	FLAGS_v           = 2;

	// setup YSL for current thread: use 4 sapces as indent
	YSL::StreamLogger::set_thread_format(YSL::LoggerFormat::Indent, 4);

	// plain glog LOG
	LOG(INFO) << "This is a plain glog record";

	// simple key-value scope
	// YSL(INFO) << key << value;
	{
		YSL_FSCOPE(INFO, "A YSL Frame");
		YSL(INFO) << "name"          // key
				  << "Mark McGwire"; // value
		YSL(INFO) << "hr" << 65;
		YSL(INFO) << "avg" << 0.278f;
	}

	// stl and flow emission, explicit end-of-document
	{
		// manually start a frame
		YSL(INFO) << YSL::ThreadFrame("Some Containers");
		// setup local precision
		YSL(INFO) << YSL::FloatPrecision(3);
		// start a mapping
		YSL(INFO) << YSL::BeginMap;
		YSL(INFO) << "hello" << std::map<int, float>{{1, 3.4f}, {2, 6.78f}, {3, 9.0f}};
		YSL(INFO) << "PI";
		YSL(INFO) << YSL::Flow << std::vector<int>{3, 1, 4, 1, 5, 9, 2, 6};
		// end the mapping and the frame(explicitly)
		YSL(INFO) << YSL::EndMap << YSL::EndDoc;
	}

	// comment, literal and implicit/explicit new line
	{
		YSL_FSCOPE(INFO, "About Comment, Literal And Newline");
		YSL(INFO) << YSL::Comment("This is a comment");
		YSL(INFO) << "glog newline" << nullptr;
		LOG(INFO) << std::endl;
		YSL(INFO) << "ysl newline" << true;
		YSL(INFO) << YSL::Newline;
		YSL(INFO) << "newline and literal" << YSL::Literal << "multi\nline\nliteral";
		YSL(INFO) << YSL::EndMap << YSL::Newline;
	}

	// LOG_AT_LEVEL like
	YSL_AT_LEVEL(2) << YSL::ThreadFrame("At level 2") << YSL::EndDoc;

	// logging level and LOG_IF like
	for (int loop = 0; loop < 10; ++loop)
	{
		YSL_IF(ERROR, loop & 1) << YSL::ThreadFrame("Loop " + std::to_string(loop));
		YSL_IF(ERROR, loop & 1) << YSL::BeginSeq;
		YSL_IF(WARNING, loop & 1) << YSL::Flow << std::vector<int>(loop, loop);
		YSL_IF(ERROR, loop & 1) << YSL::EndSeq;
		VYSL_IF(1, loop & 3) << std::vector<int>(loop, loop);
	}

	// threaded logging
	const int  n      = 4;
	const int  m      = 1000;
	const auto worker = [](int idx) {
		// use quoted string style for better performance
		YSL::StreamLogger::set_thread_format(YSL::DoubleQuoted); // better performance
		YSL::StreamLogger::set_thread_format(YSL::LoggerFormat::FloatPrecision, 3);

		// interval logging
		for (int loop = 0; loop < m; ++loop)
		{
			YSL(INFO) << YSL::ThreadFrame("Thread " + std::to_string(idx)) << YSL::Flow
					  << YSL::BeginMap;
			auto phase = idx * .1f + loop * .2f;
			YSL(INFO) << "cos" << std::cos(phase) << "sin" << std::sin(phase);
			YSL(INFO) << "log" << std::log(phase) << "exp" << std::exp(phase);
			YSL(INFO) << YSL::EndMap;

			// C++14
			// using namespace std::chrono_literals;
			// std::this_thread::sleep_for(10ms);
			std::this_thread::sleep_for(std::chrono::milliseconds{10});
		}

		YSL(INFO) << YSL::EndDoc; // explicit
	};
	std::vector<std::thread> threads;
	threads.reserve(n);
	for (int idx = 0; idx < n; ++idx)
	{
		threads.emplace_back(std::thread{worker, idx});
	}
	for (auto& thread : threads)
	{
		thread.join();
	}

	LOG(INFO); // HINT: extra flush
	return 0;
}
