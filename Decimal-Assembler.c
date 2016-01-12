/*
 * ~~~ Ariel Young @ Illinois Institute of Technology ~~~
 *
 * Inspired by the Little-man computer:
 * https://en.wikipedia.org/wiki/Little_man_computer
 *
 * I did this project in order to better understand
 * computer architecture, without having to care
 * about binary/hex data.
 *
 * I wrote this code when I just started learning C
 * and it needs a lot of refactoring. If you would
 * like to see a cleaner and better version of this
 * code working with actual binaries and not a decimal
 * object code, please take a look at my LC-3 Simulator.
 */

#include <stdio.h>
#include <stdlib.h>

/* Assembler declarations */
#define NREG 10
#define MEMLEN 100

/* CPU & Memory State */
/* One day I will refactor the global
 * variables below and package them
 * into a struct that I will pass
 * around in functions... but today
 * is not that day */
int pc;
int ir;
int running;     /* Is the CPU running? (1 yes, 0 no) */
int reg[NREG];   /* CPU registers */
int mem[MEMLEN]; /* memory */

/* Function Prototypes */

/* Initizialization */
FILE *get_datafile(int argc, char *argv[]);
void initialize_control_unit(int reg[], int nreg);
void initialize_memory(int argc, char *argv[], int mem[], int memlen);

/* Dumping info (program + debug) */
void dump_control_unit(int pc, int ir, int running, int reg[], int nreg);
void dump_memory(int mem[], int memlen);
void dump_registers(int reg[], int nreg);
void help_message(void);

/* Manipulate CPU */
int read_execute_command(int reg[], int nreg, int mem[], int memlen);
int execute_command(char cmd_char, int reg[], int nreg, int mem[], int memlen);
void one_instruction_cycle(int reg[], int nreg, int mem[], int memlen);
void many_instruction_cycles(int nbr_cycles, int reg[], int nreg, int mem[], int memlen);
void exec_HLT();


int main(int argc, char *argv[])
{
  printf("SDC Simulator\n");

  /* initialize everything */
  initialize_control_unit(reg, NREG);
  initialize_memory(argc, argv, mem, MEMLEN);

  char *prompt = "> ";
  printf("\nBeginning execution; type h for help\n%s", prompt);
  int done = read_execute_command(reg, NREG, mem, MEMLEN);
  while (!done) {
    printf("%s", prompt);
    done = read_execute_command(reg, NREG, mem, MEMLEN);
  }

  /* Dump everything when done */
  printf("Termination\n");
  dump_control_unit(pc, ir, running, reg, NREG);
  printf("\n");
  dump_memory(mem, MEMLEN);

  return 0;
}

void initialize_control_unit(int reg[], int nreg)
{
  int i;
  for (i = 0; i < nreg; i++)
    reg[i] = 0;

  pc = 0;
  running = 1;
  ir = 0;

  printf("\nInitial control unit:\n");
  dump_control_unit(pc, ir, running, reg, nreg);
  printf("\n");
}

void initialize_memory(int argc, char *argv[], int mem[], int memlen)
{
  FILE *datafile = get_datafile(argc, argv);

  int value_read, words_read, loc = 0, done = 0;

  char *buffer = NULL;
  size_t buffer_len = 0, bytes_read = 0;


  /* Fetch first instruction */
  bytes_read = getline(&buffer, &buffer_len, datafile);
  while (bytes_read != -1 && !done) {

    words_read = sscanf(buffer, "%d", &value_read);

    /* If beginning was not an integer (junk) discard it and go on
     * else populate memory location and advance to the next one.
     * Always check that you have not hit the memory bound */
    if (words_read == 0 || words_read == -1) {
      bytes_read = getline(&buffer, &buffer_len, datafile);
      continue;
    }

    if (loc > memlen) {
      printf("The memory location is out of range");
      done = 1;
    } else if (value_read > 9999 || value_read < -9999) {
      printf("Hit sentinel, quitting loop");
      done = 1;
    } else {
      mem[loc++] = value_read;
      bytes_read = getline(&buffer, &buffer_len, datafile);
    }
  }

  /* buffer is not needed anymore */
  free(buffer);

  /* zero-out the rest of the memory locations */
  while (loc < memlen) {
    mem[loc] = 0;
    loc++;
  }

  dump_memory(mem, memlen);
}

FILE *get_datafile(int argc, char *argv[])
{
  char *datafile_name;

  /* if a datafile is not provided, use the default */
  if (argv[1] != NULL)
    datafile_name = argv[1];
  else
    datafile_name = "default.sdc";

  FILE *datafile = fopen(datafile_name, "r");

  if (datafile == NULL) {
    printf("Failed to open: %s\n", datafile_name);
    exit(EXIT_FAILURE);
  }

  return datafile;
}

void dump_control_unit(int pc, int ir, int running,
                       int reg[], int nreg)
{
  printf("PC:\t %d \t IR: \t %d Running: \t %d \n", pc, ir, running);
  dump_registers(reg, nreg);
}

void dump_memory(int mem[], int memlen)
{
  int loc = 0;
  int i;

  printf("\n%d: ", loc);
  for (i = 0; i < memlen; i++) {
    if (((i) % 10) == 0) {
      printf("\n%d: ", loc);
      loc += 10;
    }
    printf("\t%4d", mem[i]);
  }
  printf("\n");
}

void dump_registers(int reg[], int nreg)
{
  int i;

  for (i = 0; i < nreg / 2; i++) {
    printf("R%d: %d \t", i, reg[i]);
  }
  printf("\n");

  for (i = nreg / 2; i < nreg; i++) {
    printf("R%d: %d \t", i, reg[i]);
  }
}

