/* C standard library */
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* POSIX */
#include <unistd.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>

/* Linux */
#include <syscall.h>
#include <sys/ptrace.h>

#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>

/* Capstone */
#include <capstone/capstone.h>

#define TOOL "mdb"

#define die(...) \
    do { \
        fprintf(stderr, TOOL": " __VA_ARGS__); \
        fputc('\n', stderr); \
        exit(EXIT_FAILURE); \
    } while (0)

//break point struct
typedef struct breakpoint {
    char *address;
    long original_instr;
} bpoint;


static bpoint *bpoint_list;
static int num_of_bpoints;

//capstone handle
static csh handle;

void print_initial_menu(char * binaryName){
    printf("\nMINIMAL DEBUGGER\n====================\n");
    printf("Binary loaded: %s\n", binaryName);
    printf("--------------------\n");
    printf("Supported Commands:\n");
    printf("\t1. Add breakpoint command: b <function symbol>\n");
    printf("\t2. Add breakpoint command: b *<address in hex>\n");
    printf("\t3. List all existing breakpoints command: l\n");
    printf("\t4. Delete a breakpoint command: d <b number>\n");
    printf("\t5. Run the programm command: r\n");
    printf("\t6. Continue the execution command: c\n");
    printf("\t7. Execute just a single instruction command: si\n");
    printf("\t8. Disassemble next 48 bytes command: disas\n");
    printf("\t9. Disassemble next <n> bytes command: disas <n>\n");
    printf("\t10. Exit mdb command: q\n");
}

void list_breakPoints(){
    fprintf(stderr, "Breakpoint List:\n");
    fprintf(stderr,"%-5s%-20s\n", "Num", "Address");
    int i;
    for (i = 0; i < num_of_bpoints; i++)
        fprintf(stderr, "%-5d%-20s\n", i, bpoint_list[i].address);
}

char *get_address_of_symbol(char * filename, char * symbol){
    Elf *elf;
    Elf_Scn *symtab; 

    /* Initilization.  */
    if (elf_version(EV_CURRENT) == EV_NONE) 
        die("(version) %s", elf_errmsg(-1));

    int fd = open(filename, O_RDONLY);

    elf = elf_begin(fd, ELF_C_READ, NULL); 
    if (!elf) 
        die("(begin) %s", elf_errmsg(-1));

    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    size_t shstrndx;
    if (elf_getshdrstrndx(elf, &shstrndx) != 0)  
        die("(getshdrstrndx) %s", elf_errmsg(-1));

    int s_index = 0;
    //iterate all sections until you find .symtab
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) != &shdr)
            die("(getshdr) %s", elf_errmsg(-1));

        /* Locate symbol table.  */
        if (!strcmp(elf_strptr(elf, shstrndx, shdr.sh_name), ".symtab")){ 
            symtab = scn;
            break;
        }
    }

    Elf_Data *data;
    int count = 0;

    /* Get the descriptor.  */
    if (gelf_getshdr(symtab, &shdr) != &shdr)
        die("(getshdr) %s", elf_errmsg(-1));

    data = elf_getdata(symtab, NULL);
    count = shdr.sh_size / shdr.sh_entsize;

    //compare each symbol with the function argument 'symbol' we are looking for
    //if found, return its address in string 
    for (int i = 0; i < count; ++i) {
        GElf_Sym sym;
        gelf_getsym(data, i, &sym);
        if (strcmp(symbol, elf_strptr(elf, shdr.sh_link, sym.st_name)) == 0){
            //str will be 18bytes long: 16characters of address + "0x"
            char *str = malloc(18);
            sprintf(str, "0x%lx", sym.st_value);
            return str;
        }
    }
    fprintf(stderr,"Symbol not found\n");
    return NULL;
}

void continue_execution(pid_t pid){
    int status;
    pid_t return_pid = waitpid(pid, &status, WNOHANG);
    if (return_pid == 0){
        //tracee is running, so ptrace continue and wait for another event
        if (ptrace(PTRACE_CONT, pid, 0, 0) == -1)
            die("(cont) %s", strerror(errno));
        waitpid(pid, 0, 0);
        if (waitpid(pid, &status, WNOHANG) == pid)
            fprintf(stderr, "Process %d exited with status %d", pid, status);
    }
    //tracee not running
    else {
        fprintf(stderr, "The program is not being run.\n");
    }
}

