# MidiTraC
polyphonic Step Sequencer / Pattern Generator

https://youtu.be/VM5_3FSjN7w
https://youtu.be/qxsmedl3lIE

						_  ALICE_D25
			  _ __ ___   _     _ _/ |_ ___  __ _  __
			 | `_ ` _ \ (_) __| (_)  _| __|/ _` |/ _|
			 | | | | | || |/ _` | | | | | | (_| | |_ 
			 |_| |_| |_||_|\(_|_|_|__||_|  \__,_|\__| polyphonic

MidiTraC, a polyphonic MultiTrack Step Sequencer like to the one used in the Roland JX-3P,
or a Pattern Generator like the Roland MSQ-700. Input via midi keyboard can be monophonic
as well as polyphonic. Sustain serves a switch for Tie & Rest. While sustaining a note,
Sustain causes the note to extend, depending on the Clock Divider. In conjunction with 
e.g. Arturia keystep, the transport buttons can be used for START, STOP and RECORD. 
Another practical feature is the HOLD button, which can be set to Toggle via ControlCenter. 
(as shown in the video) MidiTraC controlled via LoopBack some VST's in Ableton, but it also 
works with hardware Synth's or multitimbral like G2 Nord Modular. The MIDI channel, activated
via the keyboard controller is recorded and played back. 

using Transport Buttons from Hardware Midi Controller:
[Start] stop Step recording (cc 0x33b)
[Stop] stop playing, start StepRecord (cc 0x32b)
[Rec] erase MidiBuffer, during StepRecord (cc 0x36b)

using Sustain Pedal (cc 0x40) for Tie & Rest Notes
while Key is hold [Sustain] tie Notes, else insert Rest...

MidiTraC v0.3:
+ send MidiClock
+ adjust BPM during play
+ add Keyboard Controls:
	+ adjust ClkDiv while recording = [LSHIFT]
	+ stop Step recording / Play    = [SPACE]
	+ start Step Record / Stop      = [LSTRG]
	+ erase Buffer while recording  = [BACK]
	+ erase Tracks while playing    = [RSHIFT]
	+ adjust Tempo while playing    = [LEFT<>Right]
+ remove display Steps & Events while playing
+ add Colors
