/*+++ MidiTraC v2 +++ © 2019 AliceD25 +++ irrenhouse(at)web.de*/
/*polyphonic Step Sequencer inspired by Roland JX-3P, MSQ-700*/
/********************************************************************************************************************/
/* This program is free software: you can redistribute it and / or modify it under the terms of the GNU General
   Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License...
   for more details: <http://www.gnu.org/licenses/>.
*********************************************************************************************************************/

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <chrono>
#include <Windows.h>

#pragma comment(lib, "winmm.lib")

#define POLYPHONIE 8
#define CLKDIV 7500 // 1/32 Notes
#define STATUS (dwParam1 >> 4 & 0xf)

typedef unsigned int uint;
uint loop = 1024; // Patternlength
uint midiInDevice = 0; 
uint midiOutDevice = 0; // 0 = GM WaveTable Synth
uint instrument[16] = { 1, 13, 9, 38, 85, 81, 88, 80, 50, 0, 49, 39, 5, 87, 20, 55 };
/*********************GM Instruments 0-127 @ [0-15] MidiChannels - channel9 = Drums*/

uint countNoteOn = 0, countNoteOff = 0, countSteps = 0;
std::vector<long> midiInVector, timeVector;
std::multimap<long, long>  m;

HMIDIOUT hMidiOut;
HMIDIIN hMidiIn;

long divider = 4; // 1/8 Notes...
long speed = 240, bpm = 125; // ...@ 125 BPM (speed = 240ms)
bool eraseMap = false, ende = false, recOff = false, recOn = true, eraseVector = false;

void intro();
void setGMinstrument(HMIDIOUT *hMidiOut, uint instrument[]);
void getMidiInDev(uint* dev_in);
void getMidiOutDev(uint* dev_out);
void getSettings(long* bpm, long* speed, long* divider, uint* loop);
void copySeq(std::vector<long>& midiInVector, std::vector<long>& timeVector, uint* loop);
void sortSeq(std::vector<long> midiInVector, std::vector<long>& timeVector, long* speed, long * divider, long* bpm);
void mapSeq(std::multimap<long, long>& m, std::vector<long>& midiInVector, std::vector<long>& timeVector, bool* eraseMap);
void play(HMIDIOUT* hMidiOut, HMIDIIN* hMidiIn, std::multimap<long, long>& m, bool* eraseMap, bool* ende, long* divider, uint* loop);
void rec(HMIDIIN* hMidiIn, std::vector<long>& midiInVector, std::vector<long>& timeVector, uint* countSteps, long* divider);
void CALLBACK MidiInProc(HMIDIIN hmidiIN, uint wMsg, long dwInstance, long dwParam1, long dwParam2);

int main() {
	HWND hwnd = FindWindow("ConsoleWindowClass", NULL);
	MoveWindow(hwnd, 64, 48, 640, 480, TRUE);
	system("color 13");
	getMidiInDev(&midiInDevice);
	getMidiOutDev(&midiOutDevice);
	getSettings(&bpm, &speed, &divider, &loop);
	midiOutOpen(&hMidiOut, midiOutDevice, 0, 0, 0);
	if (midiOutDevice == 0) setGMinstrument(&hMidiOut, instrument);
	midiInOpen(&hMidiIn, midiInDevice, (DWORD_PTR)(void*)MidiInProc, 0, CALLBACK_FUNCTION);
	while (!ende) {
		rec(&hMidiIn, midiInVector, timeVector, &countSteps, &divider);
		copySeq(midiInVector, timeVector, &loop);
		sortSeq(midiInVector, timeVector, &speed, &divider, &bpm);
		mapSeq(m, midiInVector, timeVector, &eraseMap);
		play(&hMidiOut, &hMidiIn, m, &eraseMap, &ende, &speed, &loop);
	}
	return 0;
}

