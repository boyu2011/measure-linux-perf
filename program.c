//
//	Program.c 
//
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <linux/errno.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#define FORK_CREATION_COUNT 10
#define CONTEXT_SWITCH_COUNT 10
#define THREAD_CREATION_COUNT 10
#define PROC_SWITCH_COUNT 10
#define THREAD_SWITCH_COUNT 10
#define IO_COUNT 10

//#define DEBUG

enum PerfType 
{ 
	ContextSwitchTime, 
	ProcCreationTime, 
	ProcSwitchTime, 
	ThreadCreationTime, 
	ThreadSwitchTime, 
	DiskBandwithIoTime,
	ReadFileTime,
	WriteFileTime 
};

struct timeval s_tvStartTimeForThreadCreation;
struct timeval s_tvEndTimeForThreadCreation;
struct timeval s_tvStartTimeForProcSwitch;
struct timeval s_tvEndTimeForProcSwitch;

int s_iContextSwitchTimeUnitCountArr [FORK_CREATION_COUNT];
int s_iProcCreationTimeUnitCountArr [FORK_CREATION_COUNT];
int s_iProcSwitchTimeUnitCountArr [FORK_CREATION_COUNT];
int s_iThreadCreationTimeUnitCountArr [THREAD_CREATION_COUNT];
int s_iThreadSwitchTimeUnitCountArr [THREAD_SWITCH_COUNT];
int s_iReadFileTimeUnitCountArr [FORK_CREATION_COUNT];
int s_iWriteFileTimeUnitCountArr [FORK_CREATION_COUNT];

double ConvertTimevalToSecond ( struct timeval tvTime );
void PrintTime ( enum PerfType perfType, double dTime );
void EstimateProcCreationTime ();
void EstimateContextSwitchTime ();
void EstimateThreadCreationTime ();
void * ThreadFunc ( void * arg );
void EstimateProcSwitchTime ();
void EstimateThreadSwitchTime ();
void * ThreadFuncForSwitch ();
void EstimateDiskBandwithIoTime ();
void SetTimeUnitCount ( enum PerfType perfType, int iCount, double dTime );
void PrintDiagram ( enum PerfType perfType, int iCount );


int main()
{
	EstimateContextSwitchTime ();	

	EstimateProcCreationTime ();
	
	EstimateProcSwitchTime ();

	EstimateThreadCreationTime ();

	EstimateThreadSwitchTime ();

	EstimateDiskBandwithIoTime ();

	return 0;
}


void EstimateContextSwitchTime ()
{
	struct timeval tvStartTime;
	struct timeval tvEndTime;
	int iSwitchCount;
	double dStartTime;
	double dEndTime;
	double dTotalTime;

	printf ( "---------------------------------------------------------\n" );
	printf ( "    Estimate Context Switch Time\n" );
	printf ( "---------------------------------------------------------\n\n" );

	for ( iSwitchCount = 0; iSwitchCount < CONTEXT_SWITCH_COUNT; iSwitchCount++ )
	{
		printf ( "Estimate the context switch time for the %dth time\n", iSwitchCount+1 );
		
		// start the timer
		gettimeofday ( & tvStartTime, NULL );

		// invoke into the kernel and execute the context switch for two times ( in & out )
		sleep ( 0 );

		// stop the timer and print the estimation
		gettimeofday ( & tvEndTime, NULL );
	
		dStartTime = ConvertTimevalToSecond ( tvStartTime );
		dEndTime = ConvertTimevalToSecond ( tvEndTime );	
		// get the context switch time only once
		dTotalTime = ( double ) ( ( double ) ( dEndTime - dStartTime ) / ( double ) 2 );
		
		#ifdef DEBUG		
		printf ( "EstimateContextSwitchTime() ---- dStartTime = %f\n", dStartTime );
		printf ( "EstimateContextSwitchTime() ---- dEndTime = %f\n", dEndTime );
		printf ( "EstimateContextSwitchTime() ---- dTotalTime = %f\n", dTotalTime );
		#endif

		PrintTime ( ContextSwitchTime, dTotalTime );

		// pre-work for print the diagram
		SetTimeUnitCount ( ContextSwitchTime, iSwitchCount, dTotalTime );
	}

	PrintDiagram ( ContextSwitchTime, CONTEXT_SWITCH_COUNT );
}


