This application aims to get precise waiting/scheduling at not time triggered machine with sacrificing/dedicating only one core to scheduling.

There are improvements to be made but this is basically a proof of concept.

While Windows.h Sleep() and std::this_thread::sleep_for() gives precision +-0.5ms per 1ms sleep, with this application it is much much lower. Currently +-0.01ms for every sleep >1ms. 

This is achieved with std::thread pause and resume.

This application is designed for game development purposes.