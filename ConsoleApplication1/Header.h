#include <condition_variable>
#include <vector>
#include <mutex>
#include <thread>

std::vector<int> data;
std::condition_variable data_cond;
std::mutex m;