void EstimateProcCreationTime ()
{	
	struct timeval tvStart;
	struct timeval tvEnd;
	double dStartTime;
	double dEndTime;
	double dTotalTime;
	pid_t pid;
	int iForkCount;

	printf ( "---------------------------------------------------------\n" );
	printf ( "    Estimate Process Creation Time\n" );
	printf ( "---------------------------------------------------------\n\n" );

	for ( iForkCount = 0; iForkCount < FORK_CREATION_COUNT; iForkCount++ )	
	{
		// start the timer
		gettimeofday ( & tvStart, NULL );
		
		// create a new process
		pid = fork();	

		if ( pid < 0 )
		{	
			printf ( "Fork failed!\n" );
		}
		else if ( pid == 0 ) // Child process
		{
			// stop the timer
			gettimeofday ( & tvEnd, NULL );
			
			printf ( "Create a child process for the %dst time.\n", iForkCount+1 );
			dStartTime = ConvertTimevalToSecond ( tvStart );
			dEndTime = ConvertTimevalToSecond ( tvEnd );
			dTotalTime = dEndTime - dStartTime;
			PrintTime ( ProcCreationTime, dTotalTime );
			SetTimeUnitCount ( ProcCreationTime, iForkCount, dTotalTime );
			
			if ( iForkCount == 9 )
			PrintDiagram ( ProcCreationTime, FORK_CREATION_COUNT );
		}
		else // parent process
		{
			wait(NULL);
			exit(0); // !!!
		}
	} // EndOf for

} // EndOf EstimateProcCreationTime()


void EstimateProcSwitchTime ()
{
	pid_t pid;
	double dStartTime;
	double dEndTime;
	double dTotalTime;
	int iProcSwitchCount;

	printf ( "---------------------------------------------------------\n" );
	printf ( "    Estimate Procss Switching Time\n" );
	printf ( "---------------------------------------------------------\n\n" );

	for ( iProcSwitchCount = 0; iProcSwitchCount < PROC_SWITCH_COUNT; iProcSwitchCount++ )
	{
		// vfork() differs from fork() in that the child shall all memory with its parent.
		// That is convenient for communication between child and parent.
		// so using vfork() instead fork() here.
		pid = vfork();
	
		if ( pid < 0 )
		{
			printf ( "Fork failed!\n" );
			exit (-1);
		}
		// child process
		else if ( pid == 0 )
		{
			printf ( "Create a child process for the %dst time.\n", iProcSwitchCount+1 );

			// start the timer
			gettimeofday ( & s_tvStartTimeForProcSwitch, NULL );
			
			exit (0);
		}
		// parent process
		else 
		{
			// the parent is suspended until the child makes a call to execve() or _exit()
			wait ( NULL );
		
			// stop the timer
			gettimeofday ( & s_tvEndTimeForProcSwitch, NULL );
			
			dStartTime = ConvertTimevalToSecond ( s_tvStartTimeForProcSwitch );
			dEndTime = ConvertTimevalToSecond ( s_tvEndTimeForProcSwitch );
			dTotalTime = dEndTime - dStartTime;
			// just estimate the switch time once
			dTotalTime /= 2;
			PrintTime ( ProcSwitchTime, dTotalTime );
			// pre-work for print the diagram
			SetTimeUnitCount ( ProcSwitchTime, iProcSwitchCount, dTotalTime );

			if ( iProcSwitchCount == 9 )
				PrintDiagram ( ProcSwitchTime, FORK_CREATION_COUNT );
		}

	} // EndOf for()
}


