# Frequency_Counter_32kHz
An Arduino sketch that measures the accuracy of a 32kHz signal.

I have a bunch of Maxim DS3231 temperature-compensated real-time clocks. They emit a 32,768Hz signal on one of their pins. This signal is specified to be within +/- 2ppm between 0 degrees and 40 degrees Celsius.

Since I have an extremely precise GPS-disciplined oscillator (a Trimble Thunderbolt) that produces a 5V (into a 50 ohm load, be careful of impedance matching and ringing!) one-pulse-per-second (1PPS) signal, I can use it to characterize the drift of the DS3231s or any other 32kHz oscillator that produces TTL-level output.

You can find a Thunderbolt on eBay, but the price is no longer as low as it was several years ago. Instead, you might consider using the PPS outputs of other common GPS timing receivers like the older Motorola Oncore UT+ or Trimble Resolution T, both of which are available at reasonable cost on eBay. So long as the rise time of the PPS signal is fast (a few tens of nS) and the phase stability of the PPS signal is reasonable (<200nS or so) it should be fine. This sketch has no way of differentiating between an inaccurate clock being tested and an inaccurate PPS signal.

Setup:

1. Connect the PPS signal across a 50 ohm resistor to the Arduino digital pin 2. The resistor is necessary for impedance matching and ensuring that ringing on the line doesn't cause voltage spikes that will damage your Arduino (and they will!).
2. Connect the 32kHz signal to the Arduino digital pin 5. Ensure you have the necessary pull-up resistor, if applicable. With the DS3231 I find that 10k pullup works well.
3. Compile and run the sketch.
4. Open the serial console at 9600 bps, no parity bit, one stop bit.
5. When prompted, enter the number of seconds you'd like to measure. 100-1000 seconds is pretty typical.
