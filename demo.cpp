
#include <cmath>
#include <thread>

#include "ysl.hpp"

#include <yaml-cpp/stlemitter.h>

int main()
{
	FLAGS_logtostderr = true;
	FLAGS_minloglevel = 0;
	FLAGS_v           = 2;

	LOG(INFO) << "Hello world!";

	{
		YSL(INFO) << YSL::ThreadFrame("Hello YSL") << YSL::BeginMap;
		YSL(INFO) << "name"
				  << "Mark McGwire";
		YSL(INFO) << "hr" << 65;
		YSL(INFO) << "avg" << 0.278f;
		YSL(INFO) << YSL::EndMap;
	}
	{
		YSL(INFO) << YSL::ThreadFrame("Hello Container");
		YSL(INFO) << YSL::FloatPrecision(3);
		YSL(INFO) << YSL::BeginMap;
		YSL(INFO) << "hello" << std::map<int, float>{{1, 3.4f}, {2, 6.78f}, {3, 9.0f}};
		YSL(INFO) << "PI";
		YSL(INFO) << YSL::Flow << std::vector<int>{3, 1, 4, 1, 5, 9, 2, 6};
		YSL(INFO) << YSL::EndMap << YSL::EndDoc;
	}
	{
		YSL(INFO) << YSL::ThreadFrame("Hello Newline") << YSL::BeginMap;
		YSL(INFO) << YSL::Comment("This is a comment");
		YSL(INFO) << "glog newline" << -1;
		LOG(INFO) << std::endl;
		YSL(INFO) << "ysl newline" << true;
		YSL(INFO) << YSL::Newline;
		YSL(INFO) << "newline and literal" << YSL::Literal << "multi\nline\nliteral";
		YSL(INFO) << YSL::EndMap << YSL::Newline;
	}

	YSL_AT_LEVEL(2) << YSL::ThreadFrame("At level 2") << YSL::EndDoc;

	for (int loop = 0; loop < 10; ++loop)
	{
		YSL_IF(ERROR, loop & 1) << YSL::ThreadFrame("Loop " + std::to_string(loop));
		YSL_IF(ERROR, loop & 1) << YSL::BeginSeq;
		YSL_IF(WARNING, loop & 1) << YSL::Flow << std::vector<int>(loop, loop);
		YSL_IF(ERROR, loop & 1) << YSL::EndSeq;
		VYSL_IF(1, loop & 3) << std::vector<int>(loop, loop);
	}

	const int  n      = 4;
	const int  m      = 1000;
	const auto worker = [](int idx) {
		for (int loop = 0; loop < m; ++loop)
		{
			YSL(INFO) << YSL::ThreadFrame("Thread " + std::to_string(idx))
					  << YSL::FloatPrecision(3) << YSL::Flow << YSL::BeginMap;
			auto phase = idx * .1f + loop * .2f;
			YSL(INFO) << "cos" << std::cos(phase) << "sin" << std::sin(phase);
			YSL(INFO) << "log" << std::log(phase) << "exp" << std::exp(phase);
			YSL(INFO) << YSL::EndMap;

			// C++14
			// using namespace std::chrono_literals;
			// std::this_thread::sleep_for(10ms);
			std::this_thread::sleep_for(std::chrono::duration<float>(0.01f));
		}

		YSL(INFO) << YSL::EndDoc;
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

	return 0;
}
