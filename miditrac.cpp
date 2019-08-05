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

constexpr auto PPQ = 96; // 96 tics per bar
constexpr auto TPN32 = 3; // 3 tics per 1/32 Note
constexpr auto N32 = 32; // smallest step unit
constexpr auto ZEIT = 60000000; // timeOperator
constexpr auto ZDIV = 24000; // timeDivider

double speed = 125; // Default BPM
bool recOff = false, recOn = true, eraseVector = false, live = false;
bool intr0 = true, ende = false, eraseMap = false, guiOn = true;

typedef unsigned int uint;
uint countNoteOn = 0, countNoteOff = 0, countSteps = 0;
uint loop = 4; // 16 steps (1/16) 6 * 16 = 96 tics * 256 bars = 24576 tics
uint divider = 4; // default divider 1/8 Notes...
uint quantize = 4; // live recording quantize to 1/8 Notes

std::vector<uint> midiInVector, timeVector, liveMidi, liveTime, midiPlayer;
std::multimap<uint, uint>*  m = new std::multimap<uint, uint>;
std::multimap<uint, uint>* undo = new std::multimap<uint, uint>;

HMIDIOUT hMidiOut;
HMIDIIN hMidiIn;

/*Setup**************/
void intro(bool* intr0) {
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
	if (*intr0) std::cout << sp4c3 << in7r0;
	std::cout << "\033[35m" << l1n35 <<
		" \033[32m\033[7m[LSTRG]\033[0m\033[36m Play  \033[30m\033[41m[ESC]\033[0m\033[36m"
		" Record  \033[30m\033[43m[BACK]\033[0m\033[36m undo/eraseBuffer\n"
		" \033[35m\033[7m[RSHIFT]\033[0m\033[36m erase Tracks  \033[34m\033[7m[LSHIFT]\033[0m\033[36m change ClockDivider\n"
		" \033[41m\033[7mClkDiv: 1/32=1 1/16=2 1/12=3 \033[32m1/8=4\033[36m 1/4=8 ... 1/1=32\033[0m\033[35m\n" << l1n35 << std::endl;
}
void setGMinstrument(HMIDIOUT* hMidiOut, uint instrument[]) {
	for (uint i = 0; i < 16; i++) {
		midiOutShortMsg(*hMidiOut, (256 * instrument[i]) + 192 + i);
	}
}
void getMidiInDev(uint* dev_in, bool& intr0)
{
	system("cls");
	intro(&intr0);
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
void getMidiOutDev(uint* dev_out, bool& intr0)
{
	system("cls");
	intro(&intr0);
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
void getSetup(uint* loop)
{
	system("cls");
	intro(&intr0);
	std::cout << " \033[36m\033[7m alt. MidiController Transport Button Configuration\033[0m\033[33m" << std::endl;
	std::cout << " \033[32m\033[7m[START]\033[0m\033[36m Play  \033[30m\033[41m[STOP]\033[0m\033[36m"
		" Record  \033[30m\033[43m[REC]\033[0m\033[36m undo/eraseBuffer" << std::endl;
	std::cout << " \033[37m\033[7mpress [SPACE] while playing to enable liveRec.-Mode\033[0m\033[33m" << std::endl;
	static unsigned char l1n35[53];
	for (uint i = 0; i < 53; i++) l1n35[i] = 205;
	std::cout << "\033[35m" << l1n35 << "\033[33m" << std::endl;
	std::cout << " Patternlength (bars): ";
	std::cin >> *loop;
}
void setMidiClock(std::multimap<uint, uint>* m, uint* loop) {
	for (uint i = 0; i < ((*loop)*PPQ); i++) {
		m->insert(std::pair<uint, uint>(i,0xf8));
	}
}
/*processing functions******************************************************************************************/
void processMidi(std::multimap<uint, uint>* m, std::vector<uint> midiPlayer, std::vector<uint>& midiInVector, std::vector<uint>& timeVector, uint* loop, uint* divider, bool* eraseMap) {
	
	std::vector<uint> copyTime = timeVector;
	std::vector<uint> copyMidi = midiInVector;
	std::vector<uint> v;
	
	//last NoteOFFs to begin
	if (copyMidi.size() > 2) {
		for (auto it = copyMidi.rbegin(); it != copyMidi.rend(); it++) {
			if (((*it) >> 4 & 0xf) == 8) {
				v.push_back(*it);

			}
			else if (((*it) >> 4 & 0xf) == 9) {
				break;
			}
		}
		for (uint i = 0; i < v.size(); i++) {
			copyMidi.pop_back();
			copyMidi.insert(copyMidi.begin(), v.at(i));
			copyTime.pop_back();
			copyTime.insert(copyTime.begin(), 0);
		}
	}
	
	//get number of recorded Notes
	uint onCount = 0, offCount = 0, sCounter = 0, numberMessage = copyTime.size();
	for (std::vector<uint>::iterator it = copyMidi.begin(); it != copyMidi.end(); it++) {
		if (((*it) >> 4 & 0xf) == 0x9 || ((*it) >> 4 & 0xfffff) == 0x7f40b) {
			onCount++;
			offCount = 0;
		}
		else {
			if (offCount < onCount) {
				sCounter++;
				onCount = 0;
			}
		}
	}
	
	// copy notes aslong the patternlength
	if (numberMessage > 0) {
		for (uint i = 0; i < ((*loop*N32)/(*divider)* (numberMessage/sCounter)); i++) {
			copyTime.push_back(copyTime.at(i));
			copyMidi.push_back(copyMidi.at(i));
		}
	}

	//sort Notes
	std::vector<uint>::iterator itTime = copyTime.begin();
	onCount = 0, offCount = 0;
	uint Count = 0;
	uint stepCount = 0;
	for (std::vector<uint>::iterator it = copyMidi.begin(); it != copyMidi.end(); it++) {
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

	//clear MapMemory and set MidiClock again
	if (*eraseMap) {
		m->clear();
		setMidiClock(m, loop);
		*eraseMap = false;
	}

	//delete sustain information
	std::multimap<uint, uint>* copyMap = new std::multimap<uint, uint>;
	for (size_t i = 0; i < copyTime.size(); i++) {
		copyMap->insert(std::pair<uint, uint>(copyTime.at(i), copyMidi.at(i)));
	}
	copyTime.clear();
	copyMidi.clear();

	std::vector<unsigned int> timeInfo, midiInfo;

	for (auto it = copyMap->begin(); it != copyMap->end(); it++) {
		if ((it->second >> 4 & 0xfff)!= 0x40b ) {
			midiInfo.push_back(it->second);
			timeInfo.push_back(it->first);
		}
	}
	delete copyMap;

	//insert new MapData
	for (unsigned int i = 0; i < timeInfo.size(); i++) {
		m->insert(std::pair<unsigned int, unsigned int>(timeInfo.at(i), midiInfo.at(i)));
	}
}
void liveProcessing(std::multimap<uint, uint>* m, std::vector<uint>& liveTime, std::vector<uint> liveMidi, double* speed, uint* quantize, uint* loop) {
	if (liveMidi.size() > 0) {
		uint i = 0;
		for (auto it = liveTime.begin(); it != liveTime.end(); it++) {
			*it = *it / (uint)((ZEIT / (*speed)) / ZDIV);
			if ((liveMidi.at(i) >> 4 & 0xf) ==  0x9) { // == NoteOn
				*it = (*it) / (3*(*quantize)); // 12 = 1/8 Quantize
				*it = (*it) * (3*(*quantize)); // 6 = 1/16 Quantize
			}
			else if ((*it) >= (*loop) * PPQ) {
				*it = (*loop) * PPQ - 1;
			}
			i++;
		}
	

		for (uint i = 0; i < liveMidi.size(); i++) {
			m->insert(std::pair<uint, uint>(liveTime.at(i), liveMidi.at(i)));
		}
	}
}
/*play & record functions**************************************************************************************************/
void play(HMIDIOUT* hMidiOut, HMIDIIN* hMidiIn, std::multimap<uint, uint>* m,std::multimap<uint, uint>* undo, uint* loop,
	double* speed, bool* eraseMap, bool* eraseVector, bool* ende,bool* recOn, bool* guiOn) {
	
	if (!(*guiOn)) std::cout << "\r\t\t\t\t\t\b\033[7m\033[32m   PLAY   \033[0m";
	midiOutShortMsg(*hMidiOut, 0xfa); // send MidiClock START

	bool playing = true;
	while (playing) {
		if (!live) std::cout << "\r\t\t\t\t\t\t\033[0m   \033[0m";

		std::multimap<uint, uint>::iterator it = m->begin();
		uint mSize = m->size();

		midiInStart(*hMidiIn); // enable midiIN

		auto startPlay = std::chrono::high_resolution_clock::now();

		for (uint i = 0; i < mSize; i++) {
			uint Last = it->second;
			midiOutShortMsg(*hMidiOut, it->second);
			
			if (it != m->end()) it++;
			
			bool wait = true;
			while (wait) {
				auto stopPlay = std::chrono::high_resolution_clock::now();

				if (std::chrono::duration_cast<std::chrono::milliseconds>(stopPlay - startPlay).count()
					>= (it->first * (ZEIT / (*speed)) / ZDIV))  wait = false;

				if (std::chrono::duration_cast<std::chrono::milliseconds>(stopPlay - startPlay).count()
					>= ((((double)(*loop) * (PPQ))) * (ZEIT / (*speed)) / ZDIV)) {
					wait = false;
					i = mSize;
				}

				if (*guiOn) { // display mididata & BPM
					std::cout << "\r\033[0m \033[33m\033[7mbeat counter: " << std::dec << std::setw(5) << it->first / 6 << " \033[0m  \033[7m\033[35m[ << ]\033[36m"
						" BPM: " << std::setw(3) << *speed << " \033[34m[ >> ]\033[0m";//" Midi-Message: " << std::hex << (*it).second;
				} 
				if ((*eraseVector) && (!live)) { // undo function (REC Button)
					*m = *undo;
					undo->clear();
					*eraseVector = false;
					playing = false;
					wait = false;
					i = mSize - 1;
				}
				if (*recOn == true) { // enable recording (STOP Button)
					playing = false;
					wait = false;
					i = mSize - 1;
				}
				if (GetKeyState(VK_RSHIFT) & 0x8000) { // erase Maps (Keyboard)
					playing = false;
					undo->clear();
					*eraseMap = true;
					i = mSize - 1;
				}
				if (GetAsyncKeyState(VK_LEFT) & 1) { //--BPM (slower)
					if ((*speed) > 30.0) (*speed) -= 1.0;
					wait = false;
				}
				if (GetAsyncKeyState(VK_RIGHT) & 1) { //++BPM (faster)
					if ((*speed) < 240.0) (*speed) += 1.0;
					wait = false;
				}
				if (GetKeyState(VK_SPACE) & 0x8000) {
					std::cout << "\r\t\t\t\t\t\t\033[7m\033[31mREC\033[0m";
					live = true;
				}
				if (GetKeyState(VK_ESCAPE) & 0x8000) {
					*recOn = true;
					playing = false;
					wait = false;
					i = mSize - 1;
				}
				if (GetKeyState(VK_BACK) & 0x8000)* eraseVector = true;
			}
		}
		if (live) *undo = *m;
		if (live) liveProcessing(m, liveTime, liveMidi, speed, &quantize, loop);
		if (liveMidi.size() != 0) live = false;
		midiInStop(*hMidiIn); // disable midiIN 
		liveTime.clear();
		liveMidi.clear();
	}
	midiOutShortMsg(*hMidiOut, 0xfc); // send MidiClock STOP
	midiOutReset(*hMidiOut);
}
void rec(HMIDIIN* hMidiIn, std::vector<uint>& midiInVector, std::vector<uint>& timeVector, uint* countSteps, uint* divider, 
	uint* loop, bool* eraseVector, bool* recOn, bool* recOff, bool& intr0) {
	midiInVector.clear();
	timeVector.clear();
	system("cls");
	intro(&intr0);
	*countSteps = 0;
	midiInStart(*hMidiIn);
	std::cout << "\r \033[31m\033[7mStep recording...\033[0m\033[33m";
	while ((*recOff) == false) {
		//if (*countSteps == (*loop) * 8)* recOff = true, *recOn = false;
		if ((*eraseVector) == true) {
			*countSteps = 0;
			midiInVector.clear();
			timeVector.clear();
			*eraseVector = false;
		}
		if (GetKeyState(VK_LCONTROL) & 0x8000)* recOff = true, * recOn = false;
		if (GetKeyState(VK_BACK) & 0x8000)* eraseVector = true;
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
/*callback function******************************************************************************************/
void CALLBACK MidiInProc(HMIDIIN hmidiIN, uint wMsg, long dwInstance, uint dwParam1, uint dwParam2) {
	if (wMsg == MIM_DATA) {

		//live recording
		if (live == true && (STATUS == 0x9 || STATUS == 0x8)) {
			liveTime.push_back(dwParam2);
			liveMidi.push_back(dwParam1);
		}

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
			std::cout << "\r\033[0m \033[31m\033[7mStep: \033[32m" << std::dec << std::setw(3) << countSteps * divider / 2 
				<< " \033[33m ClkDiv:" << std::setw(3) << divider << std::hex << "  Midi-Message:\033[0m" << std::setw(8) << dwParam1;
		}
	}
	midiOutShortMsg(hMidiOut, dwParam1);
}

int main() {

	/*MidiTraC**************************************************************************/
	uint midiInDevice = 0;
	uint midiOutDevice = 2; // 0 = GS WaveTableSynth / 2 = loopMIDI Port
	uint instrument[16] = { 1, 13, 9, 38, 85, 81, 88, 80, 50, 0, 49, 39, 5, 87, 20, 55 };
	/*GS WavetableSynth - GM Instruments 0-127 @ [0-15] MidiChannels - channel9 = Drums*/

	/*set Console Window******************************/
	HWND hwnd = FindWindow("ConsoleWindowClass", NULL);
	MoveWindow(hwnd, 720, 10, 640, 210, TRUE); // without LOGO
	MoveWindow(hwnd, 64, 48, 640, 480, TRUE); // with LOGO 64 X 48 / 640 X 480 pixel 
	/*************************************************/

	/*set Midi Device*********************************/
	getMidiInDev(&midiInDevice, intr0);
	getMidiOutDev(&midiOutDevice, intr0);
	getSetup(&loop);
	/*************************************************/
	setMidiClock(m, &loop);
	/*start Midi**************************************************************************/
	midiOutOpen(&hMidiOut, midiOutDevice, 0, 0, 0);
	if (midiOutDevice == 0) setGMinstrument(&hMidiOut, instrument);
	midiInOpen(&hMidiIn, midiInDevice, (DWORD_PTR)(void*)MidiInProc, 0, CALLBACK_FUNCTION);
	/*************************************************************************************/
	intr0 = false;
	MoveWindow(hwnd, 720, 10, 640, 210, TRUE); // without LOGO
	/*MainLoop****************************************************************************/
	while (!ende) {
		*undo = *m;
		rec(&hMidiIn, midiInVector, timeVector, &countSteps, &divider, &loop, &eraseVector,
			&recOn, &recOff, intr0);
		
		processMidi(m, midiPlayer, midiInVector, timeVector, &loop, &divider, &eraseMap);
		
		play(&hMidiOut, &hMidiIn, m, undo, &loop, &speed, &eraseMap, &eraseVector, &ende,
			&recOn, &guiOn);
	}
	/*************************************************************************************/

	return 0;
}
