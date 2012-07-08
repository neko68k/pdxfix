// pdxfix.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <map>
#include <vector>
#include <utility>

FILE *pdxlist = NULL;
FILE *mdxlist = NULL;
FILE *otherlist = NULL;
FILE *pdxmisslist = NULL;

using namespace std;

map<unsigned int, _TCHAR*> pdxfiles;
map<unsigned int, _TCHAR*>::iterator pdxiter;


unsigned int crc_tab[256];
unsigned int count = 0;
unsigned int mdxcount = 0;
unsigned int otherfilescount = 0;
unsigned int missingcount = 0;
unsigned int replacedcount = 0;
unsigned int totalpdxcount = 0;

/* chksum_crc() -- to a given block, this one calculates the
 *                crc32-checksum until the length is
 *                reached. the crc32-checksum will be
 *                the result.
 */
unsigned int chksum_crc32 (unsigned char *block, unsigned int length)
{
   register unsigned long crc;
   unsigned long i;

   crc = 0xFFFFFFFF;
   for (i = 0; i < length; i++)
   {
      crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
   }
   return (crc ^ 0xFFFFFFFF);
}

/* chksum_crc32gentab() --      to a global crc_tab[256], this one will
 *                calculate the crcTable for crc32-checksums.
 *                it is generated to the polynom [..]
 */

void chksum_crc32gentab ()
{
   unsigned long crc, poly;
   int i, j;

   poly = 0xEDB88320L;
   for (i = 0; i < 256; i++)
   {
      crc = i;
      for (j = 8; j > 0; j--)
      {
     if (crc & 1)
     {
        crc = (crc >> 1) ^ poly;
     }
     else
     {
        crc >>= 1;
     }
      }
      crc_tab[i] = crc;
   }
}

unsigned int calcCRC(_TCHAR *fn){
	BYTE *crcbuf;
	
	struct _stat stat;
	_wstat(fn, &stat);
	int filesize = stat.st_size;
	FILE *in = _tfopen(fn, L"rb");
	crcbuf = (BYTE*)malloc(filesize);
	fread(crcbuf, 1, filesize, in);
	fclose(in);
	return  chksum_crc32(crcbuf, filesize);
}

unsigned int calcCRC(BYTE *crcbuf, int filesize){
	return  chksum_crc32(crcbuf, filesize);
}

void findPDX(_TCHAR *path, char *pdxfn1){
	_TCHAR *ext = NULL;
	_TCHAR fn[MAX_PATH];
	_TCHAR pdxfn[MAX_PATH];
	_TCHAR *srcpdxfn;
	FILE *in = NULL;
	FILE *out = NULL;
	BYTE *buffer = NULL;
	bool flag;

	memset(fn, 0, MAX_PATH);
	memset(pdxfn, 0, MAX_PATH);

	mbstowcs(pdxfn, pdxfn1, MAX_PATH);
	struct _stat stat;
	if((ext=wcsrchr(pdxfn, L'.'))==NULL)
		wcscat(pdxfn, L".pdx");
	wsprintf(fn, L"%s\\%s", path, pdxfn);
	if(_wstat(fn, &stat)==-1){
		pdxiter = pdxfiles.begin();
		for(pdxiter;pdxiter!=pdxfiles.end();pdxiter++){
			srcpdxfn = wcsrchr(pdxiter->second, '\\')+1;
			if(!lstrcmpiW(srcpdxfn, pdxfn)){
				_wstat(pdxiter->second, &stat);
				buffer = (BYTE*)malloc(stat.st_size);
				in = _tfopen(pdxiter->second, L"rb");
				out = _tfopen(fn, L"wb");
				fread(buffer, 1, stat.st_size, in);
				fwrite(buffer, 1, stat.st_size, out);
				fclose(in);
				fclose(out);
				free(buffer);
				replacedcount++;
				flag = true;
			}			
		}
		if(flag){
			flag=false;
			fwprintf(pdxmisslist, L"%s\n", fn);
		}
		missingcount++;
	}	
	totalpdxcount++;
}

