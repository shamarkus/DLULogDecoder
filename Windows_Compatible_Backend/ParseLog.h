#ifndef PARSELOG_H_
#define PARSELOG_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>

#define ATP_DEF "ATP"
#define ATO_DEF "ATO"
#define TXT_SUFFIX "txt"
#define CSV_SUFFIX "csv"
#define AT_UNDEFINED_NUM 2
#define ATP_NUM 1
#define ATO_NUM 0
#define ATP_STR_NUM "1"
#define ATO_STR_NUM "0"
#define MAX_COLUMN_SIZE 3096
#define MAX_STRING_SIZE 512
#define MAX_TIME_STRING_SIZE 25
#define MAX_LINE_SIZE 131072
#define UTC0_TIME "UTC+0" 
#define UTC4_TIME "EDT"
#define UTC5_TIME "EST"
#define UTC0_TIME_NUM 0 
#define UTC4_TIME_NUM 4
#define UTC5_TIME_NUM 5
#define MAX_INIT_TIMESTAMPS 11
#define CORE_MULT 0
#define CORE_ONE 1
#define CORE_TWO 2
#define GENERAL_OUTPUT_NAME "DLUOutputFile.csv"
#define ARG_NUM_AFTER_DATE_TIME 3
#define CHARS_AFTER_TIMESTAMP 21
//Struct & Global Variable Declarations
struct fileInfo {
	char fileName[MAX_STRING_SIZE];
	char filePath[MAX_STRING_SIZE];
	int core;

	FILE* fileInput;
	FILE* fileOutput;
};

struct Parameters { 
	int AT_DEF;
	int UTC;
	time_t startTime;
	time_t endTime;

	int argC;
	int argN[MAX_COLUMN_SIZE];
	char Args[MAX_COLUMN_SIZE][MAX_STRING_SIZE];
	char dirPath[MAX_STRING_SIZE];

	struct fileInfo* fileArray;
	int multCores;
	int fileC;
};

struct recordInfo {
	time_t epochTime;
	int secondFraction;
	char logArgs[MAX_COLUMN_SIZE][MAX_STRING_SIZE];
};

//Function declarations
time_t determineEpochTime(char* yearDate,char* hourDate);

const char *strptime_I(const char *buf, const char *fmt, struct tm *tm);

bool changeInRecords(struct recordInfo* prevRecord,struct recordInfo* curRecord);

char* convertEpochToString(struct recordInfo* curRecord,char UTCTimestamp[],int UTC);

void writeRecordInfo(struct recordInfo* curRecord,FILE* fileOutput,int UTC);

struct recordInfo* writeFirstRecord(struct recordInfo* recordArray[],int numInitFiles,FILE* fileOutput,int AT_DEF, int UTC);

int updateSecondFraction(struct recordInfo* curRecord,int numInitFiles,int AT_DEF);

void tokenizeLine(struct Parameters* inputParams,struct recordInfo* curRecord);

void writeHeaderLine(FILE* fileOutput,struct Parameters* inputParams);			

void writeChangingRecords(FILE* fileInput,FILE* fileOutput,struct Parameters* inputParams);

void parseLogFile(struct Parameters* inputParams);

int checkMultCores(struct Parameters* inputParams);

void repeatCommas(FILE* outputFile,int times);

void writeCoreHeader(FILE* outputFile, int multCores, int argC);

void concatFiles(FILE* outputFile, struct Parameters* inputParams);

void makeCombinedOutput(struct Parameters* inputParams,char* outputFileName);

#endif
