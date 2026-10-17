#include <Interfaces/serial.hpp>
#include <Schduler/schduler.hpp>

void threadTaskA() {
    while (true) {
        SerialPort::write_serial('A');
        SerialPort::write_serial('\n');
        for(volatile int i=0; i<5000000; i++);
        Schedular::yield(); 
    }
}
void threadTaskB() {
    while (true) {
        SerialPort::write_serial('B');
        SerialPort::write_serial('\n');
        for(volatile int i=0; i<5000000; i++);
        Schedular::yield();
    }
}
