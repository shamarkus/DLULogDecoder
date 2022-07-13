#include "ParseLog.h"
#include <unistd.h>
//IF NOT OKAY FOR WINDOWS (VC) ACCESS() FUNCTION
//THEN INCLUDE THIS
//#ifdef WIN32
//#include <io.h>
//#define F_OK 0
//#define access _access
//#endif
//https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c


// Executable Command Format : 
// ./DLULogProcessor FILENAME TIME1 TIME2 ARGS
// 
// EXAMPLE : 
// ./DLULogProcessor 22.06.2022.11h28-11h38.8001c0_0001447138.txt 2022/06/22 11:28:00 2022/06/22 11:38:00 17 19 20 29
// ATO FILE 22.06.2022.11h28-11h38.8001c0_0001447138.txt
// ATP FILE 22.06.2022.11h18-11h28.800280_0001447134.txt
//Filenames for conversions between column number and corresponding string for ATP & ATO style files
static const char headerConversions_ATO_Filename[] = "NumToHeaderString_ATO.txt";
static const char headerConversions_ATP_Filename[] = "NumToHeaderString_ATP.txt";

//Function Prototypes/Declarations
int determineATPorATO(struct Parameters* inputParams,char* arg);

time_t determineEpochTime(char* yearDate,char* hourDate);

int determineESTorEDT(time_t tm);

void DetermineSpecificArgs(char** argv,char Args[MAX_COLUMN_SIZE][MAX_STRING_SIZE],char* headerConversionsFilename);

struct Parameters* initializeParams(struct Parameters* inputParams,int argc,char** argv);

void printParams(struct Parameters* inputParams);

bool checkOutputFilename(struct Parameters* inputParams, char outputFilename[]);

char *strptime_I(const char *buf, const char *fmt, struct tm *tm);

char *strptime_I(const char *buf, const char *fmt, struct tm *tm){
	//if(strptime(stringDate,"%Y/%m/%d %H:%M:%S",tm) != NULL){

	//Strip the string to get the numeric value
	int Y,m,d,H,M,S;
	sscanf(buf,fmt,&Y,&m,&d,&H,&M,&S);
	//Configure based on struct tm format
	tm->tm_isdst = -1;
	tm->tm_year= (Y-1900);
	tm->tm_mon = (m-1);
	tm->tm_mday = d;
	tm->tm_hour = H;
	tm->tm_min = M;
	tm->tm_sec = S;
	if(mktime(tm) < 1){
		return NULL;
	}
	return buf;
}


//Returns the numerical value that represents whether the file is ATP or ATO
//Necessary for correct timestamping
int determineATPorATO(struct Parameters* inputParams,char* arg){
	char* delimStr = strtok(arg,".");
	char* tempStr;
	while(delimStr != NULL){
		tempStr = delimStr;
		strcat(inputParams->initFilename,tempStr);
		delimStr = strtok(NULL,".");
		if(delimStr != NULL && !strcmp(delimStr,TXT_SUFFIX)){
			if(*(tempStr+4)=='c') return ATO_NUM;
			else if(*(tempStr+4)=='8') return ATP_NUM;
		}
		strcat(inputParams->initFilename,".");
	}
	printf("Error: Passed filename was named incorrectly, and 8001c0 or 800180 was unfound\n");
	return AT_UNDEFINED_NUM;
}

//Returns Epoch Time from date strings of format %Y/%m/%d %H:%M:%S
time_t determineEpochTime(char* yearDate,char* hourDate){
	char stringDate[MAX_TIME_STRING_SIZE];	
	sprintf(stringDate,"%s %.*s",yearDate,8,hourDate);
	struct tm* tm = (struct tm*)malloc(sizeof(struct tm));

	if(strptime_I(stringDate,"%d/%d/%d %d:%d:%d",tm) != NULL){
		time_t time = mktime(tm);
		free(tm);
		return time;
	}
	else {
		printf("Error: Arguments do not contain the correct time. The current time will be returned\n");
		return time(NULL);
	}
}

//Necessary for determining whether the current timezone is EST or EDT
int determineESTorEDT(time_t tm){
	char timezoneString[MAX_STRING_SIZE];
	if(strftime(timezoneString,MAX_STRING_SIZE,"%Z",localtime(&tm)) != 0){
		if(!strcmp(timezoneString,UTC5_TIME)) return UTC5_TIME_NUM;
		return UTC4_TIME_NUM;
	}
	else {
		printf("Error: Converting to EST or EDT was Unsuccessful\n");
		exit(EXIT_FAILURE);
	}
}

