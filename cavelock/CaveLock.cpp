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

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "Platform.h"
#include "Util/CLI11.hpp"
#include "Crypto/Crypto.h"
#include "Crypto/MD.h"

#include "CaveLock.h"


namespace Hap {

	extern Http::Server http;
	extern File::FileConfig<Hap::MyDbType> cfgFile;


	CaveLock::CaveLock(int argc, char* argv[]) {
		Init(Hap::cfg, &Hap::myDb, argc, argv);
	}

	CaveLock::~CaveLock() {
		Terminate();
	}

	bool CaveLock::hasPermission() {
		// (tk) todo, check 
		// return (geteuid() == 0);
		return true;
	}

	int CaveLock::Init(Config* _config, MyDb* _db, int argc, char* argv[]) {
		config = _config;
		db = _db;

		if (!hasPermission()) {
			printf("Please run with device access permission\n");
			exit(1);
		}

		Crypto::rnd_init();

		// (tk) for debugging, no log file created
		Log::Init(LOG_CONSOLE);  // LOG_FILE_PATH

		bool reset = false;
		CLI::App app{ "HAP server" };
		app.add_flag("-D,--debug", Log::Debug, "Turn on debugging messages");
		app.add_flag("-C,--console", Log::Console, "Print log to console");
		app.add_flag("-R,--reset", reset, "Reset configuration");
		CLI11_PARSE(app, argc, argv);

		// setup wiring pi lib
		wiringPiSetup();

		return 0;
	}

	void CaveLock::Terminate() {
		
		// (tk) no need it
		//delete config;
		//delete db;
	}

	void CaveLock::Run(bool reset_config) {

		Hap::Mdns* mdns = Hap::Mdns::Create();
		Hap::Tcp* tcp = Hap::Tcp::Create(&Hap::http);

		// restore configuration
		config->Init(reset_config);

		// set config update callback
		config->Update = [mdns]() -> void {

			bool mdnsUpdate = false;

			// see if status flag must change
			bool paired = Hap::cfgFile.pairings.Count() != 0;
			if (paired && (Hap::cfg->statusFlags & Hap::Bonjour::NotPaired))
			{
				Hap::cfg->statusFlags &= ~Hap::Bonjour::NotPaired;
				mdnsUpdate = true;
			}
			else if (!paired && !(Hap::cfg->statusFlags & Hap::Bonjour::NotPaired))
			{
				Hap::cfg->statusFlags |= Hap::Bonjour::NotPaired;
				mdnsUpdate = true;
			}

			cfgFile.Save();

			if (mdnsUpdate) {
				mdns->Update();
			}
		};

		myLM.Name(config->name);

		while (run) {

			// start Lock Mechanism
			myLM.Start();

			// init HK db
			db->Init(1);

			// start servers
			mdns->Start();
			tcp->Start();

			// wait for signal, gracefully handle SIGINT and SIGTERM
			signal(SIGINT, [](int s)->void {
				printf("===Interrupted===\n");
				});

			signal(SIGTERM, [](int s)->void {
				printf("===Terminated===\n");
				});
			pause();

			config->Save();

			// stop servers
			tcp->Stop();
			mdns->Stop();

			// stop LB
			myLM.Stop();

			if (restart)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				reset_config = true;	// reset config after restart
				restart = false;
				continue;
			}

			break;
		}

	}

	void CaveLock::Restart() {
		Log::Msg("===RESTART===\n");
		restart = true;
		Terminate();
		kill(getpid(), SIGINT);
	}

	void CaveLock::Start(bool reset_config) {
		run = true;
		Run(reset_config);
	}

	void CaveLock::Stop() {
		run = false;
	}

	void CaveLock::Resume() {
		run = true;
		Run(false);
	}

}
