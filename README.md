# MidiTraC
polyphonic Step Sequencer

https://youtu.be/qxsmedl3lIE

						_  ALICE_D25
			  _ __ ___   _     _ _/ |_ ___  __ _  __
			 | `_ ` _ \ (_) __| (_)  _| __|/ _` |/ _|
			 | | | | | || |/ _` | | | | | | (_| | |_ 
			 |_| |_| |_||_|\(_|_|_|__||_|  \__,_|\__| polyphonic

using Transport Buttons from Hardware Midi Controller like Arturia Keystep
[Start] stop Step recording (cc 0x33b)
[Stop] stop playing, start StepRecord (cc 0x32b)
[Rec] erase MidiBuffer, during StepRecord (cc 0x36b)

using Sustain Pedal (cc 0x40) for Tie & Rest Notes
while Key is hold [Sustain] tie Notes
else insert Rest...

