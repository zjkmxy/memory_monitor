# Memory Monitor for ndnSim

This is a memory analyze tool for ndnSim. It can also be used out of ndnSim.
It monitors memory allocation and deallocation by overriding operator new / delete, and records all memory blocks allocated but not freed.
It can start and stop at any time, controlled by code. And outputs the call stack when the memory was allocated into a text file.

## Usage
Instructions below is for who compile ns-3 and ndnSim from source code with your scenario. Memory operations happened in NFD, ndnSim and scenario will be recorded.
1. Put memory_monitor.hpp and memory_monitor.cpp into ```ns3/src/ndnSIM/NFD/core```
1. Include ```#include "NFD/core/memory_monitor.hpp"``` in your scenario.
1. Use ```monitor::Monitor::getInstance()``` to get the instance of monitor.
   * Call ```start()``` to start recording. A record will be inserted into a list whenever a memory block is allocated. When it gets freed, the record will be deleted.
   * Call ```stop()``` to stop recording allocation. It will continue recording free.
   * Call ```stopAll()``` to stop recording allocation and free. The record list will not change any more.
   * Call ```getRecords()``` to get a copy of record list at any time.
   * Call ```writeLogFile(<file>)``` to create a log file.
1. Use ```convert.py``` to translate the log file from ```address + function name``` into ```file name + line number```.

If you write a seperate scenario, just put two files into ```extensions``` to get them linked with your scenario.

## Log file format
For every block:
```
>>TIME: time stamp
>>SIZE: size of the block
>>DEPT: depth of call stack
>>ADDR: address of call stack
.....
<<
```

## Examples
There is an example of using monitor to check memory leaks of ConsumerCbr.
You can just copy all in ```examples``` folder into ```ndnSIM/examples``` and run it.
It just a copy of ```ndn-simple``` and ```ndn-consumer-cbr``` with little modification. Read the code for more.

## Problems with MacOS
The code can not work well with clang's c++ library sometimes, so please compile it with gcc like:
```
./waf configure --enable-examples ----check-c-compiler=gcc --check-cxx-compiler=g++
```
You may need to recompile boost library with gcc before that.
```convert.py``` can not be used in MacOS for different format of ```backtrace_symbols_fd```.

## Remark
It's not well tested now. Please let me know if any bug found.

