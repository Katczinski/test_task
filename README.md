## About
This is my take on the interview test task.
The program is supposed to listen to all incoming UDP messages, append a specified prefix to each of them and redirect the result to the specified TCP server while maintaining constant connection (when possible) to the said server, reconnecting if needed, and logging all the events it deems important to the specified log file. Program should not block at any time.

## Build
The build targets are following:
- target&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(Default make target) The program itself
- test_udp    The test program to act as a UDP client to the specified server (main program)
- test_tcp    The test program to act as a TCP server for the main program to redirect messages
- all         Builds all of the above  
- debug       Builds all of the above in the debug mode

Debug mode is designed to redirect all the program output to stdout instead of the log file, while performing all the validity checks on the input parameters

### Example
``` shell
make all
```

## Usage
The main program requires 4 arguments, which are:
- UDP ip:port to listen to
- TCP ip:port to redirect to
- file name of the log to be created if it does not exist
- prefix to append to all incoming messages
``` shell
./target 127.0.0.1:6000 127.0.0.1:7000 log.txt +-+-
```
Any of the test programs:
- ip:port to start UDP client/TCP server on
``` shell
./test_udp 127.0.0.1:6000
./test_tcp 127.0.0.1:7000
```

![](https://github.com/Katczinski/test_task/blob/busy_wait/example.gif)

## Requirements
- All ip addresses MUST follow the format 'ddd.ddd.ddd.ddd:xxxxx'
- Prefix MUST be a 4-characters long string
- OS Linux
