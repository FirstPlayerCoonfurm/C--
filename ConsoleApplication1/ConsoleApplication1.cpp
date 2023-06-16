#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <set>
#include <chrono>

using namespace std;

class Task {
public:
    Task() = default;
    Task(function<void()> m__func):m_func(m__func){ }
    void operator()() const { (m_func)(); }
    int getId() const { return m_id; }
private:
    function<void()> m_func;
    int m_id;
};

class ThreadPool {
public:
    ThreadPool(int n) {
        for (int i = 0; i < n; ++i) {
            m_threads.emplace_back([this]() { this->workerThread(); });
        }
    }

    ~ThreadPool() {
        if (!m_shutdown) {
            shutdown();
        }
    }

    int addTask(Task task) {
        unique_lock<mutex> lock(m_mutex);
        int id = ++m_lastId;
        m_tasks.emplace(std::make_pair(id, std::move(task)));
        m_condition.notify_one();
        return id;
    }

    void wait(int taskId) {
        unique_lock<mutex> lock(m_mutex);
        while (!isDone(taskId)) {
            m_doneCondition.wait(lock);
        }
    }

    void waitAll() {
        unique_lock<mutex> lock(m_mutex);
        while (!m_tasks.empty() || !m_workingTasks.empty()) {
            m_doneCondition.wait(lock);
        }
    }

    bool calculated(int taskId) {
        unique_lock<mutex> lock(m_mutex);
        return isDone(taskId);
    }

    void shutdown() {
        {
            unique_lock<mutex> lock(m_mutex);
            m_shutdown = true;
            m_condition.notify_all();
        }
        for (auto& thread : m_threads) {
            thread.join();
        }
    }

private:
    void workerThread() {
        while (true) {
            std::pair<int, Task> task{};//Так делать не обязательно ибо Task - объект! и я могу положить id задачи внутрь него
            {
                unique_lock<mutex> lock(m_mutex);
                m_condition.wait(lock, [this]() { return !m_tasks.empty() || m_shutdown; });
                if (m_shutdown) {
                    break;
                }
                task = std::move(m_tasks.front());
                m_tasks.pop();
                m_workingTasks.emplace(task.first);
            }

            task.second();
            
            {
                unique_lock<mutex> lock(m_mutex);
                std::cout << "Thread: " << std::this_thread::get_id() << " Task: " << task.first << endl;
                m_workingTasks.erase(task.first);
                m_doneTasks.emplace(task.first);
                m_doneCondition.notify_all();
                //delete &(task.second);
            }
        }
    }

    bool isDone(int taskId) const {
        return m_doneTasks.find(taskId) != m_doneTasks.end();
    }

    int m_lastId = 0;
    queue<std::pair<int, Task>> m_tasks;
    set<int> m_workingTasks;
    set<int> m_doneTasks;
    vector<thread> m_threads;
    mutex m_mutex;
    condition_variable m_condition;
    condition_variable m_doneCondition;
    bool m_shutdown = false;
};

void AddsT() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main() {
    ThreadPool firstThread(10);
    Task *task;
    for (int i = 0; i < 30; i++) {
        firstThread.addTask(*(new Task(AddsT)));
    }
    firstThread.waitAll();
}