void intro() {
	static const unsigned char in7r0[] = { 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 95, 32, 32, 32, 32, 65, 108, 105, 99, 101, 95, 68, 50, 53, 32, 184, 10, 32, 32, 95, 32, 95, 95, 32,
	95, 95, 95, 32, 32, 32, 95, 32, 32, 32, 32, 32, 95, 32, 95, 47, 32, 124, 95, 32, 95, 95, 95, 32, 32, 95, 95, 32, 95,
	32, 32, 95, 95, 10, 32, 124, 32, 96, 95, 32, 96, 32, 95, 32, 92, 32, 40, 95, 41, 32, 95, 95, 124, 32, 40, 95, 41, 32,
	32, 95, 124, 32, 95, 95, 124, 47, 32, 95, 96, 32, 124, 47, 32, 95, 124, 10, 32, 124, 32, 124, 32, 124, 32, 124, 32, 
	124, 32, 124, 124, 32, 124, 47, 32, 95, 96, 32, 124, 32, 124, 32, 124, 32, 124, 32, 124, 32, 124, 32, 40, 95, 124, 
	32, 124, 32, 124, 95, 10, 32, 124, 95, 124, 32, 124, 95, 124, 32, 124, 95, 124, 124, 95, 124, 92, 40, 95, 124, 95,
	124, 95, 124, 95, 95, 124, 124, 95, 124, 32, 32, 92, 95, 95, 44, 95, 124, 92, 95, 95, 124, 112, 111, 108, 121, 112,
	104, 111, 110, 105, 99, 10 };
	std::cout << in7r0 <<   "***************************************************\n"
				"[Space] stop Step Record  [LSTRG] start Step Record\n"
				"[Rshift] Divider [Back] erase [TAB] eraseSeq  [ESC]\n"
				"Divider: 1/32=1 1/16=2 1/12=3 1/8=4 1/4=8 1/2=16...\n"
				"***************************************************\n" << std::endl;
}
void setGMinstrument(HMIDIOUT* hMidiOut, uint instrument[]) {
	for (uint i = 0; i < 16; i++) {
		midiOutShortMsg(*hMidiOut, (256 * instrument[i]) + 192 + i);
	}
}
void getMidiInDev(uint* dev_in)
{
	system("cls");
	intro();
	uint nMidiDeviceNum;
	MIDIINCAPS caps;
	nMidiDeviceNum = midiInGetNumDevs();
	if (nMidiDeviceNum > 0) {
		std::cout << " Midi Input Devices:" << std::endl;
		for (unsigned int i = 0; i < nMidiDeviceNum; ++i) {
			midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS));
			std::cout << std::setw(2) << i << " " << caps.szPname << std::endl;
		}
		std::cout << " Select Midi Input: ";
		std::cin >> *dev_in;
	}
	else std::cout << " no Midi Input Device found!" << std::endl;

}
void getMidiOutDev(uint* dev_out)
{
	system("cls");
	intro();
	uint nMidiDeviceNum;
	MIDIOUTCAPS caps;
	nMidiDeviceNum = midiOutGetNumDevs();
	std::cout << " Midi Output Devices: " << std::endl;
	if (nMidiDeviceNum > 0) {
		for (unsigned int i = 0; i < nMidiDeviceNum; ++i) {
			midiOutGetDevCaps(i, &caps, sizeof(MIDIOUTCAPS));
			std::cout << std::setw(2) << i << " " << caps.szPname << std::endl;

		}
		std::cout << " Select Midi Output: ";
		std::cin >> *dev_out;
	}
	else std::cout << " no Midi Output Device found!" << std::endl;
}
void getSettings(long* bpm, long* speed, long* divider, uint* loop) {
	system("cls");
	intro();
	std::cout << "\r BPM: ";
	std::cin >> *bpm;
	*divider *= CLKDIV;
	*speed = *divider / *bpm;
}
void copySeq(std::vector<long>& midiInVector, std::vector<long>& timeVector, uint* loop) {
	uint onCount = 0, offCount = 0, stepCount = 0;
	for (std::vector<long>::iterator it = midiInVector.begin(); it != midiInVector.end(); it++) {
		if (((*it) >> 4 & 0xf) == 0x9 || ((*it) >> 4 & 0xfffff) == 0x7f40b) {
			onCount++;
			offCount = 0;
		}
		else {
			if (offCount < onCount) {
				stepCount++;
				onCount = 0;
			}
		}
	}
	uint memorySize = POLYPHONIE * (*loop);
	size_t recSize = timeVector.size();
	if (recSize > 0) {
		for (unsigned int i = 0; i < (memorySize * ((recSize / 2) / stepCount) - recSize); i++) {
			timeVector.push_back(timeVector.at(i));
			midiInVector.push_back(midiInVector.at(i));
		}
	}
}
void sortSeq(std::vector<long> midiInVector, std::vector<long>& timeVector, long* speed, long* divider, long* bpm) {
	std::vector<long>::iterator itTime = timeVector.begin();
	uint onCount = 0, offCount = 0;
	long stepCount = 0;
	for (std::vector<long>::iterator it = midiInVector.begin(); it != midiInVector.end(); it++) {
		if (((*it) >> 4 & 0xf) == 0x9 || ((*it) >> 4 & 0xfffff) == 0x7f40b) {
			onCount++;
			*itTime = stepCount * (*divider / *bpm); //(*speed);
			offCount = 0;
		}
		else {
			if (offCount < onCount) {
				stepCount++;
				onCount = 0;
			}
			*itTime = stepCount * (*divider / *bpm); //(*speed);
		}
		itTime++;
	}
}
void mapSeq(std::multimap<long, long>& m, std::vector<long>& midiInVector, std::vector<long>& timeVector, bool* eraseMap) {
	if (*eraseMap) {
		m.clear();
		*eraseMap = false;
	}
	for (size_t i = 0; i < timeVector.size(); i++) {
		m.insert(std::pair<long, long>(timeVector.at(i), midiInVector.at(i)));
	}
	midiInVector.clear();
	timeVector.clear();
}
void play(HMIDIOUT *hMidiOut, HMIDIIN *hMidiIn, std::multimap<long, long>& m, bool* eraseMap, bool* ende, long* speed, uint* loop) {
	bool playing = true;
	while (playing) {
		midiInStart(*hMidiIn);
		midiOutShortMsg(*hMidiOut, 0xfa); // send MidiClock START
		uint onCount = 0, offCount = 0, stepCount = 0;
		auto startPlay = std::chrono::steady_clock::now();
		std::multimap<long, long>::iterator it = m.begin();
		for (uint i = 0; i < m.size(); i++) {
			midiOutShortMsg(*hMidiOut, it->second);
			bool wait = true;
			if (i < m.size() - 1) it++;
			while (wait) {
				auto stopPlay = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::milliseconds>(stopPlay - startPlay).count() >= it->first) wait = false;
				if (std::chrono::duration_cast<std::chrono::milliseconds>(stopPlay - startPlay).count() == ((double)(*speed) *(*loop))) i = m.size()-1;
				if (recOn == true) {
					i = m.size() - 1;
					playing = false;
				}
				if (GetKeyState(VK_LSHIFT) & 0x8000) {
					i = m.size() - 1;
					playing = false;
					*eraseMap = true;
				}
				std::cout << "\r Step: " << std::dec << std::setw(3) << it->first / (*speed) << " Midi-Message: " << std::hex << (*it).second;
			}
		}
	}
	midiOutShortMsg(*hMidiOut, 0xfc); // send MidiClock STOP
	midiOutReset(*hMidiOut);
}
void rec(HMIDIIN *hMidiIn, std::vector<long>& midiInVector, std::vector<long>& timeVector, uint* countSteps, long* divider) {
	midiInVector.clear();
	timeVector.clear();
	system("cls");
	intro();
	*countSteps = 0;
	midiInStart(*hMidiIn);
	std::cout << "\r Step recording...";
	while (recOff == false) {
		if (eraseVector == true){
			*countSteps = 0;
			midiInVector.clear();
			timeVector.clear();
			std::cout << "\r erase Buffer ";
			eraseVector = false;
		}
		if (GetKeyState(VK_RSHIFT) & 0x8000) {
			std::cout << "\r change ClockDivider from " << (*divider)/CLKDIV <<" to: ";
			std::cin >> *divider;
			*divider *= CLKDIV;
		}
	}
	midiInStop(*hMidiIn);

	recOff = false;
}
void CALLBACK MidiInProc(HMIDIIN hmidiIN, uint wMsg, long dwInstance, long dwParam1, long dwParam2) {
	if (wMsg == MIM_DATA) {
		if (STATUS == 0x9 || STATUS == 0x8 || (dwParam1 >> 4 & 0xfffff) == 0x7f40b || (dwParam1 >> 4 & 0xfffff) == 0x0040b) {
			if (((dwParam1) >> 4 & 0xf) == 0x9 || ((dwParam1) >> 4 & 0xfffff) == 0x7f40b) {
				countNoteOn++;
				countNoteOff = 0;
			}
			else {
				if (countNoteOff < countNoteOn) {
					countSteps++;
					countNoteOn = 0;
				}
			}
			timeVector.push_back(dwParam2);
			midiInVector.push_back(dwParam1);
		}
		/*Transport Buttons*/
		if ((dwParam1 >> 4 & 0xfffff) == 0x7f36b) recOff = true, recOn = false; //Start Button
		if ((dwParam1 >> 4 & 0xfffff) == 0x7f33b) recOn = true; // Stop Button
		if ((dwParam1 >> 4 & 0xfff) == 0x32b) eraseVector = true; // Rec Button
		if (recOn) std::cout << "\r Step:" << std::setw(3) << std::dec << countSteps << std::hex << " Note Message:" << std::setw(8) << dwParam1;
	}
	midiOutShortMsg(hMidiOut, dwParam1);
}
/*+++ MidiTraC v3 +++ © 2019 AliceD25 +++ irrenhouse(at)web.de*/
/*polyphonic Step Sequencer inspired by Roland JX-3P, MSQ-700*/
/********************************************************************************************************************/
/* This program is free software: you can redistribute it and / or modify it under the terms of the GNU General
   Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License...
   for more details: <http://www.gnu.org/licenses/>.
*********************************************************************************************************************/

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <chrono>
#include <Windows.h>

