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

#include "FileConfig.h"

// Hap configuration file
namespace Hap {

	namespace File {

		class Keys;
		class Pairings;

		// convert bin to hex, sizeof(s) must be >= size*2 + 1
		void bin2hex(uint8_t* buf, size_t size, char* s)
		{
			static const char h[] = "0123456789ABCDEF";

			while (size-- > 0)
			{
				uint8_t b = *buf++;

				*s++ = h[b >> 4];
				*s++ = h[b & 0xF];
			}
			*s++ = 0;
		}

		void hex2bin(const char* s, uint8_t* buf, size_t size)
		{
			uint8_t b;

			while (size-- > 0)
			{
				char c = *s++;
				if (c >= '0' && c <= '9')
					b = c - '0';
				else
					b = c - 'A' + 10;
				b <<= 4;

				c = *s++;
				if (c >= '0' && c <= '9')
					b += c - '0';
				else
					b += c - 'A' + 10;

				*buf++ = b;
			}
		}


		void Pairings::Reset()
		{
			init();
		}

		bool Pairings::Save(FILE* f)
		{
			if (f == NULL)
				return false;

			char* key = new char[Hap::Controller::KeyLen * 2 + 1];

			bool comma = false;
			for (unsigned i = 0; i < sizeofarr(_db); i++)
			{
				Hap::Controller* ios = &_db[i];

				if (ios->perm == Hap::Controller::None)
					continue;

				bin2hex(ios->key, ios->KeyLen, key);

				fprintf(f, "\t\t%c[\"%.*s\",\"%s\",\"%d\"]\n", comma ? ',' : ' ', ios->idLen, ios->id, key, ios->perm);
				comma = true;
			}

			delete[] key;

			return true;
		}

		bool Pairings::Add(const char* id, int id_len, const char* key, int key_len, uint8_t perm)
		{
			if (key_len != Hap::Controller::KeyLen * 2)
				return false;

			uint8_t* key_bin = new uint8_t[Hap::Controller::KeyLen];
			hex2bin(key, key_bin, Hap::Controller::KeyLen);

			bool ret = Hap::Pairings::Add((const uint8_t*)id, id_len, key_bin, Hap::Controller::Perm(perm));

			delete[] key_bin;

			return ret;
		}



		void Keys::Reset()
		{
			init();
		}

		bool Keys::Restore(const char* pub, int pub_len, const char* prv, int prv_len)
		{
			if (pub_len != PUBKEY_SIZE_BYTES * 2)
				return false;
			if (prv_len != PRVKEY_SIZE_BYTES * 2)
				return false;

			hex2bin(pub, _pubKey, PUBKEY_SIZE_BYTES);
			hex2bin(prv, _prvKey, PRVKEY_SIZE_BYTES);

			return true;
		}

		bool Keys::Save(FILE* f)
		{
			if (f == NULL)
				return false;

			char* s = new char[PRVKEY_SIZE_BYTES * 2 + 1];

			bin2hex(_pubKey, PUBKEY_SIZE_BYTES, s);
			fprintf(f, "\t\t \"%s\"\n", s);

			bin2hex(_prvKey, PRVKEY_SIZE_BYTES, s);
			fprintf(f, "\t\t,\"%s\"\n", s);

			delete[] s;

			return true;
		}

	}
}

