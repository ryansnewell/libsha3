#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int B; //width of the state. b = r + c
int R; //bit-rate, length of one message block. r = 1344 or r = 1088
int C; //capacity. c = 256 or c = 512
int OUTPUT_LENGTH; //decides length of output and if using SHA-2 replacement mode or variable output mode
int W; //Widht of the state array. Dimensions are 5x5xW;
int NUM_ROUNDS;
int mSize;
int padSize;
uint8_t* PAD;
FILE* m; //input meassge file

uint64_t round_constant[24] = { 0x0000000000000001,
							    0x0000000000008082,
								0x800000000000808A,
								0x8000000080008000,
								0x000000000000808B,
								0x0000000080000001,
								0x8000000080008081,
								0x8000000000008009,
								0x000000000000008A,
								0x0000000000000088,
								0x0000000080008009,
								0x000000008000000A,
								0x000000008000808B,
								0x800000000000008B,
								0x8000000000008089,
								0x8000000000008003,
								0x8000000000008002,
								0x8000000000000080,
								0x000000000000800A,
								0x800000008000000A,
								0x8000000080008081,
								0x8000000000008080,
								0x0000000080000001,
								0x8000000080008008};



void loadFile(char* filename){
	m = fopen(filename, "rb");
	if(m == NULL){
		printf("Error in opening file, errno: %i\n", errno);
		exit(-1);
	}

	//TODO This method does not always work correctly 
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
		printf("Pad size needed: %i\n", padSize);
	}

	//TODO Rework this so instead of this dumb block, just set padding memory to all 0's
	//		Then XOR first byte with its needed value and last byte with 1.
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

	printf("Pad set\n");
}

void setupMessageAndPad(uint8_t* message){
	for(int i = 0; i < mSize + padSize; i++){
		if(i < mSize){
			message[i] = fgetc(m);
		} else {
			message[i] = PAD[i-mSize];
		}
	}
	free(PAD);
	fclose(m);
}

//Get the n-th block from message(of size R), concat 0's(of size capacity C),
//	and XOR it with the current state.
//
//AKA absorb the next message block and capacity into the state
void absorb(uint64_t* state, uint8_t* message, int n){
	uint64_t tempLane = 0x0000000000000000;
	
	//Loop through x,y = 0,1,2,3,4, the indices of the state array
	//For every 64 bytes of the message block, XOR it with the state
	//
	//Since the message block is indexed by byte, looping through 64 bytes at a time,
	//	store it in tempLane, then XOR it into the state
	int gatheredBytes = 0;
	for(int x = 0; x < 5; x++){
		for(int y = 0; y < 5; y++){
			for(int z = 0; z < W; z++){
				if(gatheredBytes < R){
					tempLane = tempLane << 8;
					tempLane = tempLane ^ message[n*R + gatheredBytes];
					gatheredBytes++;
				}else{
					tempLane = tempLane << 8;
					tempLane = tempLane ^ 0b0000000;
					gatheredBytes++;
				}
			}
			
			state[x*5 + y] = state[x*5 + y] ^ tempLane;
			tempLane = 0x0000000000000000;
		}
	}
}

// Rotate input, word, by rot_amount bits.
uint64_t rot64(uint64_t word, int rot_amount){
	rot_amount = rot_amount % 64;
	return (word >> rot_amount) | (word << (64-rot_amount));
}

void theta(uint64_t*  state){
	uint64_t* C = malloc(5*sizeof(uint64_t));
	for(int sheet = 0; sheet < 5; sheet++){
		int x = sheet*5;
		C[sheet] = state[x] ^ state[x + 1] ^ state[x + 2] ^ state[x + 3] ^ state[x + 4]; 
	}
	
	uint64_t* D = malloc(5*sizeof(uint64_t));
	for(int x = 0; x < 5; x++){
		uint64_t c_rot = rot64(C[(x+1)%5], 1);
		D[x] = C[(x-1) % 5] ^ c_rot;

		for(int y = 0; y < 5; y++){
				state[x*5 + y] ^= D[x];
		}
	}
}

void pi(uint64_t* state){
	uint64_t* temp_B = malloc(25*sizeof(uint64_t));
	for(int i = 0; i < 25; i++){
		temp_B[i] = state[i];
	}
	

	for(int x = 0; x < 5; x++){
		for(int y = 0; y < 5; y++){
			int new_x = y % 5;
			int new_y = (2*x + 3*y) % 5;

			state[new_x*5 + new_y] = temp_B[x*5 + y];
		}
	}

	free(temp_B);
}

void rho(uint64_t* state){
	uint64_t* temp_B = malloc(25*sizeof(uint64_t));
	for(int i = 0; i < 25; i++){
		temp_B[i] = state[i];
	}
	
	int x = 1;
	int y = 0;
	for(int t = 0; t < 24; t++){
		int offset = ((t+1) * (t+2)) / 2;
		state[x*5 + y] = rot64(temp_B[x*5 + y], offset);
		
		x = y;
		y = 2*x + 3*y;
	}
	
	free(temp_B);
}

void chi(uint64_t* state){
	uint64_t* temp_B = malloc(25*sizeof(uint64_t));
	for(int i = 0; i < 25; i++){
		temp_B[i] = state[i];
	}

	for(int x = 0; x < 5; x++){
		for(int y = 0; y < 5; y++){
				state[5*x + y] = temp_B[5*x + y] ^ ( ~(temp_B[5*((x+1)%5) + y]) & temp_B[5*((x+2)%5) + y]);
		}
	}

	free(temp_B);
}

void iota(uint64_t* state, int round){
	state[0] = state[0] ^ round_constant[round - 1];
}

void keccak_f(uint64_t* state){	
	for(int n = 0; n < NUM_ROUNDS; n++){
		//printf("Starting theta step...\n");
		theta(state);
		//printf("Theta step complete. Starting rho step...\n");
		rho(state);
		//printf("Rho step complete. Starting pi step...\n");
		pi(state);
		//printf("Pi step complete. Starting chi step...\n");
		chi(state);
		//printf("Chi step complete. Starting iota step...\n");
		iota(state, n);
		//printf("Iota step complete. Starting clean up...\n");
	}
	//printf("Completed 24 rounds\n");
}


/*
 *Input:
 *	arg[1] = finename
 *	arg[2] = desired output length (bits)
*/
int main(int argc, char** argv){
	if(argc < 3){
		printf("Insufficient arguments\n");
		exit(-1);
	} else {
		printf("arg[1] = %s\n", argv[1]);
		printf("arg[2] = %s\n", argv[2]);
	}

	B =  200; //1600 bits = 200 bytes
	C =  32;  //256  bits = 32 bytes //TODO change to non static
	R = B - C;
	W = B / 25; //8 bytes = uint64_t or 8 bytes
	NUM_ROUNDS = 24; //TODO change to non static???
	OUTPUT_LENGTH = atoi(argv[2]) / 8;

	loadFile(argv[1]);
	setPAD();
	
	int messageSize = mSize + padSize;
	uint8_t* message = malloc(messageSize);
	uint64_t* state  = malloc(25*sizeof(uint64_t));
	memset(state, 0, 25);

	if(!message){
		printf("Error allocating memory\n");
	}

	setupMessageAndPad(message);
	
	for(int p = 0; p < messageSize / R; p++){
		//printf("Absorbing message block: %i\n", p);
		absorb(state, message, p);
		keccak_f(state);
	}
	
	printf("Completed successfully. Exiting\n");
	return 0;
}
