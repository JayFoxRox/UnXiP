/*************************************************************
 *
 * MaTiAz UnXIP - a command line tool for extracting XIPs
 *
 * Based on Voltaic's PIXIT tool(actually this prog is a
 * simplified version Voltaic's PIXIT).
 *
 * You can use this source file whatever way you want, as
 * long as you dont sell it to anyone!! The sources may be a
 * little dizzy for you, but its because im a C++ newbie(this
 * is my first Xbox related application on C++).
 *
 * November 20, 2022 - Added extraction of meshes/models.
 * Removed extraction of .ib and .vb files.
 *
 ************************************************************/

#ifndef _WIN32
#define _MAX_PATH 256
#define mkdir(a) 0
#include <strings.h>
#define stricmp(a,b) strcasecmp(a,b)
#else
#include <direct.h>
#endif

#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 
#include <list>
#include <stdint.h>

#define UNXIP_MAJOR_VER	0
#define UNXIP_MINOR_VER	2

typedef struct _tagXIPHDR
{
	char cbMagic[4];
	uint32_t nDataOffset;
	uint16_t nFiles;
	uint16_t nNames;
	uint32_t nDataSize;
} XIPHDR, *LPXIPHDR;

typedef struct _tagFILEDATA
{
	uint32_t nOffset;
	uint32_t nSize;
	uint32_t nType;
	uint32_t nTimestamp;
} FILEDATA, *LPFILEDATA;

typedef struct _tagFILENAME
{
	uint16_t nDataIndex;
	uint16_t nNameOffset;
} FILENAME, *LPFILENAME;

typedef struct _tagENTRY
{
	char szSrcFilename[_MAX_PATH + 1];
	char szDstFilename[_MAX_PATH + 1];
	FILEDATA fd;
	FILENAME fn;
} ENTRY, *LPENTRY;

void Usage(const char* pszError)
{
	if(pszError != NULL)
	{
		printf("Error '%s' \n", pszError);
		printf("\n");
		printf("\n");
	}
	printf("UnXiP by MaTiAz - based on PIXIT 0.5 sources.\n\n");
	printf("Usage: unxip.exe -extract [unxip-path] [default.xip]");
	printf("\n");
	printf("This app also displays info about your XiP file.");
	printf("\n");

	exit(-1);
}

bool GetCmdLineArg(const char* pszName, int argc, char* argv[])
{
	char* pszArg = NULL, szTemp[256] = "";
	int n, m, l = strlen(pszName);
	for(n = 0; n < argc; n++)
	{
		pszArg = argv[n];
		for(m = 0; ((szTemp[m] = pszArg[m]) != '=') && (szTemp[m] != '\0'); m++);
		szTemp[m] = '\0';
		if(((*pszArg == '-') || (*pszArg == '/')) && (stricmp(szTemp + 1, pszName) == 0))
			return true;
	}
	return false;
}

char* GetCmdLineArgValue(const char* pszName, int argc, char* argv[])
{
	char* pszArg = NULL;
	int n, m, l = strlen(pszName);
	for(n = 0; n < argc; n++)
	{
		pszArg = argv[n];
		if((*pszArg == '-') || (*pszArg == '/'))
		{
			for(m = 0; m < l; m++)
			{
				if(tolower(pszArg[1 + m]) != tolower(pszName[m]))
					break;
			}

			if((m == l) && (pszArg[l + 1] == '='))
				return pszArg + l + 2;
		}
	}
	return NULL;
}

void PrintXipHeader(LPXIPHDR pXipHeader)
{
	printf("Header information:\n\n");
	printf("File ID....:  %.4s\n", pXipHeader->cbMagic);
	printf("Data Offset:  0x%.8X\n", pXipHeader->nDataOffset, pXipHeader->nDataOffset);
	printf("Files......:  %lu\n", pXipHeader->nFiles);
	printf("Data Size..:  0x%.8X (%lu bytes)\n", pXipHeader->nDataSize, pXipHeader->nDataSize);
}

