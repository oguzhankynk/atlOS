#include <drivers/serial.hpp>
#include <sched/scheduler.hpp>

void threadTaskA(Threading::Scheduler& scheduler)
{
    while (true) {
        SerialPort::writeSerial('A');
        SerialPort::writeSerial('\n');
        for (volatile int i = 0; i < 5000000; i++)
            ;
        scheduler.yield();
    }
}
void threadTaskB(Threading::Scheduler& scheduler)
{
    while (true) {
        SerialPort::writeSerial('B');
        SerialPort::writeSerial('\n');
        for (volatile int i = 0; i < 5000000; i++)
            ;
        scheduler.yield();
    }
}
