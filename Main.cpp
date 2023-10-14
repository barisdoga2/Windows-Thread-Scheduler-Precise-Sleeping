#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <Windows.h>
#include <conio.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <stdint.h>
#include <climits> 
#include <condition_variable>
#include <functional>
#include <random>
#include <limits>

typedef std::chrono::steady_clock::time_point TimePoint;

static TimePoint GetTime()
{
	return std::chrono::steady_clock::now();
}

class TaskManager {
	friend class Scheduler;
private:
	bool paused = false;
	bool running = false;

	std::mutex mutex;
	std::condition_variable cv;

	std::unique_ptr<std::thread> job;

protected:
	TaskManager()
	{

	}

	~TaskManager()
	{
		this->running = false;
		this->job.get()->join();
	}

	void Start(const std::function<void(void)>& callback)
	{
		if (this->running)
			return;

		this->running = true;
		this->job = std::make_unique<std::thread>(std::bind(&TaskManager::threadFunction, this, callback));
	}

private:
	void Pause()
	{
		if (this->paused)
			return;

		std::lock_guard<std::mutex> lock(this->mutex);
		this->paused = true;
	}

	void Resume()
	{
		if (!this->paused)
			return;

		std::lock_guard<std::mutex> lock(this->mutex);
		this->paused = false;
		this->cv.notify_one();
	}

	void threadFunction(const std::function<void(void)>& callback)
	{
		while (running)
		{
			std::unique_lock<std::mutex> lock(this->mutex);
			cv.wait(lock, [this] { return !this->paused; });
			lock.unlock();

			callback();
		}
	}

};

class Scheduler {
private:
	bool schedulerRunning = true;
	std::thread schedulerThread;
	std::map<TaskManager*, TimePoint> sleepList;
	std::vector<TaskManager*> taskManagers;
	std::mutex sleepListMutex;

public:
	Scheduler()
	{
		this->schedulerRunning = true;
		this->schedulerThread = std::thread(&Scheduler::SchedulerMain, this);
	}

	~Scheduler()
	{
		this->schedulerRunning = false;
		this->schedulerThread.join();
		sleepListMutex.lock();
		for (auto it = sleepList.begin(); it != sleepList.end(); it++)
		{
			it->first->running = false;
			it->first->Resume();
		}
		sleepListMutex.unlock();
		for (TaskManager* taskManager : taskManagers)
			delete taskManager;
	}

	void NewTaskManager(TaskManager* taskManager)
	{
		if(std::find(taskManagers.begin(), taskManagers.end(), taskManager) == taskManagers.end())
			taskManagers.push_back(taskManager);
	}

	void Sleep(TaskManager* taskManager, long long nanoseconds)
	{
		taskManager->Pause();
		TimePoint endTime = GetTime() + (std::chrono::steady_clock::duration{ nanoseconds });

		sleepListMutex.lock();
		sleepList.insert({ taskManager,  endTime });
		sleepListMutex.unlock();
	}

private:
	void SchedulerMain()
	{
		while (schedulerRunning)
		{
			sleepListMutex.lock();
			for (auto it = sleepList.begin(); it != sleepList.end(); )
			{
				TimePoint currentTime = GetTime();
				if (it->second <= currentTime)
				{
					it->first->Resume();
					it = sleepList.erase(it);
				}
				else
				{
					it++;
				}
			}
			sleepListMutex.unlock();
		}
	}
};

long long minSleepNanoseconds;
long long maxSleepNanoseconds;

long long getRandomSleepNanoseconds()
{
	static std::random_device rd;
	static std::mt19937_64 eng(rd());

	static std::uniform_int_distribution<long long> distr;

	if (minSleepNanoseconds == maxSleepNanoseconds)
		return minSleepNanoseconds;
	else if (maxSleepNanoseconds < minSleepNanoseconds)
		return minSleepNanoseconds;

	long long gen = distr(eng);

	gen %= (maxSleepNanoseconds - minSleepNanoseconds);
	gen += minSleepNanoseconds;

	return gen;
}

class ThreadClass : public TaskManager {
private:
	int id = 0;
	long long targetSleepNanoseconds;
	float targetSleepMs;
	float maxErrorRate = 0;
	Scheduler* scheduler;
	TimePoint sleepStart;
	bool sleeping = false;

public:
	ThreadClass(Scheduler* scheduler) : TaskManager()
	{
		static int idCtr = 1;
		this->id = idCtr++;
		this->scheduler = scheduler;
		this->scheduler->NewTaskManager(this);
		this->Start(std::bind(&ThreadClass::ThreadFunction, this));

		this->targetSleepNanoseconds = getRandomSleepNanoseconds();
		this->targetSleepMs = targetSleepNanoseconds / ((float)(1000ll * 1000ll));
	}

private:
	void ThreadFunction()
	{
		if (sleeping)
		{
			float sleptMs = ((GetTime() - sleepStart).count() / ((float)(1000ll * 1000ll)));
			float errorRate = (sleptMs - targetSleepMs) / targetSleepMs;
			if (errorRate > maxErrorRate) maxErrorRate = errorRate;
			std::cout << "Thread(" << id << ") Slept: " << sleptMs << " ms.\t Targeted: " << targetSleepMs << " ms.\t Error Rate: " << errorRate << "%.\t Max Error Rate: " << maxErrorRate << "%." << std::endl;
		}

		// Do Work
		// ...
		// ...
		// ...

		sleeping = true;
		sleepStart = GetTime();
		scheduler->Sleep(this, targetSleepNanoseconds);
		// Do not do anyting after sleep call
	}

};

int main(int argc, char* argv[])
{
	unsigned int numThreads;
	if (argc == 1)
	{
		std::cout << "Usage: test.exe [numThreads(unsigned int)] [minSleepMilliseconds(float)] [maxSleepMilliseconds(float)]" << std::endl;
		std::cout << "Running with default parameters." << std::endl;
		numThreads = 10;
		minSleepNanoseconds = 1ll * 1000ll * 1000ll; // 1ms
		maxSleepNanoseconds = 100ll * 1000ll * 1000ll; // 100ms
	}
	else if (argc != 4)
	{
		std::cout << "Usage: test.exe [numThreads(unsigned int)] [minSleepMilliseconds(float)] [maxSleepMilliseconds(float)]" << std::endl;
		return -1;
	}
	else
	{
		numThreads = atoi(argv[1]);
		minSleepNanoseconds = (long long)(atof(argv[2]) * 1000ll * 1000ll);
		maxSleepNanoseconds = (long long)(atof(argv[3]) * 1000ll * 1000ll);
	}

	std::cout << "Running " << numThreads << " threads! MinSleepNanoseconds: " << minSleepNanoseconds << ", MaxSleepNanoseconds: " << maxSleepNanoseconds << std::endl;
	for (int i = 0; i < 3; i++)
	{
		Sleep(1000);
		std::cout << "Starting in " << (3 - i) << " seconds..." << std::endl;
	}

	Scheduler* scheduler = new Scheduler();

	for (unsigned int i = 0; i < numThreads; i++)
		new ThreadClass(scheduler); // No need to delete after program ends, deleting Scheduler will clean them all

	while (true)
	{
		if (_getch())
			break;
	}

	delete scheduler;
}