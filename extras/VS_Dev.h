#pragma once
#include "VideoStream.h"

class VSDev : public VideoStream
{
  public:
	VSDev();
	int GetFrameFromStream(char **Buffer);
	void FreeFrameBuffer(char *Buffer);

  private:
	int GetFrameCallNum = 0;
	char Frame1TestBuf[102737];
	char Frame2TestBuf[104143];
};