void mdxcallback(_TCHAR* fn){	
	_TCHAR *ext;
	unsigned char c=0;
	unsigned int i=0;
	int k = 0;
	char *title;	
	char *pdx;
	title = (char*)malloc(MAX_PATH);
	pdx = (char*)malloc(MAX_PATH);
	memset(pdx, 0, MAX_PATH);
	memset(title, 0, MAX_PATH);
	ext=wcsrchr(fn, '.');
	if(!lstrcmpiW(L".MDX", ext)){
		FILE *in = _wfopen(fn, L"rb");
		c = fgetc(in);
		while(c!=0x00){
			title[i]=c;
			i++;
			c=fgetc(in);
			if(title[0]==0x0D&&title[1]==0x0A){
				c=0;
				i=0;
				title[0]=0;
				title[1]=0;
			}
		}
		i++;
		fclose(in);
		//printf(title);
		bool flag = false;
		for(unsigned int j = 0;j<i;j++){
			if(title[j]==0x1A&&title[j-1]==0x0A&&title[j-2]==0x0D){
				title[j]=0;
				title[j-1]=0;
				title[j-2]=0;
				flag = true;
			} else {
				if(flag)
				{
					pdx[k]=title[j];
					k++;
				}
			}			
		}
		if(pdx[0]!=0){
			_TCHAR out[MAX_PATH];
			_TCHAR out2[MAX_PATH];
			mbstowcs(out, pdx, MAX_PATH);
			mbstowcs(out2, title, MAX_PATH);
			fwprintf(mdxlist, L"%s\n \t%s\n \t%s\n\n",fn,out2, out);
			_TCHAR *path = wcsrchr(fn, '\\');
			path[0]=0;
			findPDX(fn, pdx);
		}
	}	
	mdxcount++;
}

void pdxcallback(_TCHAR* fn){	
	
	_TCHAR *ext;
	if((ext=wcsrchr(fn, '.'))!=NULL){
		if(!lstrcmpiW(L".pdx", ext)){
			pdxfiles.insert(pair<unsigned int, _TCHAR*>(calcCRC(fn),fn));
			count++;
			return;
		}		
		else if(lstrcmpiW(L".mdx", ext)&&lstrcmpiW(L".pdx", ext)){
			fwprintf(otherlist, L"%s\n", fn);
			otherfilescount++;
		}
	}
}





void wtf(_TCHAR *path, _TCHAR *wildcard, bool recursive, void (*callback)(_TCHAR*)){
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	_TCHAR fn[MAX_PATH];
	
	_TCHAR *recpath;
	
	wsprintf(fn, L"%s\\*.%s", path, wildcard);
	hFind = FindFirstFile(fn, &FindFileData);
	do{
		if(wcscmp(L"..", FindFileData.cFileName)&&wcscmp(L".", FindFileData.cFileName)){
			recpath = (_TCHAR*)malloc(MAX_PATH);
			wsprintf(recpath, L"%s\\%s", path, FindFileData.cFileName);
			if(recursive&&(FindFileData.dwFileAttributes&0x10)){
				wtf(recpath, wildcard, recursive, callback);
				free(recpath);
			} else {
				callback(recpath);
				//free(recpath);
			}
		}
		//memset(recpath, 0, MAX_PATH);
		
	}while(FindNextFile(hFind, &FindFileData)!=0);
	FindClose(hFind);
}

int _tmain(int argc, _TCHAR* argv[])
{	
	chksum_crc32gentab();
	
	//pdxlist = NULL;
	//mdxlist = NULL;
	otherlist = _tfopen(L"otherfiles.txt", L"w");
	mdxlist = _tfopen(L"mdxfiles.txt", L"w");
	pdxmisslist = _tfopen(L"pdxmissfiles.txt", L"w");

	printf("Searching for unique PDX...\n");
	wtf(L"Z:\\EmuStuff\\GameMusic\\MXDRVCollection\\newsort", L"*", true, pdxcallback);
	printf("Unique PDX: %i/%i\n",pdxfiles.size(), count);
	printf("Other files: %i\n\n", otherfilescount);
	
	printf("Searching for missing PDX...\n");
	wtf(L"Z:\\EmuStuff\\GameMusic\\MXDRVCollection\\newsort", L"*", true, mdxcallback);
	printf("MDX files: %i\n", mdxcount);
	printf("PDX files: %i/%i\n", totalpdxcount, mdxcount);
	printf("Missing PDX: %i/%i\n",missingcount, totalpdxcount);
	printf("Recovered PDX: %i/%i\n",replacedcount, missingcount);

	fclose(otherlist);
	fclose(mdxlist);
	fclose(pdxmisslist);
	system("pause");
	return 0;
}