#pragma comment(lib, "winmm.lib")

#define STATUS (dwParam1 >> 4 & 0xf) /*  Midi Status @ any Midi Channel */ 
#define STAT_BUTTON (dwParam1 >> 4 & 0xfffff) /* Button Control*/
#define IS_NOTEON STATUS == 0x9 /* Note On @ any Midi Channel */
#define IS_NOTEOFF STATUS == 0x8 /* Note Off @ any Midi Channel*/
#define SUSTAIN_ON STAT_BUTTON == 0x7f40b /* press Sustain */
#define SUSTAIN_OFF STAT_BUTTON == 0x0040b /* release Sustain */
#define START_BUTTON STAT_BUTTON == 0x7f36b /* press Button */
#define STOP_BUTTON STAT_BUTTON == 0x7f33b /* press Button */
#define REC_BUTTON (dwParam1 >> 4 & 0xfff) == 0x32b /* press & release Button */

constexpr auto CLKDIV = 7500;
constexpr auto PPQ = 96; // 96 tics per bar
constexpr auto TPN32 = 3; // 3 tics per 1/32 Note
constexpr auto N32 = 32; // smallest step unit

double speed = 125; // BPM
bool ende = false, recOff = false, recOn = true, eraseMap = false, eraseVector = false;

typedef unsigned int uint;
uint loop = 256; // 16 steps (1/16) 6 * 16 = 96 tics * 256 bars = 24576 tics
uint divider = 4; // default divider 1/8 Notes...
uint countNoteOn = 0, countNoteOff = 0, countSteps = 0;

