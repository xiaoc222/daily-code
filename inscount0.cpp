/*
 * Copyright 2002-2019 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <map>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "pin.H"
using std::cerr;
using std::ofstream;
using std::ios;
using std::string;
using std::endl;

ofstream OutFile;

// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static UINT64 icount = 0;

std::map<ADDRINT, std::string> addr_dis_map;
std::map<ADDRINT, UINT64> addr_count_map;

int t = 1;

// This function is called before every instruction is executed
VOID docount(ADDRINT insaddr, char* imgname, char* rtnname, ADDRINT imglowaddr) {
        UINT64 inscount = 0;
        if ( addr_count_map.find(insaddr) != addr_count_map.end() ){
            inscount = addr_count_map[insaddr];
            inscount = inscount + 1;
        }
        //addr_count_map.insert(std::make_pair(insaddr, inscount));
        addr_count_map[insaddr] = inscount;

        const char* insdis = addr_dis_map[insaddr].c_str();

        if( (icount % 1000000) == 0){
            printf("%08lX %s %08lX %08lX %s %s pid:%ld tid:%ld  %lu %lu\n",
                insaddr, imgname, (insaddr-imglowaddr), imglowaddr, rtnname, insdis,
                (long)getpid(), (long)syscall(SYS_gettid) , icount, inscount);
        }

        icount++;
}

PIN_LOCK globalLock;

void Read(THREADID tid, ADDRINT addr, ADDRINT inst){

  PIN_GetLock(&globalLock, 1);

  ADDRINT * addr_ptr = (ADDRINT*)addr;
  ADDRINT value;
  PIN_SafeCopy(&value, addr_ptr, sizeof(ADDRINT));

  printf("Read: ADDR, VAL: %lx, %lx\n", addr, value);
  
  PIN_ReleaseLock(&globalLock);
}

void Write(THREADID tid, ADDRINT addr, ADDRINT inst ){    

  PIN_GetLock(&globalLock, 1); 


    printf("Write: ADDR: %lx\n", addr); 

      
  PIN_ReleaseLock(&globalLock);
}    


// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{

 char b[]={0x32,0x00};
    char* rtnname=b;
    char* imgname=b;
    ADDRINT imglowaddr = 0;
    ADDRINT insaddr = INS_Address(ins);
    std::string insdis = INS_Disassemble(ins);
    //printf("2222 %s\n", insdis.c_str());
    addr_dis_map.insert(std::make_pair(insaddr, insdis));
    //addr_count_map.insert(std::make_pair(insaddr, 0));

    RTN rtn = INS_Rtn(ins);
    if(RTN_Valid(rtn)){
        IMG img = SEC_Img(RTN_Sec(rtn));
	if (!IMG_IsMainExecutable(img) ){
	return ;
	}
        imgname = (char*)IMG_Name(img).c_str();
        imglowaddr = IMG_LowAddress(img);
        rtnname = (char*)RTN_Name(rtn).c_str();
    }

    /*
if(INS_IsMemoryRead(ins)) {
      INS_InsertCall(ins, 
             IPOINT_BEFORE, 
             (AFUNPTR)Read, 
             IARG_THREAD_ID,
             IARG_MEMORYREAD_EA,
             IARG_INST_PTR,
             IARG_END);
      }else if(INS_IsMemoryWrite(ins)){
      INS_InsertCall(ins, 
             IPOINT_BEFORE, 
             (AFUNPTR)Write, 
             IARG_THREAD_ID,//thread id
             IARG_MEMORYWRITE_EA,//address being accessed
             IARG_INST_PTR,//instruction address of write
             IARG_END);
      }
      */


 INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount,
                    IARG_ADDRINT, insaddr,
                    IARG_PTR, imgname, IARG_PTR, rtnname,
                    IARG_ADDRINT, imglowaddr,
                    IARG_END);


}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "inscount.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    // Write to a file since cout and cerr maybe closed by the application
    OutFile.setf(ios::showbase);
    OutFile << "Count " << icount << endl;
    OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    OutFile.open(KnobOutputFile.Value().c_str());

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
