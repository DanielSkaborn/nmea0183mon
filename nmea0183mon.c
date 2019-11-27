#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>

#define	MAXTAGS	10
#define MAXIDTS 100

char	NMEAsentence[255] = {"$PJMJP,0,BCS,0,KRX,0,TVA,0,TRE,3,NLN*32\0"};

char	tags[MAXTAGS][10];	// tag strings
int		ntags;			// number of tags found

char	idts[MAXTAGS][100][10];	// identifier strings
int		nidts[MAXTAGS];			// number of identifiers found, by tag

int		currentTag;
int		currentIdt;
char	currentData[30];

char	currentIdts[10];
int		ended;


char	portname[128] = "/dev/ttyUSB0";
int		fd;

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
	
/*	row = currentIdt / 25;
	col = ((currentIdt - (row*25)) * 25) - 20;*/

	col = (currentIdt-1) / 25;
	row = (currentIdt-1) - (col*25);

	col *= 20;

	printf("%c[%d;%df",0x1b, row+3, col+3);
	printf(" %s %s    \n",currentIdts,currentData);
	printf("\n");
	return;
}

void printSentence(void) {
	int row, col;
	row = 50;
	col = 5;
	printf("%c[%d;%df",0x1b, row+2, col+3);
	printf("%s                  \n",NMEAsentence);
	return;
}

void printRegistry(void) {
	for (int i=0;i<25;i++) {
		printf(" %d %s    %d %s   %d %s   %d %s \n", i, idts[0][i],i+25,idts[0][i+25],i+50, idts[0][i+50],i+75,idts[0][i+75]);
	}
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
	ntags = 1;
	
	for (int i=0;i<MAXTAGS;i++)
		for (int j=0;j<100;j++)
			for (int k=0;k<10;k++)
				idts[i][j][k]= '\0';
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
//	printf(tempTag);printf("\n");
//	printf(tags[0]);
//	printf("\nTagMatch %d %d\n", currentTag, match);

	if (currentTag != -1) {
		ended = 0;
//		printf("capture idts\n");
		while(ended==0) {
//			printf("n:%d\n",n);
			n=captureData(n);
//			printf("n:%d - ",n);
//			printf(currentData);
//			printf("\n");
			
			if (ended == 0) {
				n=captureIdts(n);
//				printf("n:%d - ",n);
//				printf(currentIdts);
//				printf("\n");
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
//						printf("%d \n",match);
					}
				}
				if (match == 10)
					currentIdt = i;
				i++;
			}
		
			// register tag if new
//			printf("match: %d\n",match);
			if ((match != 10)) {
//				printf("register idt: %d %s\n",nidts[currentTag],currentIdts);
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
		printSentence();
	}
}

/*
float a,b,c,d,e;
int chunk= 0;
void makeFakeNmea(void) {
	a += 0.1;
	b -= 0.3;
	c = (c+0.1)*1.1;
	d += 1;
	e -= 1;
	if ((a<-100)||(a>100)) a=0;
	if ((b<-100)||(b>100)) b=0;
	if ((c<-100)||(c>100)) c=0;
	if ((d<-100)||(d>100)) d=0;
	if ((e<-100)||(e>100)) e=0;
	chunk++;
	if (chunk == 5)
		chunk = 0;
		
	switch (chunk) {
		case 0:
			sprintf(NMEAsentence,"$PJMJP,%.1f,BCS,%.1f,KRX,%.1f,TVA,%.1f,TRE,%.1f,NLN*32\0",a,b,c,d,e);
			break;
		case 1:
			sprintf(NMEAsentence,"$PJMJP,%.1f,RRR,%.1f,LRX,%.1f,AVA,%.1f,KRE,%.1f,PLN*32\0",a,b,c,d,e);
			break;
		case 2:
			sprintf(NMEAsentence,"$PJMJP,%.1f,BYY,%.1f,QGX,%.1f,TVD,%.1f,TPD,%.1f,NOL*32\0",a,b,c,d,e);
			break;
		case 3:
			sprintf(NMEAsentence,"$PJMJP,%.1f,BOO,%.1f,GGX,%.1f,TVQ,%.1f,TWE,%.1f,GYR*32\0",a,b,c,d,e);
			break;
		case 4:
			sprintf(NMEAsentence,"$PJMJP,%.1f,BAA,%.1f,KQX,%.1f,TQA,%.1f,TWW,%.1f,NFE,99,LRK,%.2f,KDF*32\0",a,b,c,d,e,e);
			break;
	}
}
*/

void set_blocking(int should_block) {
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0) {
		return;
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;	// 0.5 seconds read timeout
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
	fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
		return;

	set_interface_attribs (fd, B4800, 0);  // set speed to 4800baud, 8n1 (no parity)
	set_blocking (0);                // set no blocking
}

int dataRead(void) {
	int newSentence = 0;
	static int npos = 255;
	static char tempNMEAsentence[255];
	char buf[100];
	
	int n,i,j;
	
	n = read (fd, buf, sizeof buf);  // read up to 100 characters if ready to read
	
	if (n!=0) {
		for (i=0;i<n;i++) {
			if (buf[i]=='$') {
				for (j=0;j<255;j++) {
					NMEAsentence[j] = tempNMEAsentence[j];
					newSentence = 1;
					npos = 0;
				}
			}
			tempNMEAsentence[npos] = buf[i];
			npos++;
		}				
	}
	return newSentence;
	
}	



int main(void) {
	printf("\033c");
	printf("  *** NMEA0183 - PJMJP monitor *** \n");
	init();
	
	openSerialPort();
	
	while(1)
		if (dataRead())
			parseSentence();

	
/*	printSentence();
	printf("\n");
	parseSentence();
	makeFakeNmea();
	parseSentence();
	makeFakeNmea();
	parseSentence();
	makeFakeNmea();
	parseSentence();
	makeFakeNmea();
	parseSentence();
	makeFakeNmea();
	parseSentence();
	printf("\033c");
	printRegistry();
	sleep(1);
	printf("\033c");
	while(1) {
		makeFakeNmea();
		parseSentence();
		usleep(100000);
	}*/
		
}
