#include "ParseLog.h"


//Returns Epoch Time from date strings of format %Y/%m/%d %H:%M:%S
time_t determineEpochTime(char* yearDate,char* hourDate){
	char stringDate[MAX_TIME_STRING_SIZE];	
	sprintf(stringDate,"%s %.*s",yearDate,8,hourDate);
	struct tm* tm = (struct tm*)malloc(sizeof(struct tm));

	if(strptime_I(stringDate,"%d/%d/%d %d:%d:%d",tm) || strptime_I(stringDate,"%d-%d-%d %d:%d:%d",tm))
	{
		time_t time = mktime(tm);
		free(tm);
		return time;
	}
	else
	{
		printf("Error: Arguments do not contain the correct time. The current time will be returned\n");
		return time(NULL);
	}
}

//Takes dates and configures to 'struct tm' from 'time.h'
//Necessary for converting string dates to epoch times
const char *strptime_I(const char *buf, const char *fmt, struct tm *tm){
	//Strip the string to get the numeric value
	int Y,m,d,H,M,S;
	sscanf(buf,fmt,&Y,&m,&d,&H,&M,&S);
	
	//check for YYYY-MM-DD or YYYY/MM/DD
	if(!m || !d)
	{
		return NULL;
	}
	else
	{
		//do nothing
	}

	//Configure based on struct tm format
	tm->tm_isdst = -1;
	tm->tm_year= (Y-1900);
	tm->tm_mon = (m-1);
	tm->tm_mday = d;
	tm->tm_hour = H;
	tm->tm_min = M;
	tm->tm_sec = S;
	if(mktime(tm) < 1)
	{
		return NULL;
	}
	else
	{
		return buf;
	}
}

//Takes a line from the DLU log & finds the necessary arguments
//argN holds the numeric argument -- location of where the token is delimeted by \t
void tokenizeLine(struct Parameters* inputParams,struct recordInfo* curRecord){

	int tokenCount = ARG_NUM_AFTER_DATE_TIME;	
	char* tokenStr = strtok(NULL,"\t");
	int argCount = 0;
	int* argN = inputParams->argN;
	//tokenizes the line and completes logArgs
	while(tokenStr != NULL){
		if(-1 == argN[argCount]) 
		{
			return;
		}
		else if(argN[argCount] == tokenCount)
		{
			strcpy(curRecord->logArgs[argCount],tokenStr);
			strcpy(curRecord->logArgs[argCount+1],"\0");
			argCount++;
		}
		else
		{
			//do nothing
		}
		tokenCount++;
		tokenStr = strtok(NULL,"\t");
	}
}

//Makes a column in the output .csv file and prints the string arguments to be compared
void writeHeaderLine(FILE* fileOutput,struct Parameters* inputParams){
	for(int i = 0; i < sizeof(inputParams->Args)/sizeof(inputParams->Args[0]);i++){
		if(!strcmp(inputParams->Args[i],"\0"))
		{
			return;
		}
		else
		{
			//do nothing
		}
		fprintf(fileOutput,",%s",inputParams->Args[i]);
	}
}

//Main function that writes a row to the fileOutput if the preceding row exhibited any differences
//recordArray holds the records that correspond to the first second of the fileInput
//Necessary for accurate timestamping -- The first line from the DLU file is always printed
//Afterwards, the current line is compared to the previous line for any differences - if so, then it is printed to the .csv file
void writeChangingRecords(FILE* fileInput,FILE* fileOutput,struct Parameters* inputParams){
	char line[MAX_LINE_SIZE];
	int lineCount = 0;	
	int numInitFiles = 0;
	int incCounter = 0;
	struct recordInfo* prevRecord = NULL;
	struct recordInfo* curRecord;
	struct recordInfo* recordArray[MAX_INIT_TIMESTAMPS];
	//Print First Record
	while(fgets(line, sizeof(line),fileInput) != NULL){
		//Header line of log
		if( 0 == lineCount )
		{
			fprintf(fileOutput,"Timestamp");
			writeHeaderLine(fileOutput,inputParams);			
			fprintf(fileOutput,"\n");
			lineCount++;
			continue;	
		}

		//Allocate space for current record struct
		curRecord = (struct recordInfo*)malloc(sizeof(struct recordInfo));
		
		//Epoch time based on date and time fields
		char* yearDate = strtok(line,"\t");
		char* hourDate = strtok(NULL,"\t");
		time_t curTime = determineEpochTime(yearDate,hourDate);
		curRecord->epochTime = curTime;

		//Case where the specified startTime is not yet reached
		if(difftime(curTime,inputParams->startTime) < 0)
		{
			free(curRecord);
			continue;
		}
		else
		{
			//do nothing
		}
		//Case where the specified endTime is reached before the EOF
		if(difftime(curTime,inputParams->endTime) >= 1)
		{
			free(curRecord);
			if(NULL != prevRecord)
			{
				free(prevRecord);
			}
			else
			{
				//do nothing
			}
			return;
		}
		else
		{
			//do nothing
		}

		//Fills in logArgs
		tokenizeLine(inputParams,curRecord);

		//The first second of the log
		if( 0 == difftime(inputParams->startTime,curTime) )
		{
			recordArray[numInitFiles] = curRecord; 
			recordArray[numInitFiles+1] = NULL; 
			numInitFiles++;
		}
		else
		{
			//write the first time block, then frees & nullifies so that it is only run once
			if( NULL != recordArray[0] )
			{
				prevRecord = writeFirstRecord(recordArray,numInitFiles,fileOutput,inputParams->AT_DEF,inputParams->UTC);
			}
			else
			{
				//do nothing
			}
			//General Line Case - previous record gets checked with current record
			//incCounter is used to increment the second fraction accurately
			if( 0  != difftime(curRecord->epochTime,prevRecord->epochTime) )
			{
				incCounter = 0;
			}
			else
			{
				incCounter++;
				if(inputParams->AT_DEF == ATP_NUM)
				{
					incCounter++;
				}
				else
				{
					//do nothing
				}
			}
			curRecord->secondFraction = incCounter;

			//check difference between cur and prev - if difference; write to .csv file
			if(changeInRecords(prevRecord,curRecord))
			{
				writeRecordInfo(curRecord,fileOutput,inputParams->UTC);
			}
			else
			{
				//do nothing
			}
			free(prevRecord);
			prevRecord = curRecord;
		}
		lineCount++;
	}
	//last free not accounted for within the while loop
	free(prevRecord);
}

