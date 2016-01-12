/*
 * ~~~ Ariel Young @ Illinois Institute of Technology ~~~
 *
 * Information around the design of LC-3 and its
 * instruction architecture can be found on the
 * link below:
 *
 * http://highered.mheducation.com/sites/0072467509/index.html
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Assembler declarations */
# define MEMLEN 65536
# define NREG 8

typedef short int Word;             /* word of LC-3 memory */
typedef unsigned short int Address; /* an LC-3 address */

typedef struct {
    Word mem[MEMLEN];    /* memory */
    Word reg[NREG];      /* registers */
    int pc;              /* program counter */
    int running;         /* Is the cpu running? (1 yes, 0 no) */
    int cc;              /* condition code (branching) */
    Word ir;             /* instruction register */
    int opcode;          /* current instruction's opcode */
    unsigned int origin; /* where the program begins in memory */
    char condition;      /* condition code in character format (debug info) */
} CPU;

/* Function Prototypes */

/* Initialization */
FILE *get_datafile(int argc, char *argv[]);  
void initialize_control_unit(CPU *cpu);
void initialize_memory(int argc, char *argv[], CPU *cpu);

/* Dumping info (program + debug) */
void dump_control_unit(CPU *cpu);
void dump_memory(CPU *cpu);
void dump_registers(CPU *cpu);
void help_message(void);

/* Instruction Execution-related */
int execute_command(char *cmd_buffer, char cmd_char, CPU *cpu);
void one_instruction_cycle(CPU *cpu);
void manyInstructionCycles(CPU *cpu, int nbr_cycles);

/* Condition Code */
void generateCondition(CPU *cpu);
void calculateCondition(int result, CPU *cpu);

/* LC-3 instruction operations*/
void add_instr(CPU *cpu);
void and_instr(CPU *cpu);
void not_instr(CPU *cpu);

void load_instr(CPU *cpu);
void ldr_instr(CPU *cpu);
void ldi_instr(CPU *cpu);
void lea_instr(CPU *cpu);

void store_instr(CPU *cpu);
void str_instr(CPU *cpu);
void sti_instr(CPU *cpu);

void branch_instr(CPU *cpu);
void jump_instr(CPU *cpu);
void jump_subr_instr(CPU *cpu);
void trap_instr(CPU *cpu);

/* Manipulate CPU */
int read_execute_command(CPU *cpu);
void jump_command(char *cmd_buffer,CPU *cpu);
void register_command(char *cmd_buffer,CPU *cpu);
void memory_command(char *cmd_buffer, CPU *cpu);
void halt_processor(CPU *cpu);

    
int main(int argc, char *argv[])
{
    printf("LC-3 Simulator\n");

    CPU cpu_value;
    CPU *cpu = &cpu_value;

    /* Initialize everything */
    initialize_control_unit(cpu);
    initialize_memory(argc, argv, cpu);

    /* Dump initial (clean) state */
    dump_control_unit(cpu);
    dump_memory(cpu);

    /* Start accepting input */
    char *prompt = "> ";
    printf("Beginning execution; type h for help\n%s", prompt);
    int done = read_execute_command(cpu);
    while (!done) {
         printf("\n%s", prompt);
         done = read_execute_command(cpu);
    }

    return 0;
}

/* Calculate cc from previous result */
void calculateCondition(int result, CPU *cpu)
{
    if (result > 0)
        cpu->cc = 1;
    else if (result == 0)
        cpu->cc = 2;
    else
        cpu->cc = 4;

}

/* Generate readable representation of cc */
void generateCondition(CPU *cpu)
{
    switch(cpu->cc) {
    case 1: cpu->condition = 'P'; 
            break;
    case 2: cpu->condition = 'Z';
            break;
    case 4: cpu->condition = 'N';
            break;
    }
}

/* init: zero-out PC and all registers, set cc to Z */
void initialize_control_unit(CPU *cpu)
{
    cpu->pc = 0;
    cpu->ir = 0;
    cpu->running = 1;
    cpu->cc = 2;
    
    int i;
    for(i = 0; i < NREG; i++)
        cpu->reg[i] = 0;
}

/* init: populate memory and set PC to the start of the program */
void initialize_memory(int argc, char *argv[], CPU *cpu)
{
    FILE *datafile = get_datafile(argc, argv);

    int value_read, words_read, loc = 0, done = 0;

    char *buffer = NULL;
    size_t buffer_len = 0, bytes_read = 0;


    /* First line of the data file, contains the beginning (origin)
     * of the program. Start populating memory from that location
     * and also update the PC to point there */
    bytes_read = getline(&buffer, &buffer_len, datafile); 
    words_read = sscanf(buffer,"%x",&value_read);
    loc = value_read;
    cpu->pc = value_read;
    cpu->origin = value_read;

    /* Fetch first instruction */
    bytes_read = getline(&buffer, &buffer_len, datafile);
    while (bytes_read != -1 && !done) {

        /* Scan for integer (in hexadecimal format) */
        words_read = sscanf(buffer, "%x", &value_read);

        /* If beginning was not an integer (junk) discard it and go on
         * else populate memory location and advance to the next one */
        if (words_read == 0 || words_read == -1) {
            bytes_read = getline(&buffer, &buffer_len, datafile);
            continue;
        } else {
            cpu->mem[loc++] = value_read;
            bytes_read = getline(&buffer, &buffer_len, datafile);
        }
    }

    /* buffer is not needed any more */
    free(buffer);
    
    /* zero-out the rest of the memory */
    while (loc < MEMLEN) {
        cpu -> mem[loc++] = 0;
    }
}

FILE *get_datafile(int argc, char *argv[])
{
    char *default_datafile_name = "program.hex";
    char *datafile_name;

    /* if a datafile is not provided, use the default. */
    if(argv[1] != NULL) {
        datafile_name = argv[1];
    } else {
        datafile_name = default_datafile_name;
    }   

    printf("Loading %s\n\n", datafile_name);

    FILE *datafile = fopen(datafile_name, "r");
    if (datafile == NULL) {
        printf("error: Could not open file %s\n",
               datafile_name);
        exit(EXIT_FAILURE);
    }

    return datafile;
} 

void dump_control_unit(CPU *cpu)
{
    printf("CONTROL UNIT:\n");
    generateCondition(cpu);
    printf("PC: x%4X\tIR: %#4X\tCC: %c\tRUNNING: %d\n",
           cpu->pc, cpu->ir, cpu->condition, cpu->running);
    dump_registers(cpu); 
}

void dump_memory(CPU *cpu)
{
    printf("MEMORY (addresses x0000 - xFFFF):\n");

    int loc = cpu->origin;

    while(cpu->mem[loc] != 0) {

        if (cpu->mem[loc] < 0) {
            int x = 65535;
            x = (x & cpu->mem[loc]);

            printf("x%4X: x%04X\t%d\n",
                   loc, x, cpu->mem[loc]);
        } else {
            printf("x%4x: x%04X\t%d\n",
                   loc, cpu->mem[loc], cpu->mem[loc]);
        }
        loc++;
    }
    printf("\n");
}

void dump_registers(CPU *cpu)
{
    int i;
    for(i = 0; i < NREG/2; i++)
        printf("R%d: %x \t", i, cpu->reg[i]);
    printf("\n");

    for(i = NREG/2; i < NREG; i++)
        printf("R%d: %x \t", i, cpu->reg[i]); 
    printf("\n\n");
}

int execute_command(char *cmd_buffer, char cmd_char, CPU *cpu)
{
    switch(cmd_char) {
    case '?':
    case 'h':
            help_message();
            return 0;
            break;

    case 'd':
            dump_control_unit(cpu);
            dump_memory(cpu);
            return 0;
            break;

    case 'q':
            printf("quitting...\n");
            return 1;
            break;

    case 'j':
            jump_command(cmd_buffer, cpu);           
            break;

    case 'r':
            register_command(cmd_buffer,cpu);
            break;

    case 'm':
            memory_command(cmd_buffer,cpu);
            break;
    default: 
            printf("Invalid command");
            break;
    }
    
    return 0;
}

void help_message(void)
{
    printf("Choose from the following menu\n");
    printf("d: dump control unit\n");
    printf("q: quit the program \n");
    printf("j xNNNN to jump to a new location\n");
    printf("m XNNNN XMMMM to assign memory location xMMMMM tox NNNN\n");
    printf("a number to run the amount of instruction cycles \n");
    printf("or a return to execute one cycle\n");
}

int read_execute_command(CPU *cpu)
{
    int done = 0, nbr_cycles;
    char *cmd_buffer = NULL;
    size_t cmd_buffer_len = 0, bytes_read = 0;

    char cmd_char;
    size_t words_read;

    /* Get user input */
    bytes_read = getline(&cmd_buffer, &cmd_buffer_len, stdin);

    /* Did we get an end-of-file? (Ctrl + D from user keyboard) */
    if (bytes_read == -1)
        done = 1;

    words_read = sscanf(cmd_buffer, "%d", &nbr_cycles);

    /* If a char was read then try to execute the according
     * command, else if an integer was entered, try to
     * execute this number of cycles */
    if (words_read == 0) {
            sscanf(cmd_buffer,"%c", &cmd_char); 
            done = execute_command(cmd_buffer, cmd_char, cpu);
    } else {

        /* If just a newline or 1, execute one instruction,
         * else check if the number is out of bounds,
         * if not execute multiple instructions as specified */
        if (strcmp(cmd_buffer, "\n") == 0) {
            one_instruction_cycle(cpu);
        } else if (nbr_cycles < 1 || nbr_cycles > MEMLEN
                                  || nbr_cycles >(MEMLEN-(cpu->pc))) {
            printf("%d is an invalid number of cycles!!", nbr_cycles);
        } else if (nbr_cycles == 1) {  
            one_instruction_cycle(cpu);
        } else {
            manyInstructionCycles(cpu, nbr_cycles);
        }
    }

    /* Command buffer not needed anymore */
    free(cmd_buffer);

    return done;
}

void one_instruction_cycle(CPU *cpu)
{
    /* Check if program is running */
    if (cpu->running == 0) {
        printf("halted!\n");
        return;
    }

   /* Check if PC is out of range */ 
   if (cpu->pc >= MEMLEN) {
       printf("Program counter out of range");
       cpu->running = 0;
   }

   /* Fetch instruction from the memory
    * to the instruction register, get the opcode
    * and then try to execute instruction */
    cpu -> ir = cpu->mem[cpu->pc++];
    printf("x%04X: x%04X ", (cpu->pc-1), (cpu->ir & 0xffff));
    cpu->opcode = (cpu->ir & (0xF000)) >> 12;

    switch(cpu->opcode) {
    /* BRANCH */
    case 0x0: branch_instr(cpu); break;
    /* ADD */
    case 0x1: add_instr(cpu); break;
    /* LOAD */
    case 0x2: load_instr(cpu); break;
    /* STORE */
    case 0x3: store_instr(cpu); break;
    /* JSRR */
    case 0x4: jump_subr_instr(cpu); break;
    /* AND */
    case 0x5: and_instr(cpu); break;
    /* LDR */
    case 0x6: ldr_instr(cpu); break;
    /* STR */
    case 0x7: str_instr(cpu); break;
    /* RTI */
    case 0x8: {
        printf("unsupported \"RTI\" halting...");
        halt_processor(cpu);         
    }   break;
    /* NOT */
    case 0x9: not_instr(cpu); break;
    /* LDI */
    case 0xA: ldi_instr(cpu); break;
    /* STI */
    case 0xB: sti_instr(cpu); break;
    /* JMP */
    case 0xC: jump_instr(cpu); break;
    /* ERR */
    case 0xD: {
        printf("unsupported \"err\" halting...");
        halt_processor(cpu);
    }   break;
    /* LEA */
    case 0xE: lea_instr(cpu); break;
    /* TRAP */
    case 0xF: trap_instr(cpu); break;
    /* Unrecognized */
    default:
        printf("Sorry, opcode not recognized");
        break;
    }
}

void manyInstructionCycles(CPU *cpu, int nbr_cycles)
{   
    int i;
    for (i = 0; i < nbr_cycles; i++) {
        one_instruction_cycle(cpu);
        printf("\n");
    }
}

void branch_instr(CPU *cpu)
{
    int pcoffset, argu = ((cpu->ir& 0x0e00) >> 9);

    if(cpu->ir == 0x0000) {
        generateCondition(cpu);
        printf("NOP, no go to CC:%c", cpu->condition);
    } else if((cpu->cc & argu) != 0 ) {
        char *conditioncode;

        switch(argu) {
        case 7:  conditioncode = "NZP"; break;
        case 6:  conditioncode = "NZ";  break;
        case 5:  conditioncode = "NP";  break;
        case 4:  conditioncode = "N";   break;
        case 3:  conditioncode = "ZP";  break;
        case 2:  conditioncode = "Z";   break;
        case 1:  conditioncode = "P";   break;
        default: conditioncode = " ";   break;
        }

        pcoffset = (cpu->ir & 0x01FF);
        int neg = pcoffset >> 8;

        if (neg == 1)
            pcoffset =  pcoffset-512;

        cpu->pc = cpu->pc + pcoffset;

        generateCondition(cpu);

        printf("BR%s %d, cc = %c  goto  to location x%X ",
               conditioncode, pcoffset, cpu->condition, cpu->pc);
    }
}

void add_instr(CPU *cpu)
{
    int mask = (1 << 5);

    /* Check if ADD contains 2 registers or is IMMED */
    int ident = (mask & cpu->ir) >> 5;

    unsigned int dst, src1;
    dst = ((7 << 9) & cpu->ir) >> 9;
    src1 = ((7 << 6) & cpu->ir) >> 6;

    switch(ident) {
    case 0:{
        unsigned int src2;
        src2 = (7 & cpu->ir); 

        printf("ADD R%d, R%d, R%d;", dst, src1, src2);
        printf(" R%d <- x%X + x%X ",
               dst, cpu->reg[src1], cpu->reg[src2]);

        cpu->reg[dst] = (cpu->reg[src1] + cpu->reg[src2]);

        printf("= x%X", cpu->reg[dst]);

        calculateCondition(cpu->reg[dst], cpu);
        generateCondition(cpu);

        printf(" CC: %c", cpu->condition);
    }   break;
    case 1:{
        int imm = (31 & cpu->ir);
        int sign = imm >> 4;

        /* Convert to a negative integer if needed */
        if (sign == 1)
            imm -= 32;

        cpu->reg[dst] = (cpu->reg[src1] + imm);

        printf("ADD R%d, R%d, %d;", dst, src1, imm);
        printf(" R%d <- x%X+%d ", dst, cpu->reg[src1], imm);

        cpu->reg[dst] = (cpu->reg[src1]+imm);

        printf("= x%X", cpu->reg[dst]);

        calculateCondition(cpu->reg[dst], cpu);
        generateCondition(cpu);

        printf(" CC: %c", cpu->condition);
    }   break;
    default:
            printf("instruction not recognized\n");
            break;
    }
}

void load_instr(CPU *cpu)
{
    int pcoffset = (511 & cpu->ir);
    unsigned int dst = ((7 << 9) & cpu->ir) >> 9;
    int flag = pcoffset >> 8;

    /* Negative integer calculation? */
    if (flag == 1)
        pcoffset -= 512;

    printf("LD R%d, %d; ", dst, pcoffset);

    int sum = (cpu->pc + pcoffset);

    printf(" R%d <- M[PC+%d] = M[x%X]", dst, pcoffset, sum);

    cpu->reg[dst] = cpu->mem[(cpu->pc) + pcoffset];

    calculateCondition(cpu->reg[dst], cpu);
    generateCondition(cpu); 

    printf(" = x%04X CC:%c", cpu->reg[dst], cpu->condition);
}

void store_instr(CPU *cpu)
{
    int pcoffset = (511 & cpu->ir);
    int flag = pcoffset >> 8;

    if (flag ==1)
        pcoffset -= 512;

    unsigned int dst = ((7 << 9) &cpu->ir) >> 9;

    printf("ST R%d, %x; ",dst, pcoffset);

    cpu->mem[(cpu->pc) + pcoffset] = cpu->reg[dst];

    int add = (cpu->pc) + pcoffset;   

    calculateCondition(cpu->reg[dst], cpu);
    generateCondition(cpu);

    printf("M[PC+%d] = M[x%04x] <- x%04x CC:%c",
           pcoffset,add,cpu->mem[add], cpu->condition);
}

void jump_subr_instr(CPU *cpu)
{
    int switchJump = ((cpu->ir & (1 << 11)) >> 11);

    switch(switchJump) {
    case 1: {
        cpu->reg[7] = cpu->pc;

        int jumpoffset = (cpu->ir & 0x7FF);
        int flag = jumpoffset >> 10;

        if (flag == 1)
            jumpoffset -= 2048;

        printf("JSR to x%X+%x", cpu->pc, jumpoffset);

        cpu->pc = cpu->pc + jumpoffset;

        printf(" = x%X (R7 = x%X)", cpu->pc,cpu->reg[7]);
    }   break;
    case 0: {
        unsigned int base, pcHolder;
        base = cpu->ir & 0x1c0; 

        if (base == 7) {
            pcHolder = cpu->pc;

            printf("JSRR R%d = x%X(R7 = x%X)",
                   base, cpu->reg[base], cpu->reg[7]);

            cpu->pc = cpu->reg[base];   
            cpu->reg[base] = pcHolder;
        } else {
            cpu->reg[7] = cpu->pc;

            printf("JSRR R%d = x%X(R7 = x%X)",
                   base, cpu->reg[base], cpu->reg[7]);

            cpu->pc = cpu->reg[base];   
        }
    } break;
    }
}

void and_instr(CPU *cpu)
{
    int mask = (1 << 5);
    int ident = (mask & cpu->ir) >> 5;

    unsigned int dst, src1;
    dst = ((7 << 9) & cpu->ir) >> 9;
    src1 = ((7 << 6) & cpu->ir) >> 6;

    switch(ident) {
    case 0: {
         unsigned int src2;
         src2 = (7 & cpu->ir);
         cpu->reg[dst] = cpu->reg[src2] & cpu->reg[src1];

         printf("AND R%d, R%d, R%d;", dst, src1, src2);
         printf(" R%d <- x%X & x%X", dst, cpu->reg[src1], cpu->reg[src2]);

         cpu->reg[dst] = (cpu->reg[src1] & cpu->reg[src2]);

         calculateCondition(cpu->reg[dst], cpu);
         generateCondition(cpu);

         printf(" = x%X; CC = %c", cpu->reg[dst], cpu->condition); 
    }    break;
    case 1: {
         int imm = (31 & cpu->ir);
         int flag = imm >> 4;

         if (flag ==1)
             imm -= 32;

         cpu->reg[dst] = (cpu->reg[src1] & imm);

         printf("AND R%d, R%d, %d;", dst,src1,imm);
         printf(" R%d <- x%X & %d = ", dst, src1, imm); 

         cpu->reg[dst] = (cpu->reg[src1] & imm);

         calculateCondition(cpu->reg[dst], cpu);
         generateCondition(cpu);

         printf("x%X; CC = %c", cpu->reg[dst], cpu->condition);
    }    break;
    default:
         printf("instruction not recognized\n"); //error code 
         break;
    }
}

void ldr_instr(CPU *cpu)
{
    unsigned int dst, base;
    dst = ((7 << 9) & cpu->ir) >> 9;
    base = ((7 << 6) & cpu->ir) >> 6;

    int offset;
    offset = (63 & cpu->ir);

    int flag = offset >> 5;
    if (flag == 1)
        offset -= 64;

    printf("LDR R%d R%d %d; R%d <- mem[x%X + %X] = ",
           dst, base, offset, dst, cpu->reg[base], offset);

    cpu->reg[dst] = cpu->mem[(cpu->reg[base] + offset)];

    calculateCondition(cpu->reg[dst],cpu);
    generateCondition(cpu);

    printf("x%x; CC = %c", cpu->reg[dst],cpu->condition);
}

void str_instr(CPU *cpu)
{
    unsigned int src, base;
    src = ((7 << 9) & cpu->ir) >> 9;
    base = ((7 << 6) & cpu->ir) >> 6;

    int offset = (63&cpu->ir);

    int flag = offset >> 5;
    if(flag == 1)
        offset -= 64;

    printf("STR R%d R%d %d; M[x%X + %d] = ",
           src, base, offset, base, offset);

    cpu->mem[(cpu->reg[base] + offset)] = cpu->reg[src];

    calculateCondition(cpu->mem[(cpu->reg[base] + offset)], cpu);
    generateCondition(cpu);

    printf("x%X; CC = %c",
           cpu->mem[(cpu->reg[base] + offset)], cpu->condition);
}

void not_instr(CPU *cpu)
{
    unsigned int dst, src;
    dst = ((7 << 9) & cpu->ir) >> 9;
    src = ((7 << 6) & cpu->ir) >> 6;

    printf("NOT R%d, R%d; R%d <- Not x%X = ",
           dst, src, dst, cpu->reg[src]);

    cpu->reg[dst] = ~cpu->reg[src];

    calculateCondition(cpu->reg[dst], cpu);
    generateCondition(cpu);

    printf("x%X; CC = %c", cpu->reg[dst], cpu->condition);

}

void ldi_instr(CPU *cpu)
{
    unsigned int dst = ((cpu->ir & 0x0e00) >> 9);

    int pcoffset1, pcoffset;
    pcoffset = (cpu->ir & 0x01FF);

    int flag = pcoffset >> 8;
    if (flag == 1)
        pcoffset -= 512;

    pcoffset1= cpu->pc + pcoffset;

    printf("LDI R%d, x%X; R%d <-M[M[PC+%X]] = M[M[x%X]] = M[x%x] = ",
           dst, cpu->mem[cpu->pc], dst, pcoffset,
           pcoffset1, cpu->mem[cpu->pc]);

    cpu->reg[dst] = cpu->mem[cpu->mem[pcoffset]];

    calculateCondition(cpu->reg[dst], cpu);
    generateCondition(cpu);

    printf("x%X; CC = %c", cpu->reg[dst], cpu->condition);
}

void sti_instr(CPU *cpu)
{
    unsigned int src = ((cpu->ir & 0x0e00) >> 9);

    int pcoffset, pcoffset1;
    pcoffset = (cpu->ir & 0x01FF);

    int flag = pcoffset >> 8;
    if (flag == 1)
        pcoffset -= 512;

    pcoffset1 = cpu->pc + pcoffset;

    printf("STI R%d, %d; M[M[PC+%d]] = M[M[x%X]] = M[x%X] = x%X; ",
           src, pcoffset, pcoffset, pcoffset1,
           cpu->mem[cpu->pc], cpu->mem[cpu->mem[cpu->pc]]);

    cpu->mem[cpu->mem[cpu->pc]] = cpu->reg[src];

    calculateCondition(cpu->mem[cpu->mem[cpu->pc]], cpu);
    generateCondition(cpu);

    printf("CC = %c", cpu->condition);

}

void jump_instr(CPU *cpu)
{
    unsigned int base = (cpu->ir & 0xe0) >> 6;
    cpu->reg[base] = cpu->pc;

    printf("JMP R%d, goto ", base);

    cpu->pc = cpu->reg[base];

    printf("x%X", cpu->pc);
}

void lea_instr(CPU *cpu)
{
    unsigned int dst = ((cpu->ir & 0x0e00) >> 9);

    int pcoffset = (cpu->ir & 0x01FF);

    int flag = pcoffset >> 8;
    if (flag == 1)
        pcoffset -= 512;

    int k = cpu->pc + pcoffset;

    cpu->reg[dst] = cpu->pc + pcoffset;

    printf("LEA R%d, %d; R%d <- PC+%d = ",
           dst,pcoffset, dst, k);

    calculateCondition(cpu->reg[dst], cpu);
    generateCondition(cpu);

    printf("x%X; CC = %c",
           cpu->reg[dst], cpu->condition);
}

void trap_instr(CPU *cpu)
{
    cpu->reg[7] = cpu->pc;
    int trapCode = cpu->ir & (0xFF);

    switch(trapCode){
    /* GETCHAR */
    case 0x20: {
        printf("Trap x20(GETC): ");

        char input;
        cpu->reg[0] = cpu->reg[0] & 0;
        scanf("%c", &input);
        cpu->reg[0] = input;

        printf("Read:%c = %d",cpu->reg[0],cpu->reg[0]);
    }   break;
    /* OUT */
    case 0x21: {
        printf("TRAP x21(OUT): %d = %c; CC = %c",
               cpu->reg[0],cpu->reg[0], cpu->condition);
    }   break;
    /* PUTS */
    case 0x22: {
        int location = cpu->reg[0];

        printf("TRAP x22 (PUTS): ");

        while(cpu->mem[location] != 0) {
            printf("%c",cpu->mem[location++]);
        }

        printf("\n\nCC = %c", cpu->condition);
    }   break;
    /* IN */
    case 0x23: {
        printf("TRAP x23(IN) Input a character: ");

        char input;
        cpu->reg[0] = cpu->reg[0] & 0;
        scanf("%c", &input);
        cpu->reg[0] = input;

        printf("Read:%c = %d",cpu->reg[0],cpu->reg[0]);
    }   break;
    /* BAD VECTOR TRAP */
    case 0x24:{
        printf("TRAP x24, bad trap vector; halting");   
        halt_processor(cpu);
    }   break;
    /* HALT */
    case 0x25:{
        printf("halted");
        halt_processor(cpu);
    }   break;
    /* BAD TRAP */
    default: {
        printf("Bad Trap code");
    }   break;
    }

    /* Return back to original PC */
    cpu->pc = cpu->reg[7];
}

void halt_processor(CPU *cpu)
{
    cpu->running = 0;
}

void jump_command(char *cmd_buffer,CPU *cpu)
{
    int inputNum,
        x = sscanf(cmd_buffer,"j x%x",&inputNum);

    if(x != 1) {
        printf("Jump command should be j address\n");
    } else {
        printf("jumping to  x%x\n", inputNum); 
        cpu->pc = inputNum;
        cpu->running = 1;
    }
}

void register_command(char *cmd_buffer,CPU *cpu) {
    int inputNum, registerInput,
        x = sscanf(cmd_buffer, "r r%d x%x\n", &registerInput, &inputNum);

    if (x != 2) {
        printf("Register command should be r rN value (xNNNN format)\n");
    } else {
        printf("Setting R%d to x%X\n", registerInput, inputNum);
        cpu->reg[registerInput] = inputNum;
    }
}

void memory_command(char *cmd_buffer, CPU *cpu) {
    int inputNum, memAdd,
        x = sscanf(cmd_buffer, "m x%x x%x", &memAdd, &inputNum);

    if (x != 2) {
        printf("Memory command should be in m addr value (xNNNN format)\n");           
    } else {
        printf("Setting m[x%04X] to x%X\n", memAdd, inputNum);
        cpu->mem[memAdd] = inputNum;
    }
}