void EstimateThreadCreationTime ()
{
	pthread_t tid;
	pthread_attr_t attr;
	int iThreadCreationCount;

	printf ( "---------------------------------------------------------\n" );
	printf ( "    Estimate Thread Creation Time\n" );
	printf ( "---------------------------------------------------------\n\n" );

	for ( iThreadCreationCount = 0; iThreadCreationCount < THREAD_CREATION_COUNT; iThreadCreationCount++ )
	{
		printf ( "Create a new thread for the %dst time.\n", iThreadCreationCount+1 );
 
		pthread_attr_init ( & attr );
	
		// start the timer
		gettimeofday ( & s_tvStartTimeForThreadCreation, NULL );	

		// create a new thread
		pthread_create ( & tid, & attr, ThreadFunc, (void *)iThreadCreationCount );
	
		// wait for the thread to exit
		pthread_join ( tid, NULL );
	}

	PrintDiagram ( ThreadCreationTime, FORK_CREATION_COUNT );
}


void * ThreadFunc ( void * arg )
{
	int iThreadCreationCount = (int) arg;
 	double dStartTime;
	double dEndTime;
	double dTotalTime;
	
	// stop the timer
	gettimeofday ( & s_tvEndTimeForThreadCreation, NULL );

	dStartTime = ConvertTimevalToSecond ( s_tvStartTimeForThreadCreation );
	dEndTime = ConvertTimevalToSecond ( s_tvEndTimeForThreadCreation );
	dTotalTime = dEndTime - dStartTime;
	PrintTime ( ThreadCreationTime, dTotalTime );
	
	// pre-work for print the diagram
	SetTimeUnitCount ( ThreadCreationTime, iThreadCreationCount, dTotalTime );
}


void EstimateThreadSwitchTime ()
{		
	pthread_t tid;
	pthread_attr_t attr;
	int iThreadSwitchCount;
 	struct timeval tvStart;
	struct timeval tvEnd;
	double dStartTime;
	double dEndTime;
	double dTotalTime;
	
	printf ( "---------------------------------------------------------\n" );
	printf ( "    Estimate Thread Switch Time\n" );
	printf ( "---------------------------------------------------------\n\n" );
	
	for ( iThreadSwitchCount = 0; iThreadSwitchCount < THREAD_SWITCH_COUNT; iThreadSwitchCount++ )
	{
		pthread_attr_init ( & attr );
		
		pthread_create ( & tid, & attr, ThreadFuncForSwitch, NULL );
	
		printf ( "Create a new thread for the %dth time.\n", iThreadSwitchCount+1 );
		
		// start the timer		
		gettimeofday ( & tvStart, NULL );
		
		pthread_join ( tid, NULL );

		// stop the timer
		gettimeofday ( & tvEnd, NULL );

		dStartTime = ConvertTimevalToSecond ( tvStart );
		dEndTime = ConvertTimevalToSecond ( tvEnd );
		dTotalTime = dEndTime - dStartTime;
		dTotalTime /= 2;
		PrintTime ( ThreadSwitchTime, dTotalTime );
		// pre-work for print the diagram
		SetTimeUnitCount ( ThreadSwitchTime, iThreadSwitchCount, dTotalTime );
	}

	PrintDiagram ( ThreadSwitchTime, FORK_CREATION_COUNT );
}


void * ThreadFuncForSwitch ( void * arg )
{
}