//Iterates over recordArray, and updates the fraction second based on how many lines exist in the first second
//Writes the first record, all other records are only written if there is change between records
//Frees all other records but the last one for reference to the general case
struct recordInfo* writeFirstRecord(struct recordInfo* recordArray[],int numInitFiles,FILE* fileOutput,int AT_DEF,int UTC){
	struct recordInfo** prev;
	for(struct recordInfo** temp = recordArray;*temp != NULL;temp++){
		numInitFiles = updateSecondFraction(*temp,numInitFiles,AT_DEF);
		if(*temp == recordArray[0] || changeInRecords(*prev,*temp)) 
		{
			writeRecordInfo(*temp,fileOutput,UTC);
		}
		else
		{
			//do nothing
		}
		prev = temp;
	}
	for(struct recordInfo** temp = recordArray;temp != prev;temp++){
		free(*temp);	
		*temp = NULL;
	}
	return *prev;
}

//Iterates over the array for argument values & compares each value by string index-wise
//returns true if there is any change AT ALL -- false if otherwise
bool changeInRecords(struct recordInfo* prevRecord,struct recordInfo* curRecord){
	//Any logs exhibit change
	for(int i = 0; strcmp(curRecord->logArgs[i],"\0") ; i++){
		if(strcmp(prevRecord->logArgs[i],curRecord->logArgs[i])) 
		{
			return true;
		}
		else
		{
			//do nothing
		}
	}

	return false;
}

//uses the 'strftime()' function in 'time.h' to convert an epoch time to a string
//Each record struct has an epoch time that must be converted to a string for human-reading
char* convertEpochToString(struct recordInfo* curRecord,char UTCTimestamp[],int UTC){
	struct tm* curTMstruct = localtime(&(curRecord->epochTime));
	curTMstruct->tm_hour -= UTC;
	curTMstruct->tm_isdst = -1;
	mktime(curTMstruct);
	//format is YYYY-MM-DD HH:MI:SS
	if( 0 != strftime(UTCTimestamp,MAX_STRING_SIZE,"%Y-%m-%d %H:%M:%S",curTMstruct) )
	{
		return UTCTimestamp;
	}
	else {
		printf("Error: Converting Record %p Of Time %d to String was Unsuccessful\n",curRecord,curRecord->epochTime);
		printf("%s \n",UTCTimestamp);
		exit(EXIT_FAILURE);
	}
}

//Writes the timestamp & the arguments of the curRecord struct
//Only called if there is change with the previous record, or if it is the first record in the log file
void writeRecordInfo(struct recordInfo* curRecord,FILE* fileOutput,int UTC){
	//Convert epoch time to string
	char UTCTimestamp[MAX_STRING_SIZE];
	fprintf(fileOutput,"%s.%d",convertEpochToString(curRecord,UTCTimestamp,UTC),curRecord->secondFraction);

	for(int i = 0; i < sizeof(curRecord->logArgs)/sizeof(curRecord->logArgs[0]);i++){
		if(!strcmp(curRecord->logArgs[i],"\0"))
		{
			fprintf(fileOutput,"\n");
			return;
		}
		else
		{
			//do nothing
		}
		fprintf(fileOutput,",%s",curRecord->logArgs[i]);
	}
}