uint midiInDevice = 0;
uint midiOutDevice = 0; // 0 = GS WaveTableSynth
uint instrument[16] = { 1, 13, 9, 38, 85, 81, 88, 80, 50, 0, 49, 39, 5, 87, 20, 55 };
/*GS WavetableSynth - GM Instruments 0-127 @ [0-15] MidiChannels - channel9 = Drums*/

std::vector<uint> midiInVector, timeVector;
std::multimap<uint, uint>  m;

HMIDIOUT hMidiOut;
HMIDIIN hMidiIn;

void intro() {
	std::cout << "\033[33m";
	static unsigned char sp4c3[23];
	for (uint i = 0; i < 23; i++) sp4c3[i] = 32;
	static unsigned char in7r0[] = { 95, 32, 32, 184, 32, 65, 108, 105, 99, 101, 68, 50, 53, 32, 50, 48, 49, 57, 32,
	10, 32, 32, 95, 32, 95, 95, 32, 95, 95, 95, 32, 32, 32, 95, 32, 32, 32, 32, 32, 95, 32, 95, 47, 32, 124, 95, 32, 
	95, 95, 95, 32, 32, 95, 95, 32, 95, 32, 32, 95, 95, 10, 32, 124, 32, 96, 95, 32, 96, 32, 95, 32, 92, 32, 40, 95,
	41, 32, 95, 95, 124, 32, 40, 95, 41, 32, 32, 95, 124, 32, 95, 95, 124, 47, 32, 95, 96, 32, 124, 47, 32, 95, 124,
	10, 32, 124, 32, 124, 32, 124, 32, 124, 32,124, 32, 124, 124, 32, 124, 47, 32, 95, 96, 32, 124, 32, 124, 32, 124,
	32, 124, 32, 124, 32, 124, 32, 40, 95, 124, 32, 124, 32, 124, 95, 10, 32, 124, 95, 124, 32, 124, 95, 124, 32, 124,
	95, 124, 124, 95, 124, 92, 40, 95, 124, 95, 124, 95, 124, 95, 95, 124, 124, 95, 124, 32, 32, 92, 95, 95, 44, 95,
	124, 92, 95, 95, 124, 112, 111, 108, 121, 112, 104, 111, 110, 105, 99, 10 };
	static unsigned char l1n35[53];
	for (uint i = 0; i < 53; i++) l1n35[i] = 205;
	std::cout << sp4c3 << in7r0 << "\033[35m" << l1n35 << 
		" \033[32m\033[7m[START]\033[0m\033[36m Play  \033[30m\033[41m[STOP]\033[0m\033[36m"
		" Record  \033[30m\033[43m[REC]\033[0m\033[36m erase MidiBuffer\n"
		" \033[35m\033[7m[RSHIFT]\033[0m\033[36m erase Tracks  \033[34m\033[7m[LSHIFT]\033[0m\033[36m change ClockDivider\n"
		" \033[41m\033[7mClkDiv: 1/32=1 1/16=2 1/12=3 \033[32m1/8=4\033[36m 1/4=8 ... 1/1=32\033[0m\033[35m\n" << l1n35 << std::endl;
}
void setGMinstrument(HMIDIOUT* hMidiOut, uint instrument[]) {
	for (uint i = 0; i < 16; i++) {
		midiOutShortMsg(*hMidiOut, (256 * instrument[i]) + 192 + i);
	}
}
void getMidiInDev(uint* dev_in)
{
	system("cls");
	intro();
	uint nMidiDeviceNum;
	MIDIINCAPS caps;
	nMidiDeviceNum = midiInGetNumDevs();
	if (nMidiDeviceNum > 0) {
		std::cout << " \033[31m\033[7mMidi Input Devices:\033[0m\033[33m" << std::endl;
		static unsigned char l1n35[53];
		for (uint i = 0; i < 53; i++) l1n35[i] = 205;
		std::cout << "\033[35m" << l1n35 << "\033[33m" << std::endl;
		for (uint i = 0; i < nMidiDeviceNum; ++i) {
			midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS));
			std::cout << std::setw(2) << i << " " << caps.szPname << std::endl;
		}
		std::cout << "\033[35m" << l1n35 << "\033[33m" << std::endl;
		std::cout << " Select Midi Input: ";
		std::cin >> *dev_in;
	}
	else  std::cout << " Input Device not found!" << std::endl;

}
void getMidiOutDev(uint* dev_out)
{
	system("cls");
	intro();
	uint nMidiDeviceNum;
	MIDIOUTCAPS caps;
	nMidiDeviceNum = midiOutGetNumDevs();
	std::cout << " \033[32m\033[7mMidi Output Devices:\033[0m\033[33m" << std::endl;
	static unsigned char l1n35[53];
	for (uint i = 0; i < 53; i++) l1n35[i] = 205;
	std::cout << "\033[35m" << l1n35 << "\033[33m" << std::endl;
	if (nMidiDeviceNum > 0) {
		for (uint i = 0; i < nMidiDeviceNum; ++i) {
			midiOutGetDevCaps(i, &caps, sizeof(MIDIOUTCAPS));
			std::cout << std::setw(2) << i << " " << caps.szPname << std::endl;

		}
		std::cout << "\033[35m" << l1n35 << "\033[33m" << std::endl;
		std::cout << " Select Midi Output: ";
		std::cin >> *dev_out;
	}
	else std::cout << " no Midi Output Device found!" << std::endl;
}
void setMidiClock(std::multimap<uint, uint>& m, uint* loop) {
	for (uint i = 0; i < ((*loop)*PPQ); i++) {
		m.insert(std::pair<uint, uint>(i,0xf8));
	}
}
void copySeq(std::vector<uint>& midiInVector, std::vector<uint>& timeVector, uint* loop, uint* divider) {
	uint onCount = 0, offCount = 0, stepCount = 0, numberMessage = timeVector.size();
	for (std::vector<uint>::iterator it = midiInVector.begin(); it != midiInVector.end(); it++) {
		if (((*it) >> 4 & 0xf) == 0x9 || ((*it) >> 4 & 0xfffff) == 0x7f40b) {
			onCount++;
			offCount = 0;
		}
		else {
			if (offCount < onCount) {
				stepCount++;
				onCount = 0;
			}
		}
	}
	if (numberMessage > 0) {
		for (uint i = 0; i < ((*loop*N32)/(*divider)* (numberMessage/stepCount)); i++) {
			timeVector.push_back(timeVector.at(i));
			midiInVector.push_back(midiInVector.at(i));
		}
	}
}
void sortSeq(std::vector<uint> midiInVector, std::vector<uint>& timeVector, uint* divider) {
	std::vector<uint>::iterator itTime = timeVector.begin();
	uint onCount = 0, offCount = 0;
	long stepCount = 0;
	for (std::vector<uint>::iterator it = midiInVector.begin(); it != midiInVector.end(); it++) {
		if (((*it) >> 4 & 0xf) == 0x9 || ((*it) >> 4 & 0xfffff) == 0x7f40b) {
			onCount++;
			*itTime = stepCount * (*divider) * TPN32;
			offCount = 0;
		}
		else {
			if (offCount < onCount) {
				stepCount++;
				onCount = 0;
			}
			*itTime = stepCount * (*divider) * TPN32;
		}
		itTime++;
	}
}
void mapSeq(std::multimap<uint, uint>& m, std::vector<uint>& midiInVector, std::vector<uint>& timeVector, uint* loop, bool* eraseMap) {
	if (*eraseMap) {
		m.clear();
		setMidiClock(m, loop);
		*eraseMap = false;
	}
	for (size_t i = 0; i < timeVector.size(); i++) {
		m.insert(std::pair<uint, uint>(timeVector.at(i), midiInVector.at(i)));
	}
	midiInVector.clear();
	timeVector.clear();
}
void play(HMIDIOUT* hMidiOut, HMIDIIN* hMidiIn, std::multimap<uint, uint>& m, uint* loop, double* speed, bool* eraseMap, bool* ende) {
	bool playing = true;
	while (playing) {

		midiInStart(*hMidiIn); // enable midi thru
		midiOutShortMsg(*hMidiOut, 0x0040b0); // sustain offset
		midiOutShortMsg(*hMidiOut, 0xfa); // send MidiClock START

		uint onCount = 0, offCount = 0, stepCount = 0;
		std::multimap<uint, uint>::iterator it = m.begin();
		auto startPlay = std::chrono::high_resolution_clock::now();
		std::cout << "\r\t\t\t\t\t\b\033[7m\033[32m   PLAY   \033[0m";
		for (uint i = 0; i < m.size(); i++) {
			midiOutShortMsg(*hMidiOut, it->second);
			bool wait = true;
			if (i < m.size() - 1) it++;
			while (wait) {
				auto stopPlay = std::chrono::high_resolution_clock::now();
				if (std::chrono::duration_cast<std::chrono::milliseconds>(stopPlay - startPlay).count() >= (it->first * (CLKDIV/(*speed))/TPN32)) wait = false;
				if (std::chrono::duration_cast<std::chrono::milliseconds>(stopPlay - startPlay).count() == ((CLKDIV/(*speed)) * (double)(*loop)*N32)) i = m.size() - 1;
				
				if (recOn == true) {
					i = m.size() - 1;
					playing = false;
				}
				if (GetKeyState(VK_RSHIFT) & 0x8000) {
					i = m.size() - 1;
					playing = false;
					*eraseMap = true;
				}
				//std::cout << "\r\033[0m beat counter: " << std::dec << std::setw(5) << it->first / 12 << "  [Left] 
				//	 BPM: " << std::setw(3) << *speed << " [Right]";//" Midi-Message: " << std::hex << (*it).second;
			}
			if (GetAsyncKeyState(VK_LEFT) & 1) {
				if ((*speed) > 30.0) (*speed) -= 1.0;
			}
			if (GetAsyncKeyState(VK_RIGHT) & 1) {
				if ((*speed) < 240.0) (*speed) += 1.0;
			}
		}
		midiOutReset(*hMidiOut);
	}
	midiOutShortMsg(*hMidiOut, 0xfc); // send MidiClock STOP
	midiOutShortMsg(*hMidiOut, 0x0040b0); // sustain offset
}
void rec(HMIDIIN* hMidiIn, std::vector<uint>& midiInVector, std::vector<uint>& timeVector, uint* countSteps, uint* divider, bool* eraseVector, bool* recOn, bool* recOff) {
	midiInVector.clear();
	timeVector.clear();
	system("cls");
	intro();
	*countSteps = 0;
	midiInStart(*hMidiIn);
	std::cout << "\r \033[31m\033[7mStep recording...\033[0m\033[33m";
	while ((*recOff) == false) {
		if ((*eraseVector) == true) {
			*countSteps = 0;
			midiInVector.clear();
			timeVector.clear();
			*eraseVector = false;
		}
		if (GetKeyState(VK_LSHIFT) & 0x8000) {
			*recOn = false;
			std::cout << "\r \033[36m\033[7mcurrent ClockDivider: " << (*divider) << "         set ClkDiv: \033[0m\033[7m\033[32m";
			std::cin >> *divider;
			std::cout << "\033[0m";
			*recOn = true;
		}
	}
	midiInStop(*hMidiIn);
	*recOff = false;
}
void CALLBACK MidiInProc(HMIDIIN hmidiIN, uint wMsg, long dwInstance, uint dwParam1, uint dwParam2) {
	if (wMsg == MIM_DATA) {

		/*Get Number of Steps*/
		if (IS_NOTEON || IS_NOTEOFF || SUSTAIN_ON || SUSTAIN_OFF) {
			if (IS_NOTEON || SUSTAIN_ON) {
				countNoteOn++;
				countNoteOff = 0;
			}
			else {
				if (countNoteOff < countNoteOn) {
					countSteps++;
					countNoteOn = 0;
				}
			}
			timeVector.push_back(dwParam2);
			midiInVector.push_back(dwParam1);
		}

		/*Transport Buttons*/
		if (START_BUTTON) recOff = true, recOn = false; //Start Button
		if (STOP_BUTTON) recOn = true; // Stop Button
		if (REC_BUTTON) eraseVector = true; // Rec Button

		/*Display Step & MidiData*/
		if (recOn) {
			std::cout << "\r\033[0m \033[33m\033[7mStep: " << std::dec << std::setw(3) << countSteps * divider / 2 << "  ClkDiv:" 
				<< std::setw(3) << divider << std::hex << "  Midi-Message:\033[0m" << std::setw(8) << dwParam1;
		}
	}
	midiOutShortMsg(hMidiOut, dwParam1);
}

