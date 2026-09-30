/* AVR startup code for Arduino (ATmega328P) */
.section .vectors, "ax"
.global __vectors
__vectors:
    rjmp    reset           /* Reset vector */
    reti                    /* INT0 */
    reti                    /* INT1 */
    reti                    /* PCINT0 */
    reti                    /* PCINT1 */
    reti                    /* PCINT2 */
    reti                    /* WDT */
    reti                    /* TIMER2 COMPA */
    reti                    /* TIMER2 COMPB */
    reti                    /* TIMER2 OVF */
    reti                    /* TIMER1 CAPT */
    reti                    /* TIMER1 COMPA */
    reti                    /* TIMER1 COMPB */
    reti                    /* TIMER1 OVF */
    reti                    /* TIMER0 COMPA */
    reti                    /* TIMER0 COMPB */
    reti                    /* TIMER0 OVF */
    reti                    /* SPI STC */
    reti                    /* USART RX */
    reti                    /* USART UDRE */
    reti                    /* USART TX */
    reti                    /* ADC */
    reti                    /* EE READY */
    reti                    /* ANALOG COMP */
    reti                    /* TWI */
    reti                    /* SPM READY */

.section .text
.global reset
reset:
    /* Initialize stack pointer */
    ldi r28, lo8(RAMEND)
    ldi r29, hi8(RAMEND)
    out __SP_L__, r28
    out __SP_H__, r29

    /* Clear .bss section */
    ldi r24, lo8(__bss_start)
    ldi r25, hi8(__bss_start)
    ldi r26, lo8(__bss_end)
    ldi r27, hi8(__bss_end)
    rcall __do_clear_bss

    /* Copy .data from flash to RAM */
    ldi r24, lo8(__data_start)
    ldi r25, hi8(__data_start)
    ldi r26, lo8(__data_end)
    ldi r27, hi8(__data_end)
    ldi r30, lo8(__data_load_start)
    ldi r31, hi8(__data_load_start)
    rcall __do_copy_data

    /* Call main */
    rcall main

    /* Infinite loop after main returns */
1:  rjmp 1b

__do_clear_bss:
    cp  r24, r26
    cpc r25, r27
    breq 1f
    st  Z+, __zero_reg__
    rjmp  __do_clear_bss
1:  ret

__do_copy_data:
    cp  r24, r26
    cpc r25, r27
    breq  1f
    lpm r0, Z+
    st  Y+, r0
    rjmp  __do_copy_data
1:  ret
