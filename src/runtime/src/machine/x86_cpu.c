# include <stdio.h>
# include <string.h>
# include <ctype.h>
/**
 * I don't know how to get the cpu the 
 * current thread is running upon. 
 */
int cpu_current()
{
	return -1;
}

/**
 * Get the total number of cpus on this machine
 */
int cpu_total()
{
	FILE *f;
	char line[1024];
	const char *attrib_name="processor";
	int count=0;

	if ((f=fopen("/proc/cpuinfo", "r"))==NULL)
		return -1;

	while (fgets(line, 1024, f)) {
		if ( (strncmp(line, attrib_name, strlen(attrib_name))==0)
				&& isspace(line[strlen(attrib_name)]) )
			count++;
	}

	if (count==0)
		return -1;

	return count;
}
