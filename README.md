# YSL
💄 YAML Stream Logging, structured Google logging with YAML!

Integrated in yaml-cpp on this branch: https://github.com/MacroBull/yaml-cpp/blob/mymod/include/yaml-cpp/contrib/ysl.hpp

## Usage

Use macro `YSL` instead of `LOG`, you get a `YAML::Emitter`-like stream with Google logging output: 

```c++
#include "ysl.hpp"

#include <yaml-cpp/stlemitter.h>

YSL(INFO) << YSL::ThreadFrame("Hello Container") << YAML::FloatPrecision(3) << YAML::BeginMap;
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

## Demo

Try `sh demo.sh`

## TODO

- [ ] more documentation
- [x] cpp: inline version
- [x] cpp: abstract emitter overloads, especially tagged literals -> extraemitter.h
- [ ] cpp: add Reconstructable< NullableStreamLogger >
- [x] cpp: add Sequential
- [x] cpp: implement Scope{Sequential enter, Sequential exit}
- [x] cpp: add SCOPE(plain), FSCOPE(frame + scope), MSCOPE(key + scope), CSCOPE(key + flow + scope)
- [x] cpp: add IFSCOPE(frame + id + scope), IMSCOPE(key + id + scope), ICSCOPE(key + id + flow + scope) overloads
- [ ] python: python < 3.6 support
- [ ] python: implement ysl.py with yaml + backends(logging)
- [ ] python: inherited yaml.XXConstructor, XXLoader
- [ ] python: implement protobuf constructor
- [ ] python: add FrameParser.fastforward_to
