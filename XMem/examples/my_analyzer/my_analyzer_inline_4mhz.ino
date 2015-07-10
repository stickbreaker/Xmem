/*
 *
 * SUMP Protocol Implementation for Arduino boards.
 *
 * Copyright (c) 2011,2012,2013,2014,2015 Andrew Gillham
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ANDREW GILLHAM ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANDREW GILLHAM BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */


/*
 * This function samples data using an unrolled loop for increased speed.
 * It also has rudimentary trigger support where it will just sit in
 * a busy loop waiting for the trigger conditions to occur.
 *
 * This loop is not clocked to the sample rate in any way, it just
 * reads the port as fast as possible waiting for a trigger match.
 * Multiple channels can have triggers enabled and can have different
 * trigger values.  All conditions must match to trigger.
 *
 * After the trigger fires (if it is enabled) the pins are sampled
 * at the appropriate rate.
 *
 */
 

void captureInline4mhz() {

  /*
   * basic trigger, wait until all trigger conditions are met on port.
   * this needs further testing, but basic tests work as expected.
   */
  if (trigger) {
    while ((trigger_values ^ CHANPIN) & trigger);
  }

  /*
   * disable interrupts during capture to maintain precision.
   * we cannot afford any timing interference so we absolutely
   * cannot have any interrupts firing.
   */
  cli();

  /*
   * toggle pin a few times to activate trigger for debugging.
   * this is used during development to measure the sample intervals.
   * it is best to just leave the toggling in place so we don't alter
   * any timing unexpectedly.
   */
  DEBUG_ENABLE;
  DEBUG_ON;
  delayMicroseconds(1);
  DEBUG_OFF;
  delayMicroseconds(1);
  DEBUG_ON;
  delayMicroseconds(1);
  DEBUG_OFF;
  delayMicroseconds(1);

  DEBUG_ON; /* debug timing measurement */

  /*
   * Unroll loop to maximize capture speed.
   * Pad with 1 NOP (1 cycle) to make this run at 4MHz.
   *
   *
   */
   uint8_t * savelogicdata = logicdata;
#undef INLINE_NOP
#define INLINE_NOP		__asm__("nop\n\t""rjmp 1f\n\t""1:\n\t""rjmp 2f\n\t""2:\n\t");

   while(highByte((uint16_t)savelogicdata)>0){
	   *savelogicdata++=CHANPIN;
       
   }
   
  DEBUG_OFF; /* debug timing measurement */

  /* re-enable interrupts now that we're done sampling. */
  sei();

  /*
   * dump the samples back to the SUMP client.  nothing special
   * is done for any triggers, this is effectively the 0/100 buffer split.
   */
  if(!inDebug) sendCapture();
}