void add_breakpoint_in_address(pid_t pid, char * address){

    //save original instruction in order to replace it later
    long original_instr = 0;
    original_instr = ptrace(PTRACE_PEEKDATA, pid, (void *)strtoul(address,(char**)0,0), 0);
    if (original_instr == -1)
        die("(peekdata) %s", strerror(errno));

    /* Insert the breakpoint. */
    long trap = (original_instr & 0xFFFFFFFFFFFFFF00) | 0xCC;
    if (ptrace(PTRACE_POKEDATA, pid, (void *)strtoul(address,(char**)0,0), (void *)trap) == -1)
        die("(pokedata) %s", strerror(errno)); 

    //Update breakpoint list 
    num_of_bpoints++;
    bpoint *tmp = (bpoint *) realloc(bpoint_list, sizeof(bpoint) * num_of_bpoints);
    bpoint_list = tmp;

    bpoint *new_bpoint = (bpoint *) malloc(sizeof(bpoint));
    new_bpoint->address = (char *) malloc(strlen(address) + 1);
    strcpy(new_bpoint->address, address);
    new_bpoint->original_instr = original_instr;
    bpoint_list[num_of_bpoints-1] = *new_bpoint;

    fprintf(stderr, "Breakpoint %d at %p\n", num_of_bpoints-1, (void *)strtoul(address,(char**)0,0));
    
}


void reset_old_bpoints(pid_t pid){

    int i;
    long original_instr = 0;
    //set a new interrupt for each breakpoint in the list
    //We could also use bpoint[i].original_instr instead of ptrace peeking 
    for (i = 0; i < num_of_bpoints; i++){
        original_instr = ptrace(PTRACE_PEEKDATA, pid, (void *)strtoul(bpoint_list[i].address,(char**)0,0), 0);
        if (original_instr == -1)
            die("(peekdata) %s", strerror(errno));

        /* Insert the breakpoint. */
        long trap = (original_instr & 0xFFFFFFFFFFFFFF00) | 0xCC;
        if (ptrace(PTRACE_POKEDATA, pid, (void *)strtoul(bpoint_list[i].address,(char**)0,0), (void *)trap) == -1)
            die("(pokedata) %s", strerror(errno));
    }
}

void delete_breakpoint(pid_t pid, int bp){
    if (bp >= num_of_bpoints){
        printf("There is no breakpoint with number %d \n", bp);
        return;
    }

    //if the tracee is running replace the interrupt with original instruction
    int status;
    pid_t return_pid = waitpid(pid, &status, WNOHANG);
    if (return_pid == 0){
        if (ptrace(PTRACE_POKEDATA, pid, (void *)strtoul(bpoint_list[bp].address,(char**)0,0), (void *)bpoint_list[bp].original_instr) == -1)
            die("(pokedata) %s", strerror(errno)); 
    }

    fprintf(stderr, "Breakpoint %d at %p deleted.\n", bp, (void *)strtoul(bpoint_list[bp].address,(char**)0,0));

    //remove from breakpoints list
    int i;
    for (i = bp; i < num_of_bpoints; i++){
        bpoint_list[i] = bpoint_list[i + 1];
    }
    num_of_bpoints--;
}

/*
 Returns the bpoint struct in the given address
*/
bpoint get_bpoint_atAddress(long address){
    int i;
    for (i = 0; i < num_of_bpoints; i++)
        if (address == strtoul(bpoint_list[i].address,(char**)0,0))
            return bpoint_list[i];
    
    bpoint notFound;
    notFound.address = NULL;
    notFound.original_instr = -1;
    return notFound;
}


