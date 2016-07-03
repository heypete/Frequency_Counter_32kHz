/* Initialize variables. Interrupt-accessible variables must be global and volatile. */
volatile unsigned int timerOverflows = 0;
volatile unsigned int numSeconds = 0;
volatile unsigned long ticks = 0;
unsigned int maxSeconds = 0;
unsigned int printDifference = 0;
unsigned int printNumSeconds = 0;
unsigned int printTimerOverflows = 0;
unsigned long printTicks = 0 - 1;
boolean firstRun = 1;

void startstop() {
  /* This is called by the interrupt. It needs to be fast.
   */
  ticks = TCNT1;
  numSeconds++;

}

ISR(TIMER1_OVF_vect) {
  timerOverflows++;
}

void setup() {

  /* The Arduino core enables the waveform generator. This conflicts with using TIMER1 for counting,
  * so we must disable the waveform generator.
  */
  TCCR1A &= ~((1 << WGM12) | (1 << WGM11) | (1 << WGM10));

  /* Setting CS12 and CS11 to 1 sets TIMER1 to use an external clock source and trigger on the falling edge. */
  TCCR1B |= ((1 << CS12) | (1 << CS11));

  /* Initialize the counter. */
  TCNT1 = 0;

  /* Initialize the TIMER1 overflow interrupt. We should never need this, but better safe than sorry. */
  TIMSK1 |= (1 << TOIE1);

  // Initalize the TIMER1 overflow counter.
  timerOverflows = 0;

  /* Enable pulse-per-second interrupt. */
  attachInterrupt(digitalPinToInterrupt(2), startstop, RISING);

  sei();

  /* Setup serial to run at 9600 bps, no parity, one stop bit. */
  Serial.begin(9600);

  /* Prompt for total number of seconds to count for. */
  Serial.println("Count for how many seconds?");
  while (Serial.available() == 0) {} // Wait for input.
  maxSeconds = Serial.parseInt();

  /* Confirm the number of seconds to count for. */
  Serial.print("Counting for ");
  Serial.print(maxSeconds);
  Serial.println(" seconds");

  /* To prevent mis-counting, we need to wait for the first full second, then reset the counters.  */
  Serial.print("Waiting for first full second...");
  while (numSeconds < 2) {
  }
  Serial.println("done.");
  Serial.println("Starting...");

  /* Resetting the counters must be atomic, so we disable interrupts first, reset the counters, then re-enable interrupts.*/
  cli();
  TCNT1 = 0;
  numSeconds = 0;
  timerOverflows = 0;
  ticks = 0;
  printTicks = 0;
  sei();
}


void loop() {
  if (firstRun) {
    while (numSeconds < 2) {
    }
    cli();
    TCNT1 = 0;
    numSeconds = 0;
    timerOverflows = 0;
    ticks = 0;
    printTicks = 0;
    sei();
    firstRun = 0;
  }
  static unsigned int oldNumSeconds = 65536;
  static unsigned long oldTicks = 0;
  cli();
  unsigned int printNumSeconds = numSeconds;
  unsigned long printTicks = ticks;
  sei();
  if (printNumSeconds > oldNumSeconds) {
    printTicks += timerOverflows*65536;
    Serial.print(printNumSeconds);
    Serial.print(" seconds, ");
    Serial.print(printTicks - oldTicks);
    Serial.print(" ticks this second, ");
    Serial.print(printTicks);
    Serial.print(" total ticks, ");
    Serial.print(timerOverflows);
    Serial.print(" timer overflows, ");
    float result = 0;
    result = (1.0 - ((float)printNumSeconds * 32768.0 / (float)printTicks)) * 1000000.0;
    Serial.print(result);
    Serial.println(" ppm");
    oldNumSeconds = printNumSeconds;
    oldTicks = printTicks;
    /* Are we done? If so, stop. */
    if (printNumSeconds >= maxSeconds) {
      Serial.println("Done. Reset to start again.");
      while (1) {};
    }
  }
}
