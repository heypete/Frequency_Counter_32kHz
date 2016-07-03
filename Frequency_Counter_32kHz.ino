/* Initialize variables. Interrupt-accessible variables must be global and volatile. */
volatile unsigned int timerOverflows = 0;
volatile unsigned int numSeconds = 0;
volatile unsigned long ticks = 0;
unsigned int maxSeconds = 0;
unsigned int printDifference = 0;
unsigned int printNumSeconds = 0;
unsigned int printTimerOverflows = 0;
unsigned long printTicks = 0 - 1;
unsigned int oldNumSeconds = 65536;
unsigned long oldTicks = 0;
boolean firstRun = 1;

void startstop() {
  /* This is called by the PPS interrupt. It needs to be fast. */
  ticks = TCNT1; // Read TIMER1 and save its current value to a global variable.
  numSeconds++; // Increment the number of seconds as marked by the PPS signal.

}

/* TIMER1 is a 16-bit counter. Each tick of the 32kHz line increments the counter by one.
 * This interrupt fires when the timer overflows and rolls over back to zero, incrementing
 * the timerOverflows variable by one. This is also very fast.
*/
ISR(TIMER1_OVF_vect) {
  timerOverflows++;
}

void timer1_init() {
  /* The Arduino core enables the waveform generator, which we don't need or want since it conflicts with using TIMER1 for counting,
   * so we must disable the waveform generator.
   */
  TCCR1A &= ~((1 << WGM12) | (1 << WGM11) | (1 << WGM10)); // Turn off the waveform generator.

  /* Setting CS12 and CS11 to 1 sets TIMER1 to use an external clock source and trigger on the falling edge.
   * Why the falling edge? With a 10k pull-up resistor on the DS3231's 32kHz pin, I found the falling edge to be much sharper.
   * Your mileage may vary.
   */
  TCCR1B |= ((1 << CS12) | (1 << CS11)); // Trigger on falling edge.
  // TCCR1B |= ((1 << CS12) | (1 << CS11) | (1 << CS10)); // Trigger on rising edge.

  /* Initialize TIMER1's counter. */
  TCNT1 = 0;

  /* Initialize the TIMER1 overflow interrupt. */
  TIMSK1 |= (1 << TOIE1);

  // Initalize the TIMER1 overflow counter.
  timerOverflows = 0;

  /* Enable pulse-per-second interrupt. */
  attachInterrupt(digitalPinToInterrupt(2), startstop, RISING);

  /* Enable global interrupts. */
  sei();
}

void resetCounters() {

  /* To prevent mis-counting, we need to wait for the first full second, then reset the counters.  */
  Serial.println("Initializing counters...");
  numSeconds = 0;
  while (numSeconds < 2) {
  }
  /* Resetting the counters must be atomic, so we disable interrupts first, reset the counters, then re-enable interrupts.*/
  cli(); // Disable interrupts.
  firstRun = 0; // We only need to run this once. After it's run once, turn it off.
  numSeconds = 0; // Reset seconds counter.
  timerOverflows = 0; // Reset the number of times TIMER1 has overflowed.
  ticks = 0; // Reset the number of 32kHz ticks.
  printTicks = 0; // Reset the tick counter we use to print results over serial.
  TCNT1 = 0; // Reset TIMER1.
  sei(); // Re-enable interrupts.

}

void printResults() {
  /* TIMER1 overflows every 65536 ticks. Each time it does it fires an interrupt. We multiply the number of times the timer has overflowed
  by 65536 ticks, then add the current value of the timer. This yields the total number of ticks since the timer was reset. */
  printTicks += timerOverflows * 65536;

  /* Print the results. */
  Serial.print(printNumSeconds);
  Serial.print(" second(s), ");
  Serial.print(printTicks - oldTicks);
  Serial.print(" ticks this second, ");
  Serial.print(printTicks);
  Serial.print(" total ticks, ");
  Serial.print(timerOverflows);
  Serial.print(" timer overflows, ");

  /* An ideal 32kHz oscillator ticks 32768 times per second. Here we calculate the difference between the number of actual ticks counted
  in a given time period compared with the ideal number over the same time period and convert it to parts per million. */
  float result = 0;
  result = (1.0 - ((float)printNumSeconds * 32768.0 / (float)printTicks)) * 1000000.0;
  Serial.print(result);
  Serial.println(" ppm");

  /* Update the "old" values so we can compare them the next time through. */
  oldNumSeconds = printNumSeconds;
  oldTicks = printTicks;

  /* Are we done? If so, stop. */
  if (printNumSeconds >= maxSeconds) {
    Serial.println("Done. Reset to start again.");
    while (1) {};
  }
}

void serial_init(){
    /* Setup serial to run at 9600 bps, no parity, one stop bit. */
  Serial.begin(9600);

  /* Prompt for total number of seconds to count for. */
  Serial.println("Count for how many seconds?");
  while (Serial.available() == 0) {} // Wait for input.
  maxSeconds = Serial.parseInt();

  /* Confirm the number of seconds to count for. */
  Serial.print("Counting for ");
  Serial.print(maxSeconds);
  Serial.println(" seconds.");
}

void setup() {

  timer1_init();
  serial_init();

}


void loop() {
  if (firstRun) { // If this is the first time through the loop, reset everything to start from a consistent state.
    resetCounters();
  }

  /* Interrupts can occur at any time and can change the "ticks" and "numSeconds" variables without warning, causing corruption.
   * We can avoid this by briefly disabling interrupts, copying the values into other variables not changed by interrupts,
   * then re-enabling interrupts. This is very fast and only takes a few clock cycles.
  */
  cli(); // Disable interrupts.
  printNumSeconds = numSeconds; // Copy value into new variable.
  printTicks = ticks; // Copy value into new variable.
  sei(); // Re-enable interrupts.

  /* Wait until we have new data, then print the results to the serial console.
   * We could use delay(1000) instead, but over long periods of time the start of the serial message would drift relative
   * to the PPS signal marking the start of each second. This drift might cause issues, so it's better to keep the serial
   * messages more closely aligned to the start of the second.
   */
  if (printNumSeconds > oldNumSeconds) { // Check to see if its a new second.
    printResults();
  }
}
