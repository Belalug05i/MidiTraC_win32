# MidiTraC
polyphonic Step Sequencer

https://youtu.be/qxsmedl3lIE

						_  ALICE_D25
			  _ __ ___   _     _ _/ |_ ___  __ _  __
			 | `_ ` _ \ (_) __| (_)  _| __|/ _` |/ _|
			 | | | | | || |/ _` | | | | | | (_| | |_ 
			 |_| |_| |_||_|\(_|_|_|__||_|  \__,_|\__| polyphonic

MidiTraC, a polyphonic MultiTrack Step Sequencer like to the one used in the Roland JX-3P. 
The input via midi keyboard can be monophonic as well as polyphonic. Sustain serves as a
switch for Tie & Rest. While sustaining a note, Sustain causes the note to extend, 
depending on the Clock Divider. In conjunction with my Arturia keystep, the transport 
buttons can be used for START, STOP and RECORD. Another practical feature is the HOLD 
button, which can be set to Toggle via ControlCenter. (as shown in the video) MidiTraC 
controlled via LoopBack some VST's in Ableton, but the software can also be used with 
hardware Synth's or multitimbral (eg G2 Nord Modular). The MIDI channel, activated via the 
keyboard controller is recorded and played back. The number of steps or tracks and polyphony 
is arbitrary; in the attached VS build, a maximum of 1024 steps and 8-fold polyphony are possible.

using Transport Buttons from Hardware Midi Controller like Arturia Keystep
[Start] stop Step recording (cc 0x33b)
[Stop] stop playing, start StepRecord (cc 0x32b)
[Rec] erase MidiBuffer, during StepRecord (cc 0x36b)

using Sustain Pedal (cc 0x40) for Tie & Rest Notes
while Key is hold [Sustain] tie Notes
else insert Rest...

