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

unsigned long long int round_constant[24] = { 0x0000000000000001,
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
}

// state is a 3d array of dimensions 5x5xW
// postition [i][j][k] is accessed by [x*5*W + j*W + k]
void theta(uint8_t*  state){
	uint8_t* C = malloc(5*W);
	for(int sheet = 0; sheet < 5; sheet++){
		for(int z = 0; z < W; z++){
			int x = sheet*5*W;
			C[x*5 + z] = state[x + z] ^ state[x + z + W] ^ state[x + z + W*2] ^ state[x + z + W*3] ^ state[x + z + W*4]; 
		}
	}

	uint8_t last = 0;
	uint8_t next = 0;
	uint8_t D[5][W];
	memset(D, 0, 5*W);
	for(int x = 0; x < 5; x++){
		for(int z = 0; z < W; z++){
			D[(x+1)%5][z] ^= C[5*x+z];
			next = C[5*x+z] & 1;
			C[5*x+z] = (C[5*x+z] >> 1) | (last << 8);
			last = next;
			D[(x+4)%5][z] ^= C[5*x+z];
		}
	}

	for(int x = 0; x < 5; x++){
		for(int y = 0; y < 5; y++){
			for(int z = 0; z < W; z++){
				state[x*W*5 + y*W + z] ^= D[x][z];
			}
		}
	}
}

void pi(uint8_t* state){
	uint8_t* temp_B = malloc(25*W);
	for(int i = 0; i < 25*W; i++){
		temp_B[i] = state[i];
	}
	
	for(int z = 0; z < W; z++){
		for(int x = 0; x < 5; x++){
			for(int y = 0; y < 5; y++){
				int new_x = y % 5;
				int new_y = (2*x + 3*y) % 5;
				
				state[new_x*W*5 + new_y*W + z] = temp_B[x*W*5 + y*W + z];
			}
		}
	}

	free(temp_B);
}

void rho(uint8_t* state){
	uint8_t* temp_B = malloc(25*W);
	for(int i = 0; i < 25*W; i++){
		temp_B[i] = state[i];
	}
	
	int x = 1;
	int y = 0;
	for(int t = 0; t < 24; t++){
		int offset = ((t+1) * (t+2)) / 2;
		// state(x,y) = ROT(temp_B, offset)
		
		for(int z = 0; z < W; z++){
			state[5*x*W + 5*y + z] = temp_B[5*x*W + 5*y + ((z + offset) % W)];
		}

		x = y;
		y = 2*x + 3*y;
	}
	
	free(temp_B);
}

void chi(uint8_t* state){
	uint8_t* temp_B = malloc(25*W);
	for(int i = 0; i < 25*W; i++){
		temp_B[i] = state[i];
	}

	for(int x = 0; x < 5; x++){
		for(int y = 0; y < 5; y++){
			for(int z = 0; z < W; z++){
				state[5*x*W + 5*y + z] = temp_B[5*x*W + 5*y + z] ^ ( ~(temp_B[5*W*((x+1)%5) + 5*y + z]) & temp_B[5*W*((x+2)%5) + 5*y + z] );
			}
		}
	}

	free(temp_B);
}

void iota(uint8_t* state){
	printf("\tsizeof(uint8_t)=%lu\n", sizeof(uint8_t));
	for(int z = W - 1; z >= 0; z--){
		unsigned long long int rconst = (round_constant[NUM_ROUNDS - 1] >> ((W-z) * sizeof(uint8_t)));
		printf("\tz: %i\tpre-Xor constant: %llu\n", z, rconst);
		state[z] = state[z] ^ rconst;
	}
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

	B =  1600;
	C =  256; //TODO change to non static
	R = B - C;
	W = B / 25;
	NUM_ROUNDS = 24; //TODO change to non static???
	OUTPUT_LENGTH = atoi(argv[2]);

	loadFile(argv[1]);
	setPAD();
	
	uint8_t* state   = malloc(25*W);
	uint8_t* message = malloc(mSize + padSize);

	if(!message){
		printf("Error allocating memory\n");
	}

	setupMessageAndPad(message);
	
	printf("Setup complete. Starting theta step...\n");
	theta(state);

	printf("Theta step complete. Starting rho step...\n");
	rho(state);
	
	printf("Rho step complete. Starting pi step...\n");
	pi(state);

	printf("Pi step complete. Starting chi step...\n");
	chi(state);

	printf("Chi step complete. Starting iota step...\n");
	iota(state);

	printf("Iota step complete. Starting clean up...\n");
	cleanup();

	printf("Completed successfully. Exiting\n");
	return 0;
}
