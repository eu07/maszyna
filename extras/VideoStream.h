#pragma once

class VideoStream
{
  public:
	/**
	  \brief Get Frame data

	  \param Buffer - Pointer to char pointer 

	  \warning This function alocate some memory after use bufer data you must call FreeFrameBuffer

	  \return If Frame ready and complete return num of bytes in frame buffer else return 0

	  **/
	virtual int GetFrameFromStream(char **Buffer) = 0;
	/**
		\brief Free alocated memory

		\param Buffer - Pointer to buffer
	 **/
	virtual void FreeFrameBuffer(char *Buffer) = 0; 
};