void disassemble(pid_t pid, long start_address, int size){

    unsigned char code[size];

    /* 
      With each ptrace peekdata we get 8 bytes of instructions, so in each iteration we add
      8 at the request address
     */
    int peek_time = 0;
    int i = 0, j;
    while(i < size){
        long instr = ptrace(PTRACE_PEEKDATA, pid, start_address + (peek_time * 8), 0);
        if (instr == -1) 
            die("(peekdata) %s", strerror(errno));
        for (j = 0; j < 8; j++){
            //gets the least significant byte
            unsigned char LSByte = instr % (16*16);
            code[i] = LSByte;
            //shifts 1 byte to the right so the next byte comes in place
            instr = instr >> 8;
            i++;
            if (i >= size)
                break;
        }
        peek_time++;
    }
    
    cs_insn *insn;
    size_t count;

    //Returns number of succesful disassembled instructions
    count = cs_disasm(handle, code, size, 0x0, 0, &insn);

    if (count > 0) {
        size_t k;
        for (k = 0; k < count; k++) {
            if (k == 0)
                fprintf(stderr, "=> 0x%"PRIx64":\t%s\t\t%s\n", insn[k].address + start_address, insn[k].mnemonic,
                    insn[k].op_str);
            else
                fprintf(stderr, "   0x%"PRIx64":\t%s\t\t%s\n", insn[k].address + start_address, insn[k].mnemonic,
                    insn[k].op_str);
        }
        cs_free(insn, count);
    } else
        fprintf(stderr, "ERROR: Failed to disassemble given code!\n");
}

void disas(pid_t pid, int size){
    int status;
    pid_t return_pid = waitpid(pid, &status, WNOHANG);
    if (return_pid != 0){ //tracee not running
        fprintf(stderr, "No running process.\n");
        return;
    }

    struct user_regs_struct regs;

    if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) 
            die("(getregs) %s", strerror(errno));

    //start disassemble at %rip
    disassemble(pid, regs.rip, size);
}

void single_step(int pid) {

    if (ptrace(PTRACE_SINGLESTEP, pid, 0, 0) == -1)
        die("(singlestep) %s", strerror(errno));
 
    waitpid(pid, 0, 0);

    struct user_regs_struct regs;

    if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) 
            die("(getregs) %s", strerror(errno));

    fprintf(stderr, "Single instruction executed.\n");
    fprintf(stderr, "We 've stopped at 0x%lx\n", regs.rip);
}

void serve_breakpoint(pid_t pid){
    int status;
    pid_t return_pid = waitpid(pid, &status, WNOHANG);
    if (return_pid != 0)
        return;

    struct user_regs_struct regs;

    if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) 
            die("(getregs) %s", strerror(errno));

    fprintf(stderr, "We 're on a breakpoint at 0x%lx\n", regs.rip -1);

    long current_ins = ptrace(PTRACE_PEEKDATA, pid, regs.rip, 0);
    if (current_ins == -1) 
        die("(peekdata) %s", strerror(errno));

    bpoint bp = get_bpoint_atAddress(regs.rip - 1);

    //Replaces interrupt with the original instruction
    if (ptrace(PTRACE_POKEDATA, pid, (void *)(regs.rip - 1), (void *)bp.original_instr)  == -1)
        die("(pokedata) %s", strerror(errno));
    
    //Replaces %rip with the previous instruction
    regs.rip--;
    if (ptrace(PTRACE_SETREGS, pid, 0, &regs) == -1)
        die("(setregs) %s", strerror(errno));

    disassemble(pid, regs.rip, 48);
}

pid_t run_tracee_program(char ** argv){
    /* fork() for executing the program that is analyzed.  */
    pid_t pid = fork();
    switch (pid) {
        case -1: /* error */
            die("Failed to fork. Error: %s", strerror(errno));
        case 0:  /* Code that is run by the child. */
            /* Install a parent death signal, so the child dies too*/
            prctl(PR_SET_PDEATHSIG, SIGTERM);
            /* Start tracing.  */
            ptrace(PTRACE_TRACEME, 0, 0, 0);
            /* execvp() is a system call, the child will block and
               the parent must do waitpid().
             */
            execvp(argv[1], argv + 1);
            if (errno == 2){
                char *local_bin = malloc(strlen(argv[1])+3);
                strcat(local_bin, "./");
                strcat(local_bin, argv[1]);
                execvp(local_bin, argv + 1);
            }
            die("Faild to execute the binary. Error: %s", strerror(errno));
    }

    /* Code that is run by the parent.  */

    /*
    Sets the folowing option:
    Send a SIGKILL signal to the tracee if the tracer
    exits.  This option is useful for ptrace jailers
    that want to ensure that tracees can never escape
    the tracer's control.
    */
    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);

    waitpid(pid, 0, 0);

    return pid;
}

