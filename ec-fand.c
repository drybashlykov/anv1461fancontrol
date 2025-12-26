#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/io.h>
#include <stdlib.h>
#include <signal.h>

#define EC_SC   0x66
#define EC_DATA 0x62

#define EC_OBF 0x01
#define EC_IBF 0x02

#define EC_READ  0x80
#define EC_WRITE 0x81

#define FAN_MODE_ADDR 0x64
#define FAN_DUTY_ADDR 0x49
#define RPM_H_ADDR    0x5C
#define RPM_L_ADDR    0x5D

static volatile int running = 1;

void sigint(int _) { running = 0; }

static inline void wait_ibf_clear(void) {
    while (inb(EC_SC) & EC_IBF);
}

static inline void wait_obf_set(void) {
    while (!(inb(EC_SC) & EC_OBF));
}

uint8_t ec_read(uint8_t addr) {
    wait_ibf_clear();
    outb(EC_READ, EC_SC);

    wait_ibf_clear();
    outb(addr, EC_DATA);

    wait_obf_set();
    return inb(EC_DATA);
}

void ec_write(uint8_t addr, uint8_t val) {
    wait_ibf_clear();
    outb(EC_WRITE, EC_SC);

    wait_ibf_clear();
    outb(addr, EC_DATA);

    wait_ibf_clear();
    outb(val, EC_DATA);
}

int read_rpm() {
    int hi = ec_read(RPM_H_ADDR);
    int lo = ec_read(RPM_L_ADDR);
    return (hi << 8) | lo;
}

int read_temp() {
    FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) return 0;
    int t;
    fscanf(f, "%d", &t);
    fclose(f);
    return t / 1000;
}

uint8_t fan_curve(int temp) {
    if (temp < 40) return 0;
    if (temp < 50) return 30;
    if (temp < 60) return 50;
    if (temp < 70) return 70;
    if (temp < 75) return 90;
    return 100;
}

int main() {
    if (ioperm(EC_SC, 1, 1) || ioperm(EC_DATA, 1, 1)) {
        perror("ioperm");
        return 1;
    }

    signal(SIGINT, sigint);
    signal(SIGTERM, sigint);

    while (running) {
        // force manual mode
        ec_write(FAN_MODE_ADDR, 0xD1);

        int temp = read_temp();
        uint8_t duty = fan_curve(temp);

        ec_write(FAN_DUTY_ADDR, duty);

        int rpm = read_rpm();
        printf("temp=%dC duty=%d rpm=%d\n", temp, duty, rpm);

        sleep(3);
    }

    return 0;
}
