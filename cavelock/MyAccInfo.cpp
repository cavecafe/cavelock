/*
MIT License

Copyright(c) 2019 Thomas Kim

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <stdio.h>
#include <string>
#include <iostream>
#include <thread>
#include <cmath>
#include <signal.h>
#include <unistd.h>
#include <ctime>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "Platform.h"
#include "Util/CLI11.hpp"
#include "Crypto/Crypto.h"
#include "Crypto/MD.h"

#include "MyAccInfo.h"


#define GPIO_SERVO 18
#define GPIO_BUTTON 27
#define GPIO_RELAY 17

// for SPECTRUM A5030 servo
#define SERVO_RANGE_MAX		200
#define SERVO_UNLOCK_POS	11		// 0 degree
#define SERVO_LOCK_POS		24		// 90 degree
#define SERVO_WAIT_TIME		500

namespace Hap {

	time_t volatile done_at = 0;
	bool buttonProcessing = false;

	void onButtonPressed(void)
	{
		time_t now = time(0);
		Log::Msg("done_at=%d, now=%d\n", done_at, now);
		if (done_at + 2 <= now && !buttonProcessing) {
			buttonProcessing = true;
			done_at = now;
			Log::Dbg("Button pressed\n");
			myLM.ToggleState();
			buttonProcessing = false;
		}
	}

	MyAccInfo myAccInfo;
	MyLockMechanism  myLM(ACCESSORY_NAME, GPIO_SERVO, GPIO_BUTTON, GPIO_RELAY, onButtonPressed);
	MyAcc myAcc;
	MyDb myDb;

	File::FileConfig<MyDbType> cfgFile(&myDb, &DEFAULT_CONFIG, CONFIG_NAME);
	Config* cfg = &cfgFile;

	// statically allocated storage for HTTP processing
	// curent implementation is single-threaded so only one set of buffers.
	// The http server uses this buffers only during processing a request.
	// All session-persistent data is kept in Session objects.
	BufStatic<char, Hap::MaxHttpFrame * 2> http_req;
	BufStatic<char, Hap::MaxHttpFrame * 4> http_rsp;
	BufStatic<char, Hap::MaxHttpFrame * 1> http_tmp;
	Http::Server::Buf buf = { http_req, http_rsp, http_tmp };
	Http::Server http(buf, myDb, cfgFile.pairings, cfgFile.keys);

	//--------------------------------
	// implementation MyLockMechanism
	//--------------------------------

	MyLockMechanism::MyLockMechanism(Hap::Characteristic::Name::V name, int gpio_servo, int gpio_button, int gpio_relay, void (*readCallback) (void))
	{
		AddName(_name);
		AddCurrentState(_currentState);
		AddTargetState(_targetState);

		Init(gpio_servo, gpio_button, gpio_relay, readCallback);

		_name.Value(name);

		_currentState.onRead([this](Hap::Obj::rd_prm& p) -> void {
			Log::Msg("%s: read CurrentState: %d\n", _name.Value(), _currentState.Value());
			});
		_currentState.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::LockMechanismCurrentState::V v) -> void {
			Log::Msg("%s: write CurrentState: %d -> %d\n", _name.Value(), _currentState.Value(), v);
			});

		_targetState.onRead([this](Hap::Obj::rd_prm& p) -> void {
			Log::Msg("%s: read TargetState: %d\n", _name.Value(), _targetState.Value());
			});

		_targetState.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::LockMechanismTargetState::V v) -> void {
			Log::Msg("%s: write TargetState: %d -> %d\n", _name.Value(), _targetState.Value(), v);
			
			if (_targetState.Value() != v) {
				SetState(v);
				_currentState.Value(v);
			}

			});

	}

	MyLockMechanism::~MyLockMechanism()
	{
		Stop();
		Terminate();
	}

	void MyLockMechanism::Init(int gpio_servo, int gpio_button, int gpio_relay, void (*readCallback) (void)) {
		Log::Msg("%s: Init (servo=%d, button=%d, relay=%d)\n", _name.Value(), gpio_servo, gpio_button, gpio_relay);

		servo = new Servo(gpio_servo, gpio_relay);
		
		servo->Init();
		button = new Button(gpio_button);
		button->Init(readCallback);
	}

	void MyLockMechanism::Terminate() {
		if (servo) {
			servo->Detach();
			delete servo;
			servo = nullptr;
		}

		if (button) {
			delete button;
			button = nullptr;
		}
	}

	void MyLockMechanism::SetState(uint8_t v) {
		Log::Dbg("%s: SetState(%d)\n", _name.Value(), v);
		servo->Attach(0, SERVO_RANGE_MAX);
		servo->Write(v == 1 ? SERVO_LOCK_POS : SERVO_UNLOCK_POS);
		servo->Wait(SERVO_WAIT_TIME);
		servo->Detach();
	}

	void MyLockMechanism::Start()
	{
		Log::Msg("%s: Start\n", _name.Value());

		_run = true;

		// (tk)
		// Run();
	}

	void MyLockMechanism::Stop()
	{
		if (_run)
		{
			Log::Msg("%s: Stop\n", _name.Value());
			_run = false;

			// (tk)
			//if (_poll.joinable())
			//	_poll.join();
		}
	}

	void MyLockMechanism::ToggleState() 
	{
		uint8_t v = (_targetState.Value() == 0) ? 1 : 0;
		_targetState.Value(v);
		SetState(v);
		_currentState.Value(v);
	}

	void MyLockMechanism::Name(Characteristic::Name::V v)
	{
		_name.Value(v);
	}

	Characteristic::LockMechanismCurrentState::V MyLockMechanism::LockMechanismCurrentState()
	{
		return _currentState.Value();
	}
	void MyLockMechanism::LockMechanismCurrentState(Characteristic::LockMechanismCurrentState::V v)
	{
		_currentState.Value(v);
	}

	Characteristic::LockMechanismTargetState::V MyLockMechanism::LockMechanismTargetState()
	{
		return _targetState.Value();
	}

	void MyLockMechanism::LockMechanismTargetState(Characteristic::LockMechanismTargetState::V v)
	{
		_targetState.Value(v);
	}


	//------------------------
	// implementation MyAcc 
	//------------------------

	MyAcc::MyAcc()
	{
		AddService(&myAccInfo);
		AddService(&myLM);
	}


	//------------------------
	// implementation MyDb 
	//------------------------

	
	MyDb::MyDb() {
		AddAcc(&Hap::myAcc);
	}

	void MyDb::Init(iid_t aid)
	{
		// assign instance IDs
		myAcc.setId(aid);

		// config AIS
		myAccInfo.config();
	}

	bool MyDb::Restore(Json::ParserDef& js, int t)
	{
		if (js.type(t) != Hap::Json::JSMN_OBJECT)
		{
			Log::Msg("Db::Restore: 'db' is not an object");
			return false;
		}

		// (tk) make this as generic (NOT hard coded)
		struct LockMechanism {
			const char* nm;
			MyLockMechanism& lock;
		} _lock[] = {
			{ "lock1", myLM }
		};

		for (unsigned i = 0; i < sizeofarr(_lock); i++)
		{
			LockMechanism& lock = _lock[i];
			int a;	// accessory
			int c;	// characteristic

			a = js.find(t, lock.nm);
			if (a > 0)
			{
				a++;
				if (js.type(a) != Hap::Json::JSMN_OBJECT)
				{
					Log::Msg("Db::Restore: %s is not an object\n", lock.nm);
					continue;
				}

				c = js.find(a, "LockMechanismCurrentState");
				if (c > 0) {
					c++;
					Hap::Characteristic::LockMechanismCurrentState::V v;
					if (!js.is_number(c, v))
					{
						Log::Msg("Db::Restore: %s.LockMechanismCurrentState: invalid\n", lock.nm);
						continue;
					}
					Log::Msg("Db::Restore: %s.LockMechanismCurrentState: %d\n", lock.nm, v);
					lock.lock.LockMechanismCurrentState(v);
				}

				c = js.find(a, "LockMechanismTargetState");
				if (c > 0) {
					c++;
					Hap::Characteristic::LockMechanismTargetState::V v;
					if (!js.is_number(c, v))
					{
						Log::Msg("Db::Restore: %s.LockMechanismTargetState: invalid\n", lock.nm);
						continue;
					}
					Log::Msg("Db::Restore: %s.LockMechanismTargetState: %d\n", lock.nm, v);
					lock.lock.LockMechanismTargetState(v);
				}
			}
		}

		return true;
	}

	// (tk) todo, make it as generic
	bool MyDb::Save(FILE* f)
	{
		fprintf(f, "\t\t\"lock1\":{\n");
		fprintf(f, "\t\t\t\"LockMechanismCurrentState\":%d,\n", myLM.LockMechanismCurrentState());
		fprintf(f, "\t\t\t\"LockMechanismTargetState\":%d,\n", myLM.LockMechanismTargetState());
		fprintf(f, "\t\t},\n");

		return true;
	}


}