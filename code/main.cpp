#include <iostream>
#include <exception>

#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

#define MAX_SIZE 10

class Global
{
	private:
		Global();
		~Global();
	public:
		static std::atomic<bool> isRunning;
		static std::queue<int> dataQueue;
		static std::mutex dataQueueMutex;
		static std::condition_variable dataQueueEmpty;
		static std::condition_variable dataQueueFull;
};

std::atomic<bool> Global::isRunning = true;
std::queue<int> Global::dataQueue;
std::mutex Global::dataQueueMutex;
std::condition_variable Global::dataQueueEmpty;
std::condition_variable Global::dataQueueFull;


void consumer() 
{
	auto threadID = std::this_thread::get_id();
	std::cout << "consumer thread id: " << threadID << std::endl;

	int data;
	while (Global::isRunning) 
	{
		while (Global::dataQueue.empty()) 
		{
			if (!Global::isRunning) 
				return;
			std::unique_lock<std::mutex> lock(Global::dataQueueMutex);
			Global::dataQueueEmpty.wait(lock);
		}
		{
			std::lock_guard<std::mutex> guard(Global::dataQueueMutex);
			data = Global::dataQueue.front();
			Global::dataQueue.pop();
		}
		Global::dataQueueFull.notify_one();
		std::cout << "data: " << data << std::endl;
	}
}

void producer() 
{
	auto threadID = std::this_thread::get_id();
	std::cout << "producer thread id: " << threadID << std::endl;
	int count = 1;
	while (Global::isRunning) 
	{
		while (Global::dataQueue.size() > MAX_SIZE) 
		{
			if (!Global::isRunning) 
				return;
			std::unique_lock<std::mutex> lock(Global::dataQueueMutex);
			Global::dataQueueFull.wait(lock);
		}
		{
			std::lock_guard<std::mutex> guard(Global::dataQueueMutex);
			Global::dataQueue.push(count);
		}
		Global::dataQueueEmpty.notify_one();
		++count;
	}
}

int main() 
{
	auto threadID = std::this_thread::get_id();
	std::cout << "main thread id: " << threadID << std::endl;
	try 
	{
		std::thread produce_thrd(producer);
		std::thread consume_thrd(consumer);
		std::this_thread::sleep_for(std::chrono::microseconds(20000));
		Global::isRunning = false;
		produce_thrd.join();
		consume_thrd.join();
	} 
	catch (const std::exception &e) 
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
