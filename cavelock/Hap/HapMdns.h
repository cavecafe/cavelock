/*
MIT License

Copyright (c) 2019 Gera Kazakov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _HAP_MDNS_H_
#define _HAP_MDNS_H_

// Interface to system-dependent MDNS implementation

namespace Hap
{
	class Mdns
	{
	protected:
		char _name[64];			// service name - initially set to _cfg.name
		char _txt[128];			// txt buffer
		uint8_t _txt_len = 0;

		void update()
		{
			char* p = _txt;
			int max = sizeof(_txt) - 1;

			char l;
			
			if (!Hap::cfg->BCT)
			{
				strcpy(_name, Hap::cfg->name);
			}
			else
			{
				strcpy(_name, "New - Bonjour Service Name");
			}

			// c# - current Configuration number
			//		updates only when accessory/service/characteristic is added or removed
			l = snprintf(p + 1, max,  "c#=%d", Hap::cfg->configNum);
			max -= l;
			if (max <= 0) goto Ret;
			*p++ = l;
			p += l;

			// ff - feature flags
			//		1 - supports HAP pairing
			l = snprintf(p + 1, max, "ff=0");
			max -= l;
			if (max <= 0) goto Ret;
			*p++ = l;
			p += l;

			// id - Device ID (randomly generated on each device reset)
			l = snprintf(p + 1, max, "id=%s", Hap::cfg->deviceId);
			max -= l;
			if (max <= 0) goto Ret;
			*p++ = l;
			p += l;

			// md - Model name
			l = snprintf(p + 1, max, "md=%s", Hap::cfg->model);
			max -= l;
			if (max <= 0) goto Ret;
			*p++ = l;
			p += l;

			// pv - protocol version, requred if not 1.0
			l = snprintf(p + 1, max, "pv=1.0");
			max -= l;
			if (max <= 0) goto Ret;
			*p++ = l;
			p += l;

			// s# - current state number, must be 1
			l = snprintf(p + 1, max, "s#=1");
			max -= l;
			if (max <= 0) goto Ret;
			*p++ = l;
			p += l;

			// statusFlags - status flags
			l = snprintf(p + 1, max, "sf=%u", Hap::cfg->statusFlags);
			max -= l;
			if (max <= 0) goto Ret;
			*p++ = l;
			p += l;

			// category identifier
			l = snprintf(p + 1, max, "ci=%d", Hap::cfg->categoryId);
			max -= l;
			if (max <= 0) goto Ret;
			*p++ = l;
			p += l;
		
		Ret:
			_txt_len = p - _txt;
		}
	
	public:
		static Mdns* Create();
		virtual void Start() = 0;
		virtual void Stop() = 0;
		virtual void Update() = 0;
	};
}

#endif
