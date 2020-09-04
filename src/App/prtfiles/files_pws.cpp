#include "files_pws.h"



extern __no_init uint8_t 		fbuff[16384] @ "CCMRAM";
extern __no_init FIL			ufile @ "CCMRAM";
extern __no_init FIL			sfile @ "CCMRAM";


FPWS_HEADER			fpws_header;
FPWS_INFO			fpws_info;
FPWS_PREVIEW		fpws_preview;


uint8_t		FPWS_ReadFileInfo(FIL *file)
{
	uint32_t	rd;
	
	memset(&fpws_header, 0, sizeof(fpws_header));
	memset(&fpws_info, 0, sizeof(fpws_info));
	memset(&fpws_preview, 0, sizeof(fpws_preview));

	// header
	if (f_read(file, &fpws_header, sizeof(fpws_header), &rd) != FR_OK || rd != sizeof(fpws_header))
		return 0;
	if (strcmp(fpws_header.mark, (char*)"ANYCUBIC") != 0)
		return 0;
	if (fpws_header.version != 1)
		return 0;
	
	// info
	if (f_lseek(file, fpws_header.info_offset) != FR_OK)
		return 0;
	if (f_read(file, &fpws_info, sizeof(fpws_info), &rd) != FR_OK || rd != sizeof(fpws_info))
		return 0;
	if (strcmp(fpws_info.mark, (char*)"HEADER") != 0)
		return 0;
	
	// preview
	if (f_lseek(file, fpws_header.preview_offset) != FR_OK)
		return 0;
	if (f_read(file, &fpws_preview, sizeof(fpws_preview), &rd) != FR_OK || rd != sizeof(fpws_preview))
		return 0;
	if (strcmp(fpws_preview.mark, (char*)"PREVIEW") != 0)
		return 0;

	
	return 1;
}
//==============================================================================




uint8_t		FPWS_SetPointerToPreview(FIL *file)
{
	if (fpws_header.mark[0] == 0)
		return 0;
	
	if (f_lseek(file, fpws_header.preview_offset) != FR_OK)
		return 0;

	return 1;
}
//==============================================================================




uint32_t	FPWS_GetPreviewDataOffset()
{
	if (fpws_header.mark[0] == 0)
		return 0;
	
	return fpws_header.preview_offset + sizeof(FPWS_PREVIEW);
}
//==============================================================================




uint16_t	FPWS_GetPreviewWidth()
{
	return fpws_preview.width;
}
//==============================================================================




uint16_t	FPWS_GetPreviewHeight()
{
	return fpws_preview.height;
}
//==============================================================================




uint8_t		FPWS_DrawPreview(FIL *file, TG_RECT *rect)
{
	if (fpws_header.mark[0] == 0)
		return 0;

	uint16_t		pw = 0, ph = 0, rw = 0, rh = 0, iw = 0, ih = 0, ix = 0, iy = 0;
	float			pscale = 0, nextcol = 0, nextline = 0;
	uint32_t		cpainted = 0;
	uint32_t		lpainted = 0;
	uint32_t		rd = 0;
	uint16_t		*buff;
	uint32_t		doffset = FPWS_GetPreviewDataOffset();
	uint16_t		dbuff[480];
	uint32_t		bufpos = 0, oldbufpos = 0, oldline = 0, btoread = 0;


	pw = FPWS_GetPreviewWidth();
	ph = FPWS_GetPreviewHeight();
	// read by full lines of source preview image
	btoread = sizeof(fbuff) / ( pw * 2) * pw;

	if (pw == 0 || ph == 0)
		return 0;
	
	rw = rect->right - rect->left + 1;
	rh = rect->bottom - rect->top + 1;
	pscale = (float)pw / (float)rw;
	if ((float)ph / (float)rh > pscale)
		pscale = (float)ph / (float)rh;
//	if (pscale < 1)
//		pscale = 1;
	
	iw = (uint32_t)((float)pw / pscale);
	ih = (uint32_t)((float)ph / pscale);
	ix = rect->left + (rw - iw) / 2;
	iy = rect->top + (rh - ih) / 2;

	buff = (uint16_t*)fbuff;

	LCD_SetWindows(ix, iy, iw, ih);
	LCD_WriteRAM_Prepare();

	if (f_lseek(file, doffset) != FR_OK)
		return 0;
	if (f_read(file, fbuff, btoread, &rd) != FR_OK || rd != btoread)
		return 0;

	bufpos = (uint32_t)(nextcol);
	while (lpainted < ih)
	{
		while (cpainted < iw)
		{
			if (bufpos >= btoread / 2)
			{
				if (f_read(file, fbuff, btoread, &rd) != FR_OK)
					return 0;
				bufpos = 0;
				oldbufpos = bufpos;
				nextcol = 0;
			}
			dbuff[cpainted] = buff[bufpos];
//			LCD_WriteRAM(buff[(uint32_t)(nextcol)]);
			nextcol += pscale;
			cpainted++;
			bufpos = oldbufpos + (uint32_t)(nextcol);
		}
		LCD_WriteRAM_DMA(dbuff, cpainted);
		nextcol = 0;
		nextline += pscale;
		cpainted = 0;
		lpainted++;
		if (((uint32_t)nextline - oldline) > 1)
			bufpos += ((uint32_t)nextline - oldline) * pw;
		if (((uint32_t)nextline - oldline) == 0)
			bufpos -= pw;
		if (bufpos % pw)
			bufpos -= (bufpos % pw);
		oldline = (uint32_t)nextline;
		oldbufpos = bufpos;
	}


	return 1;
}
//==============================================================================




