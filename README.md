# YSL
💄 YAML Stream Logging, structured Google logging with YAML!

Integrated in yaml-cpp on this branch: https://github.com/MacroBull/yaml-cpp/blob/mymod/include/yaml-cpp/contrib/ysl.hpp

## Usage

Use macro `YSL` instead of `LOG`, you get a `YAML::Emitter`-like stream with Google logging output: 

```c++
#include "ysl.hpp"

#include <yaml-cpp/stlemitter.h>

YSL(INFO) << YSL::Frame("Hello Container") << YAML::FloatPrecision(3) << YAML::BeginMap;
YSL(INFO) << "hello" << std::map<int, float>{{1, 3.4f}, {2, 6.78f}, {3, 9.0f}};
YSL(INFO) << "PI" << YAML::Flow << std::vector<int>{3, 1, 4, 1, 5, 9, 2, 6};
YSL(INFO) << YAML::EndMap;
```

Output:

```
I0926 15:42:30.953778 12990 main.cpp:19] --- # --------------- Hello Container: 0 ------------ # ---
I0926 15:42:30.953805 12990 main.cpp:20] hello:
I0926 15:42:30.953812 12990 main.cpp:20]   1: 3.4
I0926 15:42:30.953830 12990 main.cpp:20]   2: 6.78
I0926 15:42:30.953848 12990 main.cpp:20]   3: 9
I0926 15:42:30.953855 12990 main.cpp:21] PI: [3, 1, 4, 1, 5, 9, 2, 6]
```
