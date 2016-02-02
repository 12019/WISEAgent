#include "util_path.h"
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

void util_split_path_file(char const *filepath, char* path, char* file)
{
	char* dirc = strdup(filepath);
       char* basec = strdup(filepath);
	char* dir = dirname(dirc);
	char* name = basename(basec);
	strcpy(path, dir);
	strcpy(file, name);
	free(dirc);
	free(basec);
}

void util_path_combine(char* destination, const char* path1, const char* path2)
{
	if(path1 == NULL && path2 == NULL) {
		strcpy(destination, "");;
	}
	else if(path2 == NULL || strlen(path2) == 0) {
		strcpy(destination, path1);
	}
	else if(path1 == NULL || strlen(path1) == 0) {
		strcpy(destination, path2);
	} 
	else {
		char directory_separator[] = {FILE_SEPARATOR, 0};
		const char *last_char, *temp = path1;
		const char *skip_char = path2;
		int append_directory_separator = 0;

		while(*temp != '\0')
		{
			last_char = temp;
			temp++;        
		}
		
		if(strcmp(last_char, directory_separator) != 0) {
			append_directory_separator = 1;
		}
		strcpy(destination, path1);
		if(append_directory_separator)
		{
			if(strncmp(path2, directory_separator, strlen(directory_separator) != 0))
				strcat(destination, directory_separator);
		}
		else
		{
			if(*skip_char == FILE_SEPARATOR)
				skip_char++;   
		}
		strcat(destination, skip_char);
	}
}

int util_module_path_get(char * moudlePath)
{
	int iRet = 0;
	char * lastSlash = NULL;
	char tempPath[MAX_PATH] = {0};
	if(NULL == moudlePath) return iRet;
	
	readlink("/proc/self/exe", tempPath, sizeof(tempPath));

	if( 0 == access( tempPath, F_OK ) )
	{
		lastSlash = strrchr(tempPath, FILE_SEPARATOR);
		if(NULL != lastSlash)
		{
			strncpy(moudlePath, tempPath, lastSlash - tempPath + 1);
			iRet = lastSlash - tempPath + 1;
		}	
	}
	return iRet;
}

bool util_create_directory(char* path)
{
    struct stat st;
    bool status = true;
    mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    if (stat(path, &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path, mode) != 0)
            status = false;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        status = false;
    }

    return(status);
}

unsigned long util_get_temp_path(char* lpBuffer, int nBufferLength)
{
	int len = 0;
	char const *folder = getenv("TMPDIR");
	if (folder == 0)
	{
		len = strlen("/tmp/");
		strncpy(lpBuffer, "/tmp/", strlen("/tmp/")+1);
	}
	else
	{
		len = strlen(folder);
		if(nBufferLength < len+1)
		{
			strncpy(lpBuffer, folder, nBufferLength);
			len = nBufferLength;
		}
		else
			strncpy(lpBuffer, folder, len+1);
	}
	return len;
}