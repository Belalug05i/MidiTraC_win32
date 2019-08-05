# MidiTraC
polyphonic Step Sequencer / Pattern Creator

https://youtu.be/VM5_3FSjN7w

https://youtu.be/qxsmedl3lIE

						_  ALICE_D25
			  _ __ ___   _     _ _/ |_ ___  __ _  __
			 | `_ ` _ \ (_) __| (_)  _| __|/ _` |/ _|
			 | | | | | || |/ _` | | | | | | (_| | |_ 
			 |_| |_| |_||_|\(_|_|_|__||_|  \__,_|\__| polyphonic

MidiTraC, a polyphonic MultiTrack Step Sequencer like the one used in the Roland JX-3P, or a Pattern Creator like the Roland MSQ-700.
+ Recording Notes via midi keyboard can be monophonic as well as polyphonic.
+ Sustain serves a switch for Tie & Rest. While sustaining a note, Sustain causes the note to extend, depending on the Clock Divider. 
+ Using a SustainPedal(or cc 0x40) for Tie & Rest Notes: while a Key is pressed, [Sustain] tie Notes, else it insert a Rest. 
+ In conjunction with  a Midi-Keyboard Controller, the transport buttons canmbe used for: START, STOP & RECORD.
as shown in the video, MidiTraC controlled some VST Intruments in Ableton using LoopMIDI to route the MidiStream, but it also works well on most hardware Synth's.

using Transport Buttons from Midi Controller:
+ [Start] stop Step recording (cc 0x33b)
+ [Stop] stop playing, start StepRecord (cc 0x32b)
+ [Rec] erase MidiBuffer, during StepRecord (cc 0x36b)



MidiTraC v0.3:
+ send MidiClock
+ adjust BPM during play
+ add Keyboard Controls:
	+ adjust ClkDiv while recording = [LSHIFT]
	+ stop Step recording / Play    = [LSTRG]
	+ start Step Record / Stop      = [ESCAPE]
	+ erase Buffer while recording  = [BACK]
	+ erase Tracks while playing    = [RSHIFT]
	+ adjust Tempo while playing    = [LEFT<>Right]
+ add Colors

MidiTraC v0.4:
+ add liveRecordings
	+ press [SPACE] while playing to start liveRec.-Mode...
	+ recording is enaled till the end of the pattern, if Notes are recorded.
	+ after liveRec.-Mode ends itself, recorded Notes are quantize to 8n Notes
+ smaller UI and new MemoryManagement
+ display Stats while playing
+ add undo function...