const char* GetFilenameAt(const char* pszFilenameBlock, int nIndex)
{
	const char* pszFilename = pszFilenameBlock;
	for(int n = 0; n < nIndex; n++)
	{
		if(n == nIndex)
			break;
		pszFilename += strlen(pszFilename) + 1;
		if(pszFilename == NULL)
			return NULL;
	}
	return pszFilename;
}

int ExtractArchive(const char* pszExtractFolder, const char* pszArchiveFile)
{
	char szFolder[_MAX_PATH + 1] = "";

	if(pszExtractFolder == NULL)
	{
		strcpy(szFolder, pszArchiveFile);
		strcat(szFolder, ".d");
	}
	else
	{
		strcpy(szFolder, pszExtractFolder);
	}

	int len = strlen(szFolder);
	if(szFolder[len - 1] == '\\' || szFolder[len - 1] == '/')
		szFolder[len - 1] = '\0';

	mkdir(szFolder);

	FILE* fpIn = fopen(pszArchiveFile, "rb");
	if(fpIn == NULL)
	{
		printf("Error opening file '%s'. (errno: %d)\n", pszArchiveFile, errno);
		return -2;
	}

	printf("Extracting files from '%s' to '%s'.\n\n\n", pszArchiveFile, szFolder);

	XIPHDR xh;
	memset(&xh, 0, sizeof(xh));

	int nRead = fread(&xh, 1, sizeof(xh), fpIn);
	if(nRead != sizeof(xh))
	{
		fclose(fpIn);
		printf("Error reading file. (errno: %d)\n", errno);
		return -2;
	}

	if(strncmp(xh.cbMagic, "XIP0", 4) != 0)
	{
		fclose(fpIn);
		printf("Invalid file format.\n");
		return -2;
	}

	PrintXipHeader(&xh);

	printf("\n\n");

	LPFILEDATA pFileData = new FILEDATA[xh.nFiles];
	if(pFileData == NULL)
	{
		fclose(fpIn);
		printf("Out of memory.\n");
		return -2;
	}

	nRead = fread(pFileData, 1, xh.nFiles * sizeof(FILEDATA), fpIn);
	if(nRead != (int)(xh.nFiles * sizeof(FILEDATA)))
	{
		delete [] pFileData;
		fclose(fpIn);
		printf("Error reading file. (errno: %d)\n", errno);
		return -2;
	}

	LPFILENAME pFileName = new FILENAME[xh.nFiles];
	if(pFileName == NULL)
	{
		fclose(fpIn);
		printf("Out of memory.\n");
		return -2;
	}

	nRead = fread(pFileName, 1, xh.nFiles * sizeof(FILENAME), fpIn);
	if(nRead != (int)(xh.nFiles * sizeof(FILENAME)))
	{
		delete [] pFileName;
		delete [] pFileData;
		fclose(fpIn);
		printf("Error reading file. (errno: %d)\n", errno);
		return -2;
	}

	int nFilenameBlockOffset = xh.nNames * sizeof(FILENAME) + xh.nFiles * sizeof(FILEDATA) + sizeof(xh);
	int nFilenameBlockSize = xh.nDataOffset - nFilenameBlockOffset;

	char* pszFilenameBlock = new char[nFilenameBlockSize + 1];
	if(pszFilenameBlock == NULL)
	{
		delete [] pFileName;
		delete [] pFileData;
		fclose(fpIn);
		printf("Out of memory\n");
		return -2;
	}

	nRead = fread(pszFilenameBlock, 1, nFilenameBlockSize, fpIn);
	if(nRead != nFilenameBlockSize)
	{
		delete [] pszFilenameBlock;
		delete [] pFileName;
		delete [] pFileData;
		fclose(fpIn);
		printf("Error reading file. (errno: %d)\n", errno);
		return -2;
	}

	pszFilenameBlock[nFilenameBlockSize] = NULL;

	printf("Extracting files:\n\n");

	for(int n = 0; n < xh.nFiles; n++)
	{
		LPFILEDATA pData = &pFileData[n];

		const char* pszFilename = GetFilenameAt(pszFilenameBlock, n);

		// nType == 5 .ib
		// nType == 6 .vb
		// These two filetypes are unescesarry.
		// Removed nType == 4, as we want the XIP files to be extracted in full. Thus, we want XM (model/mesh) files.
		if(pData->nType == 5)
		{
			printf("Skipping file '%s' ...\n", pszFilename);
			continue;
		}
		if (pData->nType == 6)
		{
			printf("Skipping file '%s' ...\n", pszFilename);
			continue;
		}

		printf("Extracting file '%s' ... ", pszFilename);

		char szFilename[_MAX_PATH + 1];
		sprintf(szFilename, "%s/%s", szFolder, pszFilename);
		FILE* fpOut = fopen(szFilename, "wb");
		if(fpOut == NULL)
		{
			delete [] pszFilenameBlock;
			delete [] pFileName;
			delete [] pFileData;
			fclose(fpIn);
			printf("Error creating output file '%s'. (errno: %d)\n", szFilename, errno);
			return -2;
		}

		fseek(fpIn, xh.nDataOffset + pData->nOffset, SEEK_SET);

		unsigned char cbBuffer[4096];
		unsigned long nTotalBytesWritten = 0L;

		long nBlocks = pData->nSize / sizeof(cbBuffer);
		for(int m = 0; m < nBlocks; m++)
		{
			nRead = fread(cbBuffer, 1, sizeof(cbBuffer), fpIn);
			if(nRead != sizeof(cbBuffer))
			{
				fclose(fpOut);
				delete [] pszFilenameBlock;
				delete [] pFileName;
				delete [] pFileData;
				fclose(fpIn);
				printf("Error reading file. (errno: %d)\n", errno);
				return -2;
			}

			int nWritten = fwrite(cbBuffer, 1, sizeof(cbBuffer), fpOut);
			if(nWritten != sizeof(cbBuffer))
			{
				fclose(fpOut);
				delete [] pszFilenameBlock;
				delete [] pFileName;
				delete [] pFileData;
				fclose(fpIn);
				printf("Error writing file. (errno: %d)\n", errno);
				return -2;
			}

			nTotalBytesWritten += nWritten;
		}
		long nRemainder = pData->nSize % sizeof(cbBuffer);
		if(nRemainder > 0)
		{
			nRead = fread(cbBuffer, 1, nRemainder, fpIn);
			if(nRead != nRemainder)
			{
				fclose(fpOut);
				delete [] pszFilenameBlock;
				delete [] pFileName;
				delete [] pFileData;
				fclose(fpIn);
				printf("Error reading file. (errno: %d)\n", errno);
				return -2;
			}

			int nWritten = fwrite(cbBuffer, 1, nRemainder, fpOut);
			if(nWritten != nRemainder)
			{
				fclose(fpOut);
				delete [] pszFilenameBlock;
				delete [] pFileName;
				delete [] pFileData;
				fclose(fpIn);
				printf("Error writing file. (errno: %d)\n", errno);
				return -2;
			}

			nTotalBytesWritten += nWritten;
		}

		fclose(fpOut);

		printf(" %lu bytes written.\n", nTotalBytesWritten);
	}

	delete [] pFileName;
	delete [] pFileData;
	fclose(fpIn);

	return 0;
}

int main(int argc, char* argv[])
{
	char* pszArchiveFile = "default.xip";

	printf("UnXiP version %d.%d\n", UNXIP_MAJOR_VER, UNXIP_MINOR_VER);
	printf("\n");

	if(argc < 2)
		Usage(NULL);

	if(argv[argc - 1][0] != '-')
		pszArchiveFile = argv[argc - 1];

	if(GetCmdLineArg("extract", argc, argv) != 0)
	{
		char* pszExtractFolder = GetCmdLineArgValue("extract", argc, argv);
		return ExtractArchive(pszExtractFolder, pszArchiveFile);
	}
	
	Usage("Unknown command.");

	return 0;
}