int main() {

	/*set Console Window*******************************/
	HWND hwnd = FindWindow("ConsoleWindowClass", NULL);
	MoveWindow(hwnd, 64, 48, 640, 480, TRUE);
	/**************************************************/

	/*set Midi Device**********************************/
	getMidiInDev(&midiInDevice);
	getMidiOutDev(&midiOutDevice);
	/**************************************************/
	setMidiClock(m, &loop);
	/*start Midi***************************************************************************/
	midiOutOpen(&hMidiOut, midiOutDevice, 0, 0, 0);
	midiOutShortMsg(hMidiOut, 0x0040b0); // sustain offset
	if (midiOutDevice == 0) setGMinstrument(&hMidiOut, instrument);
	midiInOpen(&hMidiIn, midiInDevice, (DWORD_PTR)(void*)MidiInProc, 0, CALLBACK_FUNCTION);
	/**************************************************************************************/

	/*MainLoop*****************************************************************************************/
	while (!ende) {

		rec(&hMidiIn, midiInVector, timeVector, &countSteps, &divider, &eraseVector, &recOn, &recOff);
		copySeq(midiInVector, timeVector, &loop, &divider);
		sortSeq(midiInVector, timeVector, &divider);
		mapSeq(m, midiInVector, timeVector, &loop, &eraseMap);
		play(&hMidiOut, &hMidiIn, m, &loop, &speed, &eraseMap, &ende);
	}
	/******************************************************************/

	return 0;
}
