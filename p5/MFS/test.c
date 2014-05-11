#include <stdio.h>
#include <stdlib.h>
#include "mfs.h"

int main (int argc, char *argv[])
{
	if (argc != 3){
		printf("Usage: client <host> <port>\n");
		exit(1);
	}

	char name1[] = "foo.txt";
	char name2[] = "bar.txt";
	char buffer[4096];
	MFS_Stat_t m;
	int error = 0;
	char *host = argv[1];
	int port = atoi(argv[2]);

	//TEST MFS_INIT
	printf("TESTER: About to call MFS_Init().\n");
	int rc = MFS_Init(host, port);	
	printf("TESTER: MFS_Init ran and returned %d. Expected 0.\n", rc);

	//TEST MFS_CREAT
	printf("TESTER: About to call MFS_Creat(). with filename %s.\n", name1);	
	rc = MFS_Creat(0, MFS_REGULAR_FILE, name1);
	printf("TESTER: MFS_Lookup() ran and returned %d. Expected 0.\n", rc);

	//TEST MFS_CREAT
	printf("TESTER: About to call MFS_Creat() with filename %s.\n", name2);	
	rc = MFS_Creat(0, MFS_REGULAR_FILE, name2);
	printf("TESTER: MFS_Lookup() ran and returned %d. Expected 0.\n", rc);

	//TEST MFS_LOOKUP
	printf("TESTER: About to call MFS_Lookup().\n");	
	rc = MFS_Lookup(0, name1);
	printf("TESTER: MFS_Lookup() ran and returned %d. Expected 1.\n", rc);

	//TEST MFS_LOOKUP
	printf("TESTER: About to call MFS_Lookup().\n");	
	rc = MFS_Lookup(0, name2);
	printf("TESTER: MFS_Lookup() ran and returned %d. Expected 2.\n", rc);

	//TEST MFS_STAT
	printf("TESTER: About to call MFS_Stat().\n");	
	rc = MFS_Stat(1, &m);
	printf("TESTER: MFS_Stat() ran and returned %d. Expected 0.\n", rc);
	printf("TESTER: m.type = %d, m.size = %d.\n", m.type, m.size);

	//TEST MFS_STAT
	printf("TESTER: About to call MFS_Stat().\n");	
	rc = MFS_Stat(2, &m);
	printf("TESTER: MFS_Stat() ran and returned %d. Expected 0.\n", rc);
	printf("TESTER: m.type = %d, m.size = %d.\n", m.type, m.size);

	//TEST MFS_WRITE
	printf("TESTER: About to call MFS_Write().\n");
	int i;
	for (i = 0; i < 4096; i++) buffer[i] = i % 255;	
	rc = MFS_Write(1, buffer, 0);
	printf("TESTER: MFS_Write() ran and returned %d. Expected 0.\n", rc);

	//TEST MFS_READ
	printf("TESTER: About to call MFS_Read().\n");	
	rc = MFS_Read(1, buffer, 0);
	printf("TESTER: MFS_Read() ran and returned %d.\n", rc);
	for (i = 0; i < 4096; i++)
	{
		 if (buffer[i] != (i % 255)) error = 1;
	} 
	if (error == 1) printf("MFS_Read() did not get the data it expected.\n");

	//TEST MFS_UNLINK
	printf("TESTER: About to call MFS_Unlink().\n");	
	rc = MFS_Unlink(0, name1);
	printf("TESTER: MFS_Unlink() ran and returned %d. Expected 0.\n", rc);

	//TEST MFS_LOOKUP
	printf("TESTER: About to call MFS_Lookup().\n");	
	rc = MFS_Lookup(0, name1);
	printf("TESTER: MFS_Lookup() ran and returned %d. Expected -1.\n", rc);

	//TEST MFS_SHUTDOWN
	printf("TESTER: About to call MFS_Shutdown().\n");	
	rc = MFS_Shutdown();
	printf("TESTER: MFS_Shutdown() ran and returned %d. Expected 0.\n", rc);

	//ALL DONE!
	return 0;
}