void EstimateDiskBandwithIoTime ()
{
	int fd;
	struct timeval tvStart;
	struct timeval tvEnd;
	char strBuffer[] = "abcde";
	char strOut[] = "opqrs";
	int iIoCount;
	double dStartTime;
	double dEndTime;
	double dTotalTime;

	printf ( "---------------------------------------------------------\n" );
	printf ( "    Estimate Disk Bandwidth with IO Time\n" );
	printf ( "---------------------------------------------------------\n\n" );
	
	for ( iIoCount = 0; iIoCount < IO_COUNT; iIoCount++ )
	{
		//
		// write data to a file and estimate the time
		//
		if ( ( fd = open ( "tmp", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR ) ) < 0 )
		{
			printf ( "open failed.\n" );
		}

		// start the timer
		gettimeofday ( & tvStart, NULL );	

		if ( ( write ( fd, strBuffer, sizeof(strBuffer) ) ) != sizeof(strBuffer) )
		{
			printf ( "Write failed.\n" );
		}

		// stop the timer
		gettimeofday ( & tvEnd, NULL );

		printf ( "Wrote a string \"%s\" to a file for the %dst time.\n", strBuffer, iIoCount+1 );
		dStartTime = ConvertTimevalToSecond ( tvStart );
		dEndTime = ConvertTimevalToSecond ( tvEnd );
		dTotalTime = dEndTime - dStartTime;
		PrintTime ( DiskBandwithIoTime, dTotalTime );
		// pre-work for print the diagram
		SetTimeUnitCount ( WriteFileTime, iIoCount, dTotalTime );

		close ( fd );
	}
	
	PrintDiagram ( WriteFileTime, FORK_CREATION_COUNT );	
	printf ( "\n" );
	
	for ( iIoCount = 0; iIoCount < IO_COUNT; iIoCount++ )
	{
		//
		// read data from a file and estimate the time
		//
		if ( (fd = open ( "tmp", O_RDONLY ) ) < 0 )
		{	
			printf ( "open failed.\n" );
		}

		// start the timer
		gettimeofday ( & tvStart, NULL );

		if ( read ( fd, strOut, sizeof(strBuffer) ) != sizeof(strBuffer) )
		{
			printf ( "read failed.\n" );
		}

		// stop the timer
		gettimeofday ( & tvEnd, NULL );	
		
		printf ( "Read a string \"%s\" from a file for the %dst time.\n", strOut, iIoCount+1 );	
		dStartTime = ConvertTimevalToSecond ( tvStart );
		dEndTime = ConvertTimevalToSecond ( tvEnd );
		dTotalTime = dEndTime - dStartTime;
		PrintTime ( DiskBandwithIoTime, dTotalTime );
		SetTimeUnitCount ( ReadFileTime, iIoCount, dTotalTime );

		close ( fd );
	}

	PrintDiagram ( ReadFileTime, FORK_CREATION_COUNT );
}


double ConvertTimevalToSecond ( struct timeval tvTime )
{
	double dTime = 0.0;

	// convert to usec
	dTime = 1000000 * tvTime.tv_sec + tvTime.tv_usec;
	// convert to sec	
	dTime /= 1000000;

	return dTime;
}


void PrintTime ( enum PerfType perfType, double dSecond )
{
	char * pPerfType;

	switch ( perfType )
	{
		case ProcCreationTime:
			pPerfType = "Estimate the Process Creation Time";
			break;
		case ContextSwitchTime:
			pPerfType = "Estimate the Context Switching Time";
			break;
		case ThreadCreationTime:
			pPerfType = "Estimate the Thread Creation Time";
			break;
		case ProcSwitchTime:
			pPerfType = "Estimate the Process Switching Time from child to parent";
			break;
		case ThreadSwitchTime:
			pPerfType = "Estimate the Thread Switch Time";
			break;
		case DiskBandwithIoTime:
			pPerfType = "Estimate the Disk Bandwith IO Time";
			break;
	}

	printf ( "%s = %f\n\n", pPerfType, dSecond );
}


void SetTimeUnitCount ( enum PerfType perfType, int iCount, double dTime )
{
	int iUnitCount;

	switch ( perfType )
	{
		case ContextSwitchTime:
			// BUG: CONVERSION PROBLEM!!!
			s_iContextSwitchTimeUnitCountArr [ iCount ] = ( int ) ( ( double ) dTime / ( double ) 0.0000001 );
			
			#ifdef DEBUG
			printf ( "SetTimeUnitCount() ---- dTime = %f\n", dTime );	
			printf ( "SetTimeUnitCount() ---- dTime / 0.000001 = %d\n", (int)(dTime/0.000001) ); // something wrong??!!
			printf ( "SetTimeUnitCount() ---- 0.000004 / 0.000001 = %d\n", (int)(0.000004/0.000001) );	
			#endif

			break;

		case ProcCreationTime:
			s_iProcCreationTimeUnitCountArr [ iCount ] = ( int ) ( ( double ) dTime / ( double ) 0.0001 );
			break;

		case ProcSwitchTime:
			s_iProcSwitchTimeUnitCountArr [ iCount ] = ( int ) ( ( double ) dTime / ( double ) 0.000001 );
			break;

		case ThreadCreationTime:
			s_iThreadCreationTimeUnitCountArr [ iCount ] = ( int ) ( ( double ) dTime / ( double ) 0.000001 );
			break;

		case ThreadSwitchTime:
			s_iThreadSwitchTimeUnitCountArr [ iCount ] = ( int ) ( ( double ) dTime / ( double ) 0.000001 );
			break;

		case WriteFileTime:
			s_iWriteFileTimeUnitCountArr [ iCount ] = ( int ) ( ( double ) dTime / ( double ) 0.00001 );
			break; 

		case ReadFileTime:
			s_iReadFileTimeUnitCountArr [ iCount ] = ( int ) ( ( double ) dTime / ( double ) 0.000001 );
			break;
	}		
}