//updates the fraction based on the block containing the first second
//ATO Logs Inc by 0.1 seconds 
//ATP Logs Inc by 0.2 seconds
int updateSecondFraction(struct recordInfo* curRecord,int numInitFiles,int AT_DEF){
	int logInc = 1;
	if( ATP_NUM == AT_DEF )
	{
		logInc = 2;
	}
	else
	{
		//do nothing
	}
	curRecord->secondFraction = 10 - (logInc * numInitFiles);
	return --numInitFiles;
}

//Function that loops over fileArray to write all changing records of each file
//Each input will have its own separate output file with the changing records
//These will then get parsed to be concatenated (if multiple)
void parseLogFile(struct Parameters* inputParams){
	for(int i = 0; i < inputParams->fileC; i++){
		FILE* fileInput = inputParams->fileArray[i].fileInput;
		FILE* fileOutput = inputParams->fileArray[i].fileOutput;

		if( NULL == fileInput )
		{
			printf("Unable to open Input files\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			//do nothing
		}
	
		if( NULL == fileOutput )
		{
			printf("Unable to open Output files\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			//do nothing
		}
	
		writeChangingRecords(fileInput, fileOutput, inputParams);
		fclose(fileInput);
	}
}

//Checks if input files are part of different cores
//returns the core number if the cores are unanimous
//returns 0 to signify some files are of core 1 & some are of core 2
int checkMultCores(struct Parameters* inputParams){
	for(int i = 1; i < inputParams->fileC; i++){
		if(inputParams->fileArray[i].core != inputParams->fileArray[i-1].core)
		{
			return CORE_MULT;
		}
		else
		{
			//do nothing
		}
	}
	return inputParams->fileArray[0].core;
}

//Prints 'times' many commas to the output
//Necessary for spacing in .csv files between different cores
void repeatCommas(FILE* outputFile,int times){
	for(int i = 0; i < times; i++){
		fprintf(outputFile,",");
	}
}

//Prints out the core number columns to the .csv
//Core 1 & Core 2 OR Core #
void writeCoreHeader(FILE* outputFile, int multCores, int argC){
	fprintf(outputFile,",Core %d", (!multCores ? CORE_ONE : multCores));
	repeatCommas(outputFile,(argC - 1));

	if(!multCores)
	{
		fprintf(outputFile,",Core %d", CORE_TWO);
		repeatCommas(outputFile,(argC - 1));
	}
	else
	{
		//do nothing
	}
	fprintf(outputFile,"\n");
}

//Loops over files in fileArray & prints their outputs to outputFile .csv
//Spacing adjusted if files deal with multiple cores 
void concatFiles(FILE* outputFile, struct Parameters* inputParams){
	for(int i = 0; i < inputParams->fileC;i++){
		char line[MAX_LINE_SIZE];
		int lineCount = 0;
		rewind(inputParams->fileArray[i].fileOutput);
		while(fgets(line, sizeof(line),inputParams->fileArray[i].fileOutput) != NULL){
			if(!lineCount)
			{
				lineCount++;
				continue;
			}
			else if( 1 == lineCount )
			{
				fprintf(outputFile,"%s.%s,",inputParams->fileArray[i].fileName,TXT_SUFFIX);
			}	
			else
			{
				fprintf(outputFile,",");
			}

			//Dealing with multiple cores + file is part of core 2 -> spacing must be adjusted with commas
			if(!inputParams->multCores && (CORE_TWO == inputParams->fileArray[i].core))
			{
				//Print the timestamp -- the first 21 chars
				fprintf(outputFile,"%.21s",line);
				//Print empty commas
				repeatCommas(outputFile,inputParams->argC);
				//Print the rest of the data
				fprintf(outputFile,"%s",line+CHARS_AFTER_TIMESTAMP);
			}
			else
			{
				line[strlen(line)-1]='\0';
				fprintf(outputFile,"%s",line);
				if(!inputParams->multCores)
				{
					repeatCommas(outputFile,inputParams->argC);
				}
				fprintf(outputFile,"\n");
			}
			lineCount++;
		}
		fclose(inputParams->fileArray[i].fileOutput);
		remove(inputParams->fileArray[i].filePath);
	}
}

//Takes the outputFileName & creates a file that will contain all other input files
//Adjusts for spacing, core specificity, file name, and includes all changing records of all files
void makeCombinedOutput(struct Parameters* inputParams,char* outputFileName){
	FILE* outputFile = fopen(outputFileName,"w");
	
	inputParams->multCores = checkMultCores(inputParams);
	fprintf(outputFile,"%s,",outputFileName+strlen(inputParams->dirPath));
	writeCoreHeader(outputFile,inputParams->multCores,inputParams->argC);

	fprintf(outputFile,",Timestamp");
	writeHeaderLine(outputFile,inputParams);
	if(!inputParams->multCores)
	{
		writeHeaderLine(outputFile,inputParams);
	}
	else
	{
		//do nothing
	}
	fprintf(outputFile,"\n");

	concatFiles(outputFile,inputParams);

	fclose(outputFile);
}