void capstone_init(){
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
        die("Capstone iinitialization failed.");

    /* AT&T */
    cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);
}

int main(int argc, char **argv)
{
    if (argc <= 1)
        die("Not enough arguments given: %d", argc);

    bpoint_list = NULL;
    num_of_bpoints = 0;

    capstone_init();

    pid_t pid = run_tracee_program(argv);

    print_initial_menu(argv[1]);

    char command[256];
    char** tokens = malloc(5 * sizeof(char *)); //maximum 5 tokens per command 

    //prompt command line until exit command
    while(1){
        char *original_command = malloc(256);

        printf("(mdb) ");
        //tokenize command given 
        if (fgets(command, sizeof(command), stdin) != NULL){
            strcpy(original_command, command);
            tokens[0] = strtok(command, "\n");
            tokens[0] = strtok(command, " ");
            int i = 0;
            while (tokens[i] != NULL && i < 5){
                i++;
                tokens[i] = strtok(NULL, " ");
            }
        }

        //if tracee is still running, continue, else start a new tracee process
        if (strcmp(tokens[0], "r") == 0 && tokens[1] == NULL){
            int status;
            pid_t return_pid = waitpid(pid, &status, WNOHANG);
            if (return_pid == 0){ //tracce still running
                continue_execution(pid);
            }
            else {
                pid = run_tracee_program(argv);
                if (num_of_bpoints > 0)
                    //we have to reset all breakpoints that had been set in a previous execution
                    reset_old_bpoints(pid);
                continue_execution(pid);
            }
            serve_breakpoint(pid);
        }
        else if (strcmp(tokens[0], "c") == 0 && tokens[1] == NULL){
            continue_execution(pid);
            serve_breakpoint(pid);
        }
        //set breakpoint using hexadecimal address
        else if (strcmp(tokens[0], "b") == 0 && tokens[1] != NULL && tokens[1][0] == '*' && tokens[2] == NULL){
            int status;
            pid_t return_pid = waitpid(pid, &status, WNOHANG);
            if (return_pid != 0) //tracee not running
                pid = run_tracee_program(argv);
            add_breakpoint_in_address(pid, tokens[1] + 1);
        }
        //set breakpoint using a symbol
        else if (strcmp(tokens[0], "b") == 0 && tokens[1] != NULL && tokens[1][0] != '*' && tokens[2] == NULL){
            int status;
            pid_t return_pid = waitpid(pid, &status, WNOHANG);
            if (return_pid != 0) //tracee not running
                pid = run_tracee_program(argv);
            char *symbol_address = get_address_of_symbol(argv[1], tokens[1]);
            if (symbol_address != NULL)
                add_breakpoint_in_address(pid, symbol_address);
        }
        else if (strcmp(tokens[0], "l") == 0 && tokens[1] == NULL)
            list_breakPoints();
        else if (strcmp(tokens[0], "d") == 0 && tokens[1] != NULL && tokens[2] == NULL)
            delete_breakpoint(pid, atoi(tokens[1]));
        else if (strcmp(tokens[0], "si") == 0 && tokens[1] == NULL)
            single_step(pid);
        else if (strcmp(tokens[0], "disas") == 0 && tokens[1] == NULL)
            disas(pid, 48);
        else if (strcmp(tokens[0], "disas") == 0 && tokens[1] != NULL && tokens[2] == NULL)
            disas(pid, atoi(tokens[1]));
        else if (strcmp(tokens[0], "q") == 0 && tokens[1] == NULL)
            die("Exiting mdb...");
        else 
            if (strcmp(original_command, "\n") != 0)
                fprintf(stderr,"Undefined command: %s", original_command);

        free(original_command);
    }

}
