#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
int B; //width of the state. b = r + c
int R; //bit-rate, length of one message block. r = 1344 or r = 1088
int C; //capacity. c = 256 or c = 512
int OUTPUT_LENGTH; //decides length of output and if using SHA-2 replacement mode or variable output mode
int W; //Widht of the state array. Dimensions are 5x5xW;
int mSize;
int padSize;

unsigned char* PAD;

FILE* m; //input meassge file

void loadFile(char* filename){
	printf("Opening %s\n", filename);

	m = fopen(filename, "rb");
	if(m == NULL){
		printf("Error in opening file, errno: %i\n", errno);
		exit(-1);
	}

	fseek(m, 0, SEEK_END);
	mSize = ftell(m);
	fseek(m, 0, SEEK_SET);
	
	printf("Filename: %s, file size: %i\n", filename, mSize);
}

void setPAD(){
	padSize = R - ((mSize) % R);
	PAD = malloc(padSize);
	if(!PAD){
		printf("Error allocating memory for PAD\n");
		exit(-1);
	} else {
		printf("\nPad size needed: %i\n", padSize);
	}

	//TODO Rework this so instead of this dumb block, just set padding memory to all 0's
	//		Then XOR first byte with it's needed value and last byte with 1.
	//		That way even if it's only one byte it still gets the needed values.
	if(padSize == 1){
		PAD[0] = 0b11101001;
	} else if(padSize == 2){
		PAD[0] = 0b11101000;
		PAD[1] = 0b00000001;
	} else {
		PAD[0] = 0b11101000;
		for(int i = 1; i < padSize - 1; i++){
			PAD[i] = 0b00000000;
		}
		PAD[padSize - 1] = 0b0000001;
	}

	printf("\n");
}

void cleanup(){
	free(PAD);

	fclose(m);
}

/*
 *Input:
 *	arg[1] = finename
 *	arg[2] = desired output length
*/
int main(int argc, char** argv){
	if(argc < 3){
		printf("Insufficient arguments\n");
		exit(-1);
	} else {
		printf("arg[1] = %s\n", argv[1]);
		printf("arg[2] = %s\n", argv[2]);
	}

	B = 1600;
	R = 1088; //TODO change to non static
	C = B - R;
	W = B / 25;
	OUTPUT_LENGTH = atoi(argv[2]);
	
	loadFile(argv[1]);
	setPAD();
	
	
	cleanup();
	return 0;
}
