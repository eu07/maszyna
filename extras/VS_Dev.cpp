#include "VS_Dev.h"
#include <iostream>
#include <fstream>

using namespace std;

VSDev::VSDev()
{
	ifstream DevFile;
	DevFile.open("test01.bin", std::ios_base::binary);
	DevFile.read(this->Frame1TestBuf, sizeof(this->Frame1TestBuf));
	DevFile.close();
	DevFile.open("test02.bin");
	DevFile.read(this->Frame2TestBuf, sizeof(this->Frame2TestBuf));
	DevFile.close();
}

int VSDev::GetFrameFromStream(char ** Buffer)
{
	if ((this->GetFrameCallNum % 2) == 0)
	{
		*Buffer = this->Frame1TestBuf;
		this->GetFrameCallNum++;
		return sizeof(this->Frame1TestBuf);
	}
	else if ((this->GetFrameCallNum % 3) == 0)
	{
		*Buffer = this->Frame2TestBuf;
		this->GetFrameCallNum++;
		return sizeof(this->Frame2TestBuf);
	}
	else
	{
		this->GetFrameCallNum++;
		return 0;
	}
}

void VSDev::FreeFrameBuffer(char * Buffer) {
	//For dev do nothing...
}
