// NanoDesignTask.cpp
#include <iostream>
#include <fstream>
#include <climits>
#include <cstring>

using namespace std;

unsigned int static g_randNumber = 0;

class OtherFunctions
{
public:
	unsigned int static myRandom() {
		g_randNumber = g_randNumber * 1103515245 + 12345;
		return (g_randNumber % UINT_MAX);
	}

	uint16_t static crc16Modbus(uint8_t* message, int len) {
		const uint16_t generator = 0xa001;
		uint16_t crc16 = 0xffff;
		for (int i = 0; i < len; ++i) {
			crc16 ^= (uint16_t)message[i];
			for (int a = 0; a < 8; ++a) {
				if ((crc16 & 1) != 0) {
					crc16 >>= 1;
					crc16 ^= generator;
				}
				else
					crc16 >>= 1;
			}
		}
		return crc16;
	}
};

class BinaryOperation
{
public:
	int static getParsedBinary(uint8_t byte) {
		int count = 0;
		while (byte) {
			count += byte & 1;
			byte >>= 1;
		}
		return count;
	}

	uint16_t static joinShiftRight1000(uint8_t byte0, uint8_t byte1) {

		//join two bytes
		uint16_t result = ((byte0 << 8) + byte1);

		//shift right by 1000 places, same as right shift by 8 (1000 & 16 = 8)
		result >>= 1000;

		return result;
	}
};

int main(int argc, char* argv[]) {

	//amd64 uses 64 bit wide registers with little endian (reverse in memory)
	int amd64Flag = 0;

	//armv7e uses 32 bit wide registers with little endian (reverse in memory)
	int armv7eFlag = 0;

	if (strcmp(argv[1], "amd64") == 0)
		amd64Flag = 1;
	if (strcmp(argv[1], "armv7e") == 0)
		armv7eFlag = 1;

	BinaryOperation binaryOperation;
	OtherFunctions otherFunctions;
	int buffLen = 400;

	//create 400 int long buffer
	unsigned int* buff = new unsigned int[buffLen];
	if (buff == nullptr) {
		std::cout << "Error: memory allocation failed!" << std::endl;
		return 1;
	}

	//fill it with pseudorandom numbers
	for (int i = 0; i < buffLen; i++) {
		buff[i] = otherFunctions.myRandom();
	}

	//create file
	ofstream wf("output", ios::out | ios::binary);
	if (!wf) {
		std::cout << "Error: cannot create file!" << std::endl;
		return 1;
	}

	for (int i = 0; i < buffLen; i++) {
		//create array to store 4 bytes of single integer (in buff)
		uint8_t intBytes[4] = { 0 };

		//split integer into 4 bytes
		for (int a = 0; a < 4; a++) {
			intBytes[a] = (buff[i] >> (8 * (3 - a)));
		}

		//create buffer, which will store output for 1 integer
		uint8_t fileBuffer[28] = { 0 };

		//iterate 4 bytes and create output according to assignment
		for (int a = 0; a < 4; a += 2) {
			uint16_t output2 = binaryOperation.joinShiftRight1000(intBytes[a], intBytes[a + 1]);

			 
			fileBuffer[0 + a * 7] = (uint8_t)0xaa;
			fileBuffer[1 + a * 7] = (uint8_t)0xbb;
			fileBuffer[2 + a * 7] = (uint8_t)0x01;
			//first/third byte, function1
			fileBuffer[3 + a * 7] = (uint8_t)intBytes[a];
			fileBuffer[4 + a * 7] = (uint8_t)0xff;
			//first/third byte, function1, output
			fileBuffer[5 + a * 7] = (uint8_t)binaryOperation.getParsedBinary(intBytes[a]);
			fileBuffer[6 + a * 7] = (uint8_t)0x02;

			//second/forth byte, function1
			fileBuffer[7 + a * 7] = (uint8_t)intBytes[a + 1];
			//first/third byte, function2
			fileBuffer[8 + a * 7] = (uint8_t)intBytes[a];
			//second/forth byte, function2
			fileBuffer[9 + a * 7] = (uint8_t)intBytes[a + 1];
			fileBuffer[10 + a * 7] = (uint8_t)0xff;
			//second/forth byte, function1, output
			fileBuffer[11 + a * 7] = (uint8_t)binaryOperation.getParsedBinary(intBytes[a + 1]);
			//function2, first byte of output (from left)
			fileBuffer[12 + a * 7] = (uint8_t)(output2 >> 8);
			//function2, second byte of output
			fileBuffer[13 + a * 7] = (uint8_t)output2;

		}

		
		if (armv7eFlag) {
			//create buffer where every 4 bytes are reversed
			uint8_t litEnd32[28] = { 0 };
			for (int i = 0; i < sizeof(fileBuffer); i += 4) {
				litEnd32[i] = fileBuffer[i + 3];
				litEnd32[i + 1] = fileBuffer[i + 2];
				litEnd32[i + 2] = fileBuffer[i + 1];
				litEnd32[i + 3] = fileBuffer[i];
			}
			//write buffer to file
			wf.write((char*)litEnd32, sizeof(litEnd32));
			//calculate crc16 modus from litEnd32
			uint16_t crc16 = otherFunctions.crc16Modbus(litEnd32, 28);

			//write crc16 into file (as 2 bytes)
			wf << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)crc16 << (uint8_t)(crc16 >> 8);
		}

		if (amd64Flag) {
			//create buffer where every 8 bytes are reversed
			uint8_t litEnd64[32] = { 0 };
			for (int i = 0; i < sizeof(fileBuffer); i += 8) {
				litEnd64[i + 4] = fileBuffer[i + 3];
				litEnd64[i + 5] = fileBuffer[i + 2];
				litEnd64[i + 6] = fileBuffer[i + 1];
				litEnd64[i + 7] = fileBuffer[i];

				//stop after fileBuffer ends
				if ((i + 4) >= sizeof(fileBuffer)) {
					break;
				}

				litEnd64[i] = fileBuffer[i + 7];
				litEnd64[i + 1] = fileBuffer[i + 6];
				litEnd64[i + 2] = fileBuffer[i + 5];
				litEnd64[i + 3] = fileBuffer[i + 4];
			}

			//write buffer to file
			wf.write((char*)litEnd64, sizeof(litEnd64));
			//calculate crc16 modus from litEnd64
			uint16_t crc16 = otherFunctions.crc16Modbus(litEnd64, 32);

			//write crc16 into file (as 2 bytes)
			wf << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)crc16 << (uint8_t)(crc16 >> 8);
		}
	}
	wf.close();
}