int read_execute_command(int reg[], int nreg, int mem[], int memlen)
{
  char *cmd_buffer = NULL;
  size_t cmd_buffer_len = 0, bytes_read = 0;

  int nbr_cycles;
  char cmd_char;
  size_t words_read;

  int done = 0;

  /* Get user input */
  bytes_read = getline(&cmd_buffer, &cmd_buffer_len, stdin);

  /* Did we hit the end of file? */
  if (bytes_read == -1) {
    done = 1;
  }

  words_read = sscanf(cmd_buffer, "%d", &nbr_cycles);

  /* If a char was read then try to execute the according
   * command, else if an integer was entered, try to
   * execute this number of cycles */
  if (words_read == 0) {
    sscanf(cmd_buffer, "%c", &cmd_char);
    done = execute_command(cmd_char, reg, nreg, mem, memlen);
  } else {

    /* Check first is the number of cycles is invalid.
     * If yes then execute one or more instructions */
    if (nbr_cycles < 1 ||
        nbr_cycles > memlen ||
        nbr_cycles > (memlen - pc)) {
      printf("%d is an invalid number of cycles!!", nbr_cycles);
    } else if (nbr_cycles == 1) {
      one_instruction_cycle(reg, nreg, mem, memlen);
    } else {
      many_instruction_cycles(nbr_cycles, reg, nreg, mem, memlen);
    }
  }

  /* Command buffer not needed anymore */
  free(cmd_buffer);

  return done;
}

int execute_command(char cmd_char, int reg[], int nreg,
                    int mem[], int memlen)
{

  if (cmd_char == '?' || cmd_char == 'h')
    help_message();

  switch (cmd_char) {
  case 'd':
    dump_control_unit(pc, ir, running, reg, nreg);
    dump_memory(mem, memlen);
    return 0;
    break;

  case 'q':
    return 1;
    break;

  case '\n':
    one_instruction_cycle(reg, nreg, mem, memlen);
    return 0;
    break;

  default:
    printf("Please enter a valid character");
    break;
  }

  return 0;
}

void help_message(void)
{
  printf("Choose from the following menu\n");
  printf("d: dump control unit\n");
  printf("q: quit the program \n");
  printf("\'\\n: one instruction \n");
  printf("Type in a number for the number of cycles");
}

void many_instruction_cycles(int nbr_cycles, int reg[], int nreg,
                             int mem[], int memlen)
{
  if (nbr_cycles <= 0) {
    printf("You have indicated to run an invalid amount(%d) of times",
           nbr_cycles);
    return;
  } else if (running == 0) {
    printf("The CPU is not running");
    return;

  } else {
    int i;
    for (i = 0; i < nbr_cycles && running != 0; i++) {
      one_instruction_cycle(reg, nreg, mem, memlen);
    }
  }
}

void one_instruction_cycle(int reg[], int nreg,
                           int mem[], int memlen)
{
  int reg_R, addr_MM, instr_sign, opcode;

  /* Check if CPU is running */
  if (running == 0) {
    printf("The CPU is not running");
    return;
  }

  /* Make sure that we didn't hit any boundaries */
  if (pc >= memlen) {
    printf("Program counter out of range");
    exec_HLT();
  }

  /* Get instruction and increment PC */
  int instr_loc = pc;
  ir = mem[pc++];

  /* Check instruction sign */
  if (ir < 0) {
    instr_sign = -1;
    ir *= -1;
  } else if (ir > 0) {
    instr_sign = 1;
  }

  /* Decode instruction into opcode, register, and memory address */
  opcode = ir / 1000;
  reg_R = (ir % 1000) / 100;
  addr_MM = ir % 100;

  printf("At %02d instr %d %d %02d: ",
         instr_loc, opcode, reg_R, addr_MM);

  switch (opcode) {
  /* HALT */
  case 0:
    exec_HLT();
    break;

  /* LOAD */
  case 1: {
    reg[reg_R] = mem[addr_MM];
  } break;

  /* STORE */
  case 2: {
    mem[addr_MM] = reg[reg_R];
  } break;

  /* ADD-IM-MM */
  case 3: {
    reg[reg_R] += mem[addr_MM];
  } break;

  /* NOT */
  case 4: {
    reg[reg_R] = (-reg[reg_R]);
  } break;

  /* LOAD IMMEDIATE */
  case 5: {
    reg[reg_R] = addr_MM * instr_sign;
  } break;

  /* ADD IMMEDIATE */
  case 6: {
    reg[reg_R] += addr_MM * instr_sign;
  } break;

  /* JUMP */
  case 7: {
    pc = addr_MM;
  } break;

  /* BRANCH CONDITIONAL */
  case 8: {
    if (reg[reg_R] > 0 && instr_sign > 0) {
      pc = addr_MM;
    } else if (reg[reg_R] < 0 && instr_sign < 0)
      pc = addr_MM;
  } break;

  /* I/O Subroutines */
  case 9: {
    switch (reg_R) {
    /* GETCHAR */
    case 0: {
      printf("enter a character>> ");
      int k = getchar();
      reg[0] = k;
    } break;

    /* PRINTCHAR */
    case 1: {
      printf("%c\n", reg[0]);
    } break;

    /* PRINT-STRING */
    case 2: {
      while (mem[addr_MM] != 0) {
          printf("%c \n", mem[addr_MM++]);
      }
      printf("\n");
    } break;

    /* DUMP CU */
    case 3: {
      dump_control_unit(pc, ir, running, reg, nreg);
    } break;

    /* DUMP MEM */
    case 4: {
      dump_memory(mem, memlen);
    } break;

    default:
      return;
      break;
    }
  } break;
  default:
    printf("Bad opcode!? %d\n", opcode);
  }
}

void exec_HLT()
{
  printf("HALT\nHalting\n");
  running = 0;
}

