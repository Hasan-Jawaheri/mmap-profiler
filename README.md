# mmap Profiler
Generic profiling tool that uses shared memory for logging

# Building
Run `make`.

# Usage
Link your code against `bin/libprofiler.a`, `librt` (`-lrt` LD flag), `libpthread` (`-lpthread` LD flag) and use the `inc/` directory. Then you can use the library as illustrated in `test_client.cpp` and `test_server.cpp`.
