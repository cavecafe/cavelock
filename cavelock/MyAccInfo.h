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


#ifndef __MYACCINFO_H__
#define __MYACCINFO_H__


#include "Util/FileConfig.h"
#include "Servo/Servo.h"
#include "Button/Button.h"

#define ACCESSORY_NAME "Cave Lock"
#define ACCESSORY_MODEL "PI208"
#define CONFIG_NAME "/etc/cavelock.cfg"

namespace Hap {

	using MyDbType = DbStatic<1>;
	const File::FileConfig<MyDbType>::Default DEFAULT_CONFIG = {
		ACCESSORY_NAME,		// name
		ACCESSORY_MODEL,	// model
		"cavecafe studio",	// manufacturer
		"0001",				// serialNumber
		"0.1",				// firmwareRevision
		"111-00-111",		// setupCode
		7889				// TCP port
	};
	
	class MyAccInfo : public AccessoryInformation 
	{
	
	public:
		MyAccInfo() {
			_identify.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::Identify::V v) -> void {
				Log::Msg("MyAccInfo: write Identify\n");
				});
		}

		void config()
		{
			_manufacturer.Value(Hap::cfg->manufacturer);
			_model.Value(Hap::cfg->model);
			_name.Value(Hap::cfg->name);
			_serialNumber.Value(Hap::cfg->serialNumber);
			_firmwareRevision.Value(Hap::cfg->firmwareRevision);
		}

	};

	class MyLockMechanism : public LockMechanism
	{

	private:
		Characteristic::Name _name;
		Characteristic::LockMechanismCurrentState _currentState;
		Characteristic::LockMechanismTargetState _targetState;

		bool _run = false;
		Servo* servo = nullptr;
		Button* button = nullptr;

		void Init(int gpio_servo, int gpio_button, int gpio_relay, void (*readCallback) (void));
		void Terminate();
		void SetState(uint8_t v);

	public:
		MyLockMechanism(Characteristic::Name::V name, int servo_pin, int button_pin, int relay_pin, void (*readCallback) (void));
		virtual ~MyLockMechanism();

		void Start();
		void Stop();
		void ToggleState();
		
		void Name(Characteristic::Name::V v);
		Characteristic::LockMechanismCurrentState::V LockMechanismCurrentState();
		void LockMechanismCurrentState(Characteristic::LockMechanismCurrentState::V v);
		Characteristic::LockMechanismTargetState::V LockMechanismTargetState();
		void LockMechanismTargetState(Characteristic::LockMechanismTargetState::V v);

	};

	class MyAcc : public Accessory<2>
	{
	public:
		MyAcc();
	};

	class MyDb : public File::Db<MyDbType>
	{
	public:
		MyDb();
		
		void Init(iid_t aid);
		bool Restore(Json::ParserDef& js, int t) override;
		bool Save(FILE* f) override;
	};


	// declaration
	extern MyAccInfo myAccInfo;
	extern MyLockMechanism myLM;
	extern MyAcc myAcc;
	extern MyDb myDb;
}

#endif // __MYACCINFO_H__
