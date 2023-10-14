This application aims to get precise waiting at not time triggered machine with sacrificing/dedicating only one core to scheduling.

There are improvements to be made but this is basically a proof of concept.

While Windows.h Sleep() and std::this_thread::sleep_for() gives precision +-5ms, with this application it is much much lower. Sor far maximum 0.01% error rate has seen.

It makes a thread to sleep mininum of 1 microsecond, while Sleep() and std::this_thread::sleep_for() can 5-15ms. The inherited thread class does not wait busy, so it doesn't waste computing powers.(Scheduler wastes 100% of a single core, but this also can be improved with a little bit modification.) 

This is achieved with some abstraction but still using std::thread.

This application is designed for game development purposes.

Some Rough Measurements below;
Threads | Sleep | Max Deviation
--- | --- | --- 
1 | 1 microsecond | 732.3%
1 | 10 microsecond | 105.2%
1 | 100 microsecond | 10.90%
10 | 1 microsecond | 955.9%
10 | 10 microsecond | 168.62%
10 | 100 microsecond | 23.05%
1 | 1 millisecond | 0.59%
1 | 10 millisecond | 0.044%
1 | 100 millisecond | 0.0003%
10 | 1 millisecond | 3.2%
10 | 10 millisecond | 0.5%
10 | 100 millisecond | 0.003%
100 | 1 millisecond | 20.13%
100 | 10 millisecond | 0.20%
100 | 100 millisecond | 0.0032%
