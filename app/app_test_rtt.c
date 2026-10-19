#include "../include/os/rtt.h"
#include "../include/os/tick.h"
#include "../include/os/app.h"

volatile uint32_t g_reached_main;

void os_app_main(void){
    g_reached_main = 0xA5A5A5A5;
    
    for(;;){
        os_delay_ticks(os_ticks_from_ms(2000));
        os_rtt_write("Testing RTT!", 12);
    }

} 