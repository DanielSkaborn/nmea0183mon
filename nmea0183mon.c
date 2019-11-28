#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>

#define	MAXTAGS	10
#define MAXIDTS 100

char	NMEAsentence[255];// = {"$PJMJP,0,BCS,0,KRX,0,TVA,0,TRE,3,NLN*32\0"};

char	tags[MAXTAGS][10];	// tag strings
int		ntags;			// number of tags found

char	idts[MAXTAGS][100][10];	// identifier strings
int		nidts[MAXTAGS];			// number of identifiers found, by tag

int		currentTag;
int		currentIdt;
char	currentData[30];

char	currentIdts[10];
int		ended;

int		makeLog = 0;

char	portname[128];// = "/dev/ttyUSB0";
int		fd;
FILE *	fp;

int captureData(int startn) {
	int i;
	int n = startn;
	
	if (NMEAsentence[n] == ',')
		n++;
		
	for (i=0;i<30;i++)
		currentData[i]='\0';

	i=0;
	while(i<30 && NMEAsentence[n] != ',' && NMEAsentence[n] != '\0' && NMEAsentence[n]!='*') {
		currentData[i] = NMEAsentence[n];
		if (NMEAsentence[n] == '*')
			ended = 1;
		n++;
		i++;
	}
	return n;
}

void printTagIdt(void) {
	int row, col;

	col = (currentIdt-1) / 25;
	row = (currentIdt-1) - (col*25);

	row += currentTag*27;
	col *= 28;

	printf("%c[%d;%df",0x1b, row+3, col+3);
	printf("                           \n",currentIdts,currentData);
	printf("%c[%d;%df",0x1b, row+3, col+3);
	printf(" %s %s\n",currentIdts,currentData);
	printf("\n");
	return;
}

void printSentence(void) {
	static int closeOpenCnt = 10;
	int row, col;
	row = 55;
	col = 5;
	printf("%c[%d;%df",0x1b, row+2, col+3);
	printf("                                                                                                            \n");

	printf("%c[%d;%df",0x1b, row+2, col+3);
	printf("%s\n",NMEAsentence);
	
	if (makeLog) {
		fprintf(fp, "%s\n", NMEAsentence);
		closeOpenCnt--;
		if (closeOpenCnt==0) {
			closeOpenCnt = 10;
			fclose(fp);
			fp = fopen("./VDRdata.nmea","a");
		}
	}

	return;
}

void printChecksum(int csresult) {
	int row, col;
	row = 56;
	col = 5;
	printf("%c[%d;%df",0x1b, row+2, col+3);
	
	switch (csresult) {
		case 0:
			printf("Checksum ERROR!      \n");
			break;
		case 1:
			printf("Checksum OK!         \n");
			break;
		case -1:
			printf("Invalid Format! [-1] \n");
			break;
		case -2:
			printf("Invalid Format! [-2] \n");
			break;
		default:
			printf("                     \n");
			break;
	}
	return;
}


void printRegistry(void) {
	for (int i=0;i<25;i++) {
		printf(" %d %s    %d %s   %d %s   %d %s \n", i, idts[0][i],i+25,idts[0][i+25],i+50, idts[0][i+50],i+75,idts[0][i+75]);
	}
}

int checksumtest(void) {
	char checksum = 0;
	char rcvChecksum = 0;
	int npos = 0;
	
	while(NMEAsentence[npos]!='*' && NMEAsentence[npos]!='\0' && npos!=255) {
		checksum ^= NMEAsentence[npos];
		npos++;
	}
	checksum ^= '$';

	if (npos == 255)
		return -1;	// invalid format, no * found
	if (NMEAsentence[npos] == '\0')
		return -2;	// invalid format, no * found
	
	if ( NMEAsentence[npos+1]>='0' && NMEAsentence[npos+1]<='9')
		rcvChecksum = (NMEAsentence[npos+1]-'0') * 0x10;
	else
		if ( NMEAsentence[npos+1]>='A' && NMEAsentence[npos+1]<='F')
			rcvChecksum = ( NMEAsentence[npos+1] - 'A' + 0xA ) * 0x10;
			
	if ( NMEAsentence[npos+2]>='0' && NMEAsentence[npos+2]<='9')
		rcvChecksum += ( NMEAsentence[npos+2] - '0' );
	else
		if ( NMEAsentence[npos+2]>='A' && NMEAsentence[npos+1]<='F')
			rcvChecksum += ( NMEAsentence[npos+2] - 'A' + 0xA );
	
//	printf("                                          %c%c %X %X  \n",NMEAsentence[npos+1], NMEAsentence[npos+2],rcvChecksum, checksum);
	
	if (rcvChecksum == checksum)
		return 1;	// checksum ok
	else
		return 0;	// checksum error
}

int captureIdts(int startn) {
	int i;
	int n = startn;

	if (NMEAsentence[n] == ',')
		n++;

	if (NMEAsentence[n] == '*') {
		ended = 1;
		return n;
	}
		
	// whipe
	for (i=0;i<10;i++)
		currentIdts[i]='\0';

	// copy in
	i=0;
	while(i<10 && NMEAsentence[n] != ',' && NMEAsentence[n] != '\0' && NMEAsentence[n]!='*') {
		currentIdts[i] = NMEAsentence[n];
		n++;
		i++;
		if (NMEAsentence[n] == '*')
			ended = 1;

	}
	return n;
}

void init(void) {
	tags[0][0]='$';
	tags[0][1]='P';
	tags[0][2]='J';
	tags[0][3]='M';
	tags[0][4]='J';
	tags[0][5]='P';
	tags[0][6]='\0';
	tags[0][7]='\0';
	tags[0][8]='\0';
	tags[0][9]='\0';
	
	tags[1][0]='$';
	tags[1][1]='P';
	tags[1][2]='B';
	tags[1][3]='M';
	tags[1][4]='J';
	tags[1][5]='P';
	tags[1][6]='\0';
	tags[1][7]='\0';
	tags[1][8]='\0';
	tags[1][9]='\0';
	ntags = 2;
	
	for (int i=0;i<MAXTAGS;i++)
		for (int j=0;j<100;j++)
			for (int k=0;k<10;k++)
				idts[i][j][k]= '\0';
	
	if (makeLog) {
		fp = fopen("./VDRdata.nmea","a");
		fprintf(fp, "*** Start recorder ***\n");
	}

	return;
}

void parseSentence(void) {
	int n=0;
	int i,j;
	int match;
	char tempTag[10];
	char tempData[30];

	for (i=0;i<10;i++)
		tempTag[i] = '\0';
	
	// get the tag
	while (NMEAsentence[n] != '\0' && NMEAsentence[n] != ',' && n<10) {
		if (n<10)
			tempTag[n] = NMEAsentence[n];
		n++;
	}
	currentTag = -1;
	
	// check if it match
	i=0;
	match=0;
	while (i!=ntags && match !=10) {
		match = 0;
		for (j=0;j<10;j++)
			if (tags[i][j] == tempTag[j])
				match++;

		if (match == 10)
			currentTag = i;
		i++;
	}

	if (currentTag != -1) {
		ended = 0;
		while(ended==0) {
			n=captureData(n);
			if (ended == 0) {
				n=captureIdts(n);
			}
			currentIdt = -1;
	
			// check if it match
			i=0;
			match = 0;
			while (i!=nidts[currentTag] && match !=10) {
				match = 0;
				for (j=0;j<10;j++) {
					if (idts[currentTag][i][j] == currentIdts[j]){
						match++;
					}
				}
				if (match == 10)
					currentIdt = i;
				i++;
			}
		
			// register tag if new
			if ((match != 10)) {
				if (nidts[currentTag] < MAXIDTS) {
					nidts[currentTag]++;
					for (i=0;i<10;i++) {
						idts[currentTag][nidts[currentTag]][i] = currentIdts[i];
						currentIdt = nidts[currentTag];
					}
				}

			}
			if (currentIdt !=-1)
				printTagIdt();
		}
	}
}

void set_blocking(int should_block) {
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0) {
		return;
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 1;	// 0.5 seconds read timeout
}


int set_interface_attribs (int fd, int speed, int parity) {
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	
	if (tcgetattr (fd, &tty) != 0)
		return -1;


	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
	tty.c_iflag &= ~IGNBRK;				// disable break processing
	tty.c_lflag = 0;					// no signaling chars, no echo,
                                        // no canonical processing
	tty.c_oflag = 0;					// no remapping, no delays
	tty.c_cc[VMIN]  = 0;				// read doesn't block
	tty.c_cc[VTIME] = 5;				// 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);	// ignore modem controls,
										// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);	// shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0) {
		return -1;
	}
	return 0;
}
void openSerialPort(void) {
	int row = 55;
	int col = 5;

	fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		row = 55;
		col = 5;
		printf("%c[%d;%df",0x1b, row+2, col+3);
		printf("                                                                                                      \n");
		printf("FAILED TO OPEN SERIAL PORT!\n");
		sleep(1);
		return;
	}

	set_interface_attribs (fd, B4800, 0);  // set speed to 4800baud, 8n1 (no parity)
	set_blocking (0);                // set no blocking
}


int dataRead(void) {
	static int npos = 0;
	int newSentence = 0;
	
	static char tempNMEAsentence[255];
	char buf[10];
	
	int n,i,j;
	
	n = read (fd, buf, sizeof buf);  // read up to 100 characters if ready to read
	if (n!=0) {
		for (i=0;i<n;i++) {
			if (buf[i]=='$') {
				for (j=0;j<255;j++) {
					NMEAsentence[j] = tempNMEAsentence[j];
					if (NMEAsentence[j] == '\n')
						NMEAsentence[j] = '\0';
				}
				printSentence();
				newSentence = 1;
				npos = 0;
			}
			tempNMEAsentence[npos] = buf[i];
			tempNMEAsentence[npos+1] ='\0';
			tempNMEAsentence[npos+2] ='\0';
			npos++;
		}				
	}
	return newSentence;
	
}	


int main (int argc, char *argv[]) {
	int n = 0;
	if (argc>=2) {
		sprintf(portname, "%s", argv[1]);
		if (portname[0] == 'h')
			printf("    *** NMEA0183 - $PJMJP $PBMJP monitor *** \n\n - nmon serial_device\n\n No serial_device defaults to /dev/ttyUSB0\n\n example usage: nmon /dev/ttyUSB2   - will try to open ttyUSB2\n\n");
	} else
		sprintf(portname, "/dev/ttyUSB0");	// default to /dev/ttyUSB0
	
	if (argc>=3)
		if ( (argv[2][0] == 'l') && (argv[2][1] == 'o') && (argv[2][2] == 'g') )
			makeLog = 1;
	
	int csresult;
	printf("\033c");
	if (makeLog) 
		printf("    *** NMEA0183 - $PJMJP $PBMJP monitor [ %s ] [ WRITE LOG ] *** \n", portname);
	else
		printf("    *** NMEA0183 - $PJMJP $PBMJP monitor [ %s ] *** \n", portname);

	init();
	openSerialPort();
	
	while(1)
		if (dataRead()) {
			csresult=checksumtest();
			if (csresult == 1)
				parseSentence();
			printChecksum(csresult);
		}
}
