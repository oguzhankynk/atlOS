#include <drivers/serial.hpp>
#include <sched/scheduler.hpp>

void threadTaskA() {
    while (true) {
        SerialPort::writeSerial('A');
        SerialPort::writeSerial('\n');
        for(volatile int i=0; i<5000000; i++);
        Scheduler::yield(); 
    }
}
void threadTaskB() {
    while (true) {
        SerialPort::writeSerial('B');
        SerialPort::writeSerial('\n');
        for(volatile int i=0; i<5000000; i++);
        Scheduler::yield();
    }
}
