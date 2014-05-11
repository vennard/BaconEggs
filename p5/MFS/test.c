#include <stdio.h>
#include "mfs.h"

int main (int argv, char *argc[])
{
	char name[] = "POOPY";
	printf("TESTER: About to call MFS_Init().\n");
	int rc = MFS_Init("mumble-38.cs.wisc.edu", 10021);	
	printf("TESTER: MFS_Init ran and returned %d.\n", rc);
	rc = MFS_Lookup(1, name);
	printf("TESTER: MFS_Lookup ran and returned %d.\n", rc);	
	return 0;
}
