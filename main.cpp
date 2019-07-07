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
	static const char cpyp[] = { 184, 32, 65, 108, 105, 99, 101, 95, 68, 50, 53 };
	printf( "                       _    %s			\n"
			"  _ __ ___   _     _ _/ |_ ___  __ _  __\n"
			" | `_ ` _ \\ (_) __| (_)  _| __|/ _` |/ _|\n"
			" | | | | | || |/ _` | | | | | | (_| | |_ \n"
			" |_| |_| |_||_|\\(_|_|_|__||_|  \\__,_|\\__| polyphonic\n%c", cpyp, 201);
	for (uint i = 0; i < 51;  i++) printf("%c",205);
	printf("%c\n%c[Start] stopRec  [Stop] startRec  [Rec] eraseBuffer%c\n", 187, 186, 186);
	printf("%c[L-Shift] erase Sequence [R-Shift] set ClockDivider%c\n", 186, 186);
	printf("%cDivider: 1/32=1 1/16=2 1/12=3 1/8=4 1/4=8 1/2=16...%c\n%c", 186, 186, 200);
	for (uint i = 0; i < 51; i++) printf("%c", 205);
	printf("%c\n", 188);

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
	intro()
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