//Takes integer based parameters which refer to a specific header string
//Conversion occurs here, through a .txt file which determines the string from the number of the argument
void DetermineSpecificArgs(char** argv,char Args[MAX_COLUMN_SIZE][MAX_STRING_SIZE],char* headerConversionsFilename){
	FILE *file = fopen(headerConversionsFilename,"r");
	int argCount = 0;
	int lineCount = 1;

	if(file != NULL){
		char line[MAX_STRING_SIZE];
		while(fgets(line, sizeof(line), file) != NULL){
			if(argv[argCount+6] == NULL) break;
			else if((atoi(argv[argCount+6])) == lineCount){
				strcpy(Args[argCount],strtok(line,"\t"));
				strcpy(Args[argCount+1],"\0");
				argCount++;
			}
			lineCount++;
		}
		fclose(file);
	}
	else printf("Error: NumToHeaderString.txt doesn't exist in the /exe folder\n");
}

//Initializes the input parameters struct object
//Determines start, end times, as well as all other input parameters
//Params initialized in format of :
// ./DLULogProcessor FILENAME : 
struct Parameters* initializeParams(struct Parameters* inputParams,int argc,char** argv){
	//Dynamically allocate space for struct
	inputParams = (struct Parameters*)malloc(sizeof(struct Parameters));
	
	//Determine whether ATP or ATO file
	inputParams->AT_DEF = determineATPorATO(inputParams,argv[1]);
	//Determine Start & End Times

	inputParams->startTime = determineEpochTime(argv[2],argv[3]); 
	inputParams->endTime = determineEpochTime(argv[4],argv[5]); 
	//Determine Timezone
	inputParams->UTC = determineESTorEDT(inputParams->endTime);
	//Update Times Based on Timezone And Normalize
	struct tm* endTMstruct = localtime(&(inputParams->endTime));
	endTMstruct->tm_hour += inputParams->UTC;
	inputParams->endTime = mktime(endTMstruct);
	
	struct tm* startTMstruct = localtime(&(inputParams->startTime));
	startTMstruct->tm_hour += inputParams->UTC;
	inputParams->startTime = mktime(startTMstruct);

	printf("%d vs %d  \n",inputParams->startTime,inputParams->endTime);
	//Free?

	//Determine Parameter Information
	inputParams->numOfArgs = argc;
	inputParams->argv= argv;
	char* headerConversionsFilename = inputParams->AT_DEF ? headerConversions_ATP_Filename : headerConversions_ATO_Filename;
	DetermineSpecificArgs(argv,inputParams->Args,headerConversionsFilename);

	return inputParams;
}

void printParams(struct Parameters* inputParams){
	printf("%d \n",inputParams->AT_DEF);
	printf("%d \n",inputParams->UTC);
	printf("%s \n",inputParams->initFilename);
	printf("%d \n",inputParams->startTime);
	printf("%d \n",inputParams->endTime);
	printf("%d \n",inputParams->numOfArgs);
	for(int i = 0; i < sizeof(inputParams->Args)/sizeof(inputParams->Args[0]); i++) {
		if(!strcmp(inputParams->Args[i],"\0")) break;
		printf("%s \n",inputParams->Args[i]);
	}
}
bool checkOutputFilename(struct Parameters* inputParams, char outputFilename[]){
	char tempOutputFilename[MAX_STRING_SIZE];
	sprintf(tempOutputFilename,"%s.%s",inputParams->initFilename,CSV_SUFFIX);
	if (access(tempOutputFilename, F_OK) != 0) {
		    // file doesnt exist
		    strcpy(outputFilename,tempOutputFilename);
		    return true;
	}
	for(int fileCounter=1;fileCounter<100;fileCounter++){
		sprintf(tempOutputFilename,"%s-%02d.%s",inputParams->initFilename,fileCounter,CSV_SUFFIX);
		if (access(tempOutputFilename, F_OK) != 0) {
			    // file doesnt exist
			    strcpy(outputFilename,tempOutputFilename);
			    printf("%s \n",outputFilename);
			    return true;
		}
	}
	printf("Error: The directory is full -- Program will be terminated\n");
	return false;	
}
int main(int argc,char** argv){

	struct Parameters* inputParams;
	char outputFilename[MAX_STRING_SIZE];

	inputParams = initializeParams(inputParams,argc,argv);

	printParams(inputParams);
	
	if(!checkOutputFilename(inputParams,outputFilename)) return 0;

	parseLogFile(inputParams,outputFilename);

	free(inputParams);

	printf("File %s Has Been Created\n",outputFilename);

	return 0;
}