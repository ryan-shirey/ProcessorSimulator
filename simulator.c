#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "instruction.h"

// Forward declarations for helper functions
unsigned int get_file_size(int file_descriptor);
unsigned int* load_file(int file_descriptor, unsigned int size);
instruction_t* decode_instructions(unsigned int* bytes, unsigned int num_instructions);
unsigned int execute_instruction(unsigned int program_counter, instruction_t* instructions, 
				 int* registers, unsigned char* memory);
void print_instructions(instruction_t* instructions, unsigned int num_instructions);
void error_exit(const char* message);

// 17 registers
#define NUM_REGS 17
// 1024-byte stack
#define STACK_SIZE 1024

int main(int argc, char** argv)
{
  // Make sure we have enough arguments
  if(argc < 2)
    error_exit("must provide an argument specifying a binary file to execute");

  // Open the binary file
  int file_descriptor = open(argv[1], O_RDONLY);
  if (file_descriptor == -1) 
    error_exit("unable to open input file");

  // Get the size of the file
  unsigned int file_size = get_file_size(file_descriptor);
  // Make sure the file size is a multiple of 4 bytes
  // since machine code instructions are 4 bytes each
  if(file_size % 4 != 0)
    error_exit("invalid input file");

  // Load the file into memory
  unsigned int* instruction_bytes = load_file(file_descriptor, file_size);
  close(file_descriptor);

  unsigned int num_instructions = file_size / 4;

  // Allocate and decode instructions (left for you to fill in)
  instruction_t* instructions = decode_instructions(instruction_bytes, num_instructions);

  

  // Allocate and initialize registers
  int* registers = (int*)malloc(sizeof(int) * NUM_REGS);
  int i;
  for(i=0;i<NUM_REGS;i++){
    registers[i] = 0 ;
  }
  //allocates stack memory and sets %esp to 1024
  unsigned char* memory = (char*)calloc(STACK_SIZE, sizeof(char));
  registers[6] = STACK_SIZE;
 

  // Run the simulation
  unsigned int program_counter = 0;

  // program_counter is a byte address, so we must multiply num_instructions by 4 to get the address past the last instruction
   while(program_counter != num_instructions * 4)
  {
    program_counter = execute_instruction(program_counter, instructions, registers, memory);
  }
  
  

  
  return 0;
}



/*
 * Decodes the array of raw instruction bytes into an array of instruction_t
 * Each raw instruction is encoded as a 4-byte unsigned int
*/
instruction_t* decode_instructions(unsigned int* bytes, unsigned int num_instructions)
{
  //allocate the memory needed to fill instruction list
  instruction_t* retval = malloc(num_instructions * sizeof(instruction_t));
  int i;
  for(i = 0; i<num_instructions;i++){
    //get the bytes needed in one variable
    unsigned int instruction = bytes[0];
    //isolate and assign needed bits for each part of struct
    unsigned char opcode = (instruction>>27);
    unsigned char reg1 = ((instruction<<5)>>27);
    unsigned char reg2 = ((instruction<<10)>>27);
    int16_t imm = ((instruction<<16)>>16);
    //create struct and assign it to proper section in retval
    retval[i] = (instruction_t){opcode,reg1,reg2,imm};
    //increment bytes
    bytes++;
  }
  return retval;
}


/*
 * Executes a single instruction and returns the next program counter
*/
unsigned int execute_instruction(unsigned int program_counter, instruction_t* instructions, int* registers, unsigned char* memory)
{
  // program_counter is a byte address, but instructions are 4 bytes each
  // divide by 4 to get the index into the instructions array
  instruction_t instr = instructions[program_counter / 4];
  unsigned int SF;
  unsigned int OF;
  switch(instr.opcode)
  {
    //0
  case subl:
    registers[instr.first_register] = registers[instr.first_register] - instr.immediate;
    break;
    //1
  case addl_reg_reg:
    registers[instr.second_register] = registers[instr.first_register] + registers[instr.second_register];
    break;
    //2
  case addl_imm_reg:
    registers[instr.first_register] = registers[instr.first_register] + instr.immediate;
    break;
    //3
  case imull:
    registers[instr.second_register] = registers[instr.first_register] * registers[instr.second_register];
    break;
    //4
  case shrl:
    registers[instr.first_register] = (registers[instr.first_register] >> 1)&0x7fffffff;
    break;
    //5
  case movl_reg_reg:
    registers[instr.second_register] = registers[instr.first_register];
    break;
    //6
  case movl_deref_ref:
    registers[instr.second_register] = *(int*)&memory[registers[6]-instr.immediate*sizeof(int)];
    break;
    //7
  case movl_reg_deref:
    *(int*)&memory[registers[6] - instr.immediate*sizeof(int)] = registers[instr.first_register];
    break;
    //8
  case movl_imm_reg:
    registers[instr.first_register] = (int)((instr.immediate<<16)>>16);
    break;
    //9
    case cmpl:
      ;
    long signedResult = (long)registers[instr.second_register]-(long)registers[instr.first_register];
    unsigned long unsignedResult = (unsigned int)registers[instr.second_register]-(unsigned int)registers[instr.first_register];
    int flagMask = 0;
    if(unsignedResult==0){
      flagMask+=64;
    }
    if((unsignedResult>>31)&1){
      flagMask+=128;
    }
    if((unsigned int)registers[instr.second_register]<(unsigned int)registers[instr.first_register]){
      flagMask+=1;
    }
    if(((signedResult<(long)0xffffffff80000000)||(signedResult>(long)0x000000007fffffffl))){
      flagMask+=2048;
    }
    
    registers[16]=flagMask;
    break;
    //10
  case je:
    if (registers[16]&0x40){
      program_counter += instr.immediate;
    }
    break;
    //11
  case jl:
    
    SF = (registers[16]&0x80);
    OF = registers[16]&0x800;
    if((SF&&!OF)||(!SF&&OF)){
      program_counter += instr.immediate;
    }
    break;
    //12
  case jle:
     
    SF = (registers[16]&0x80);
    OF = registers[16]&0x800;
    if(  ((SF&&!OF)||(!SF&&OF))||(registers[16]&0x40)  ){
      program_counter += instr.immediate;
    }
    break;
    //13
  case jge:
     
    SF = (registers[16]&0x80);
    OF = registers[16]&0x800;
    if(!(  (SF&&!OF)||(!SF&&OF)  ) ){
      program_counter += instr.immediate;
    }
    break;
    //14
  case jbe:
    if((registers[16]&0x40)||(registers[16]&0x1)){
      program_counter += instr.immediate;
    }
    break;
    //15
  case jmp:
    program_counter += instr.immediate;
    break;

    //16
  case call:
    registers[6] -= 4;
    *(int*)&memory[registers[6]] = program_counter+4;
    return program_counter + instr.immediate+4;
    break;
    //17
  case ret:
    if(registers[6] == 1024){
      exit(0);
    }
    program_counter = *(int*)&memory[registers[6]];
    registers[6]+=4;
    return program_counter;
    break;
    //18
  case pushl:
    registers[6]-=4;
    *(int*)&memory[registers[6]] = registers[instr.first_register];
    break;
    //19
  case popl:
    registers[instr.first_register] = *(int*)&memory[registers[6]];
    registers[6]+=4;
    break;
    //20
  case printr:
    printf("%d (0x%x)\n", registers[instr.first_register], registers[instr.first_register]);
    break;
    //21
  case readr:
    scanf("%d", &(registers[instr.first_register]));
    break;

  }
 
  // program_counter + 4 represents the subsequent instruction
  return program_counter + 4;
}



/*
 * Returns the file size in bytes of the file referred to by the given descriptor
*/
unsigned int get_file_size(int file_descriptor)
{
  struct stat file_stat;
  fstat(file_descriptor, &file_stat);
  return file_stat.st_size;
}

/*
 * Loads the raw bytes of a file into an array of 4-byte units
*/
unsigned int* load_file(int file_descriptor, unsigned int size)
{
  unsigned int* raw_instruction_bytes = (unsigned int*)malloc(size);
  if(raw_instruction_bytes == NULL)
    error_exit("unable to allocate memory for instruction bytes (something went really wrong)");

  int num_read = read(file_descriptor, raw_instruction_bytes, size);

  if(num_read != size)
    error_exit("unable to read file (something went really wrong)");

  return raw_instruction_bytes;
}

/*
 * Prints the opcode, register IDs, and immediate of every instruction, 
 * assuming they have been decoded into the instructions array
*/
void print_instructions(instruction_t* instructions, unsigned int num_instructions)
{
  printf("instructions: \n");
  unsigned int i;
  for(i = 0; i < num_instructions; i++)
  {
    printf("op: %d, reg1: %d, reg2: %d, imm: %d\n", 
	   instructions[i].opcode,
	   instructions[i].first_register,
	   instructions[i].second_register,
	   instructions[i].immediate);
  }
  printf("--------------\n");
}


/*
 * Prints an error and then exits the program with status 1
*/
void error_exit(const char* message)
{
  printf("Error: %s\n", message);
  exit(1);
}