void PrintDiagram ( enum PerfType perfType, int iCount )
{
	int i = 0;
	int j = 0;
	
	printf ( "[Print Diagram]\n" );	
	
	switch ( perfType )
	{
		case ContextSwitchTime:
			printf ( "Each \'#\' is about 0.0000001sec.\n" );
			break;

		case ProcCreationTime:
			printf ( "Each \'#\' is about 0.0001sec.\n" );
			break;

		case ProcSwitchTime:
			printf ( "Each \'#\' is about 0.000001sec.\n" );
			break;

		case ThreadCreationTime:
			printf ( "Each \'#\' is about 0.000001sec.\n" );
			break;

		case ThreadSwitchTime:
			printf ( "Each \'#\' is about 0.000001sec.\n" );
			break;

		case WriteFileTime:
			printf ( "Each \'#\' is about 0.00001sec.\n" );
			break; 

		case ReadFileTime:
			printf ( "Each \'#\' is about 0.000001sec.\n" );
			break;
	}		

	for ( i = 0; i < iCount; i++ )
	{ 
		if ( perfType == ContextSwitchTime )
		{
			printf ( "[%2dst time]", i+1 );

			for ( j = 0; j < s_iContextSwitchTimeUnitCountArr[i]; j++ )
			{
				printf ( "#" );			
			}
			printf ( "\n" );
		}
		else if ( perfType == ProcCreationTime )
		{
			printf ( "[%2dst time]", i+1 );
			
			for ( j = 0; j < s_iProcCreationTimeUnitCountArr[i]; j++ )
			{
				printf ( "#" );
			}	
			printf ( "\n" );
		}
		else if ( perfType == ProcSwitchTime )
		{	
			printf ( "[%2dst time]", i+1 );

			for ( j = 0; j < s_iProcSwitchTimeUnitCountArr[i]; j++ )
			{
				printf ( "#" );
			}	
			printf ( "\n" );
		}
		
		else if ( perfType == ThreadCreationTime )
		{
			printf ( "[%2dst time]", i+1 );
			for ( j = 0; j < s_iThreadCreationTimeUnitCountArr[i]; j++ )
			{
				printf ( "#" );
			}	
			printf ( "\n" );
		}
		else if ( perfType == ThreadSwitchTime )
		{
			printf ( "[%2dst time]", i+1 );
			for ( j = 0; j < s_iThreadSwitchTimeUnitCountArr[i]; j++ )
			{
				printf ( "#" );
			}	
			printf ( "\n" );
		}
		else if ( perfType == WriteFileTime )
		{
			printf ( "[%2dst time]", i+1 );
			for ( j = 0; j < s_iWriteFileTimeUnitCountArr[i]; j++ )
			{
				printf ( "#" );
			}	
			printf ( "\n" );
		}
		else if ( perfType == ReadFileTime )
		{ 
			printf ( "[%2dst time]", i+1 );
			for ( j = 0; j < s_iReadFileTimeUnitCountArr[i]; j++ )
			{
				printf ( "#" );
			}	
			printf ( "\n" );
		}
	}

	printf ( "\n" );
}
