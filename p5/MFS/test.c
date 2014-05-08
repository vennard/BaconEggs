#include <stdio.h>
#include "mfs.h"

int main (int argv, char *argc[])
{
	int rc = MFS_Init("mumble-19.cs.wisc.edu", 10021);	
	printf("MFS_Init ran and returned %d.\n", rc);
	return 0;
}
