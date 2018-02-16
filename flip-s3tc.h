// https://github.com/inequation/flip-s3tc
// Leszek Godlewski places this file in the public domain.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
namespace flip_s3tc
{
#endif

#pragma pack(push, 1)

struct dxt1_block
{
	uint16_t	c0, c1;
	uint8_t		dcba,
				hgfe,
				lkji,
				ponm;
};

struct dxt23_block
{
	uint16_t	adacabaa,
				ahagafae,
				alakajai,
				apaoanam;
	uint16_t	c0, c1;
	uint8_t		dcba,
				hgfe,
				lkji,
				ponm;
};

struct dxt45_block
{
	uint8_t		a0, a1;
	struct
	{
		uint8_t	alpha[3];
	}			ahagafaeadacabaa,
				apaoanamalakajai;
	uint16_t	c0, c1;
	uint8_t		dcba,
				hgfe,
				lkji,
				ponm;
};

#pragma pack(pop)

/** Performs an Y-flip of the given DXT1 block in place. */
void flip_dxt1_block(struct dxt1_block *block)
{
	uint8_t 		temp;
	
	temp			= block->dcba;
	block->dcba		= block->ponm;
	block->ponm		= temp;
	temp			= block->hgfe;
	block->hgfe		= block->lkji;
	block->lkji 	= temp;
}

/** Performs an Y-flip of the given DXT2/DXT3 block in place. */
void flip_dxt23_block(struct dxt23_block *block)
{
	uint8_t			temp8;
	uint16_t		temp16;
	
	temp16			= block->adacabaa;
	block->adacabaa	= block->apaoanam;
	block->apaoanam	= temp16;
	temp16			= block->ahagafae;
	block->ahagafae	= block->alakajai;
	block->alakajai	= temp16;
	
	temp8			= block->dcba;
	block->dcba		= block->ponm;
	block->ponm		= temp8;
	temp8			= block->hgfe;
	block->hgfe		= block->lkji;
	block->lkji 	= temp8;
}

/** Performs an Y-flip of the given DXT4/DXT5 block in place. */
void flip_dxt45_block(struct dxt45_block *block)
{
	uint8_t 		temp8;
	uint32_t		temp32;
	uint32_t		*as_int[2];
	
	as_int[0]		= (uint32_t *)block->ahagafaeadacabaa.alpha;
	as_int[1]		= (uint32_t *)block->apaoanamalakajai.alpha;
	// swap adacabaa with apaoanam
	temp32			= *as_int[0] & ((1 << 12) - 1);
	*as_int[0]		&= ~((1 << 12) - 1);
	*as_int[0]		|= (*as_int[1] & (((1 << 12) - 1) << 12)) >> 12;
	*as_int[1]		&= ~(((1 << 12) - 1) << 12);
	*as_int[1]		|= temp32 << 12;
	// swap ahagafae with alakajai
	temp32			= *as_int[0] & (((1 << 12) - 1) << 12);
	*as_int[0]		&= ~(((1 << 12) - 1) << 12);
	*as_int[0]		|= (*as_int[1] & ((1 << 12) - 1)) << 12;
	*as_int[1]		&= ~((1 << 12) - 1);
	*as_int[1]		|= temp32 >> 12;
	
	temp8			= block->dcba;
	block->dcba		= block->ponm;
	block->ponm		= temp8;
	temp8			= block->hgfe;
	block->hgfe		= block->lkji;
	block->lkji 	= temp8;
}

/**
 * Performs an Y-flip on the given DXT1 image in place.
 * @param	data	buffer with image data (S3TC blocks)
 * @param	width	image width in pixels
 * @param	height	image height in pixels
 */
void flip_dxt1_image(void *data, int width, int height)
{
	int x, y;
	struct dxt1_block temp1, temp2;
	struct dxt1_block *blocks = (struct dxt1_block *)data;
	
	width	= (width	+ 3) / 4;
	height	= (height	+ 3) / 4;
	
	for (y = 0; y < height / 2; ++y)
	{
		for (x = 0; x < width; ++x)
		{
			temp1						= blocks[y * width + x];
			temp2						= blocks[(height - y - 1) * width + x];
			flip_dxt1_block(&temp1);
			flip_dxt1_block(&temp2);
			blocks[(height - y - 1) * width + x]	= temp1;
			blocks[y * width + x]					= temp2;
		}
	}
}

/**
 * Performs an Y-flip on the given DXT2/DXT3 image in place.
 * @param	data	buffer with image data (S3TC blocks)
 * @param	width	image width in pixels
 * @param	height	image height in pixels
 */
void flip_dxt23_image(void *data, int width, int height)
{
	int x, y;
	struct dxt23_block temp1, temp2;
	struct dxt23_block *blocks = (struct dxt23_block *)data;
	
	width	= (width	+ 3) / 4;
	height	= (height	+ 3) / 4;
	
	for (y = 0; y < height / 2; ++y)
	{
		for (x = 0; x < width; ++x)
		{
			temp1						= blocks[y * width + x];
			temp2						= blocks[(height - y - 1) * width + x];
			flip_dxt23_block(&temp1);
			flip_dxt23_block(&temp2);
			blocks[(height - y - 1) * width + x]	= temp1;
			blocks[y * width + x]					= temp2;
		}
	}
}

/**
 * Performs an Y-flip on the given DXT1 image in place.
 * @param	data	buffer with image data (S3TC blocks)
 * @param	width	image width in pixels
 * @param	height	image height in pixels
 */
void flip_dxt45_image(void *data, int width, int height)
{
	int x, y;
	struct dxt45_block temp1, temp2;
	struct dxt45_block *blocks = (struct dxt45_block *)data;
	
	width	= (width	+ 3) / 4;
	height	= (height	+ 3) / 4;
	
	for (y = 0; y < height / 2; ++y)
	{
		for (x = 0; x < width; ++x)
		{
			temp1						= blocks[y * width + x];
			temp2						= blocks[(height - y - 1) * width + x];
			flip_dxt45_block(&temp1);
			flip_dxt45_block(&temp2);
			blocks[(height - y - 1) * width + x]	= temp1;
			blocks[y * width + x]					= temp2;
		}
	}
}

#if __cplusplus
}	// namespace flip_s3tc
#endif
