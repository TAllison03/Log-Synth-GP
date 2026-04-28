#include <iostream>
#include <string>
#include "miter.h"
#include "fileio.h"

int main(int argc, char *argv[])
{
    // Check that blif was actually passed in and build command
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.blif>" << std::endl;
        return 1;
    }
    std::string readCmd = std::string("read_blif ") + argv[1];

    // Build output paths
    std::string inputPath = argv[1];
    size_t lastSlash = inputPath.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? inputPath : inputPath.substr(lastSlash + 1);

    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos)
        filename = filename.substr(0, dotPos);

    std::string outputPath = "temp/" + filename + "_exdc.blif";
    std::string rawPath    = "temp/" + filename + "_raw.blif";

    Abc_Start();
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    // Reads the selected .blif file
    if (Cmd_CommandExecute(pAbc, readCmd.c_str()))
        return 1;
    //Cmd_CommandExecute(pAbc, "print_stats");

    Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
    if (pNtk == NULL)
        return 1;

    Abc_Ntk_t* pStrash = Abc_NtkStrash(pNtk, 0, 1, 0);
    if (pStrash == NULL)
        return 1;

    Aig_Man_t* pAig = Abc_NtkToDar(pStrash, 0, 0);
    if (pAig == NULL)
        return 1;

    int numInputs   = Aig_ManCiNum(pAig);
    int numOutputs  = Aig_ManCoNum(pAig);
    int numAndGates = Aig_ManAndNum(pAig);

    std::cout << "Primary Inputs:  " << numInputs  << std::endl;
    std::cout << "Primary Outputs: " << numOutputs << std::endl;
    std::cout << "Total AND Gates: " << numAndGates << std::endl;

    // ===== Run MakeOCDMiter =====
    if (numAndGates == 0)
        return 1;

    // Hardcoded to first gate — to be replaced with loop
    Aig_Obj_t* pTargetWire = Aig_ManObj(pAig, numInputs + 1);

    Aig_Man_t* pMiterAig = MakeOCDMiter(pAig, pTargetWire);
    if (pMiterAig == NULL)
        return 1;

    Aig_ManCleanup(pMiterAig);

    std::cout << "Miter Primary Inputs:  " << Aig_ManCiNum(pMiterAig)  << std::endl;
    std::cout << "Miter Primary Outputs: " << Aig_ManCoNum(pMiterAig)  << std::endl;
    std::cout << "Miter Total AND Gates: " << Aig_ManAndNum(pMiterAig) << std::endl;

    // Build name vectors for BLIF dump
    Abc_Obj_t* pPi;
    int i;

    // Loops over and gets all used variable names
    Vec_Ptr_t* vPiNames = Vec_PtrAlloc(Aig_ManCiNum(pAig));
    Abc_NtkForEachCi(pNtk, pPi, i)
        Vec_PtrPush(vPiNames, Abc_ObjName(pPi));

    // Gets the output names for the file
    Abc_Obj_t* pPo;
    int j;
    Vec_Ptr_t* vPoNames = Vec_PtrAlloc(Abc_NtkCoNum(pNtk));
    Abc_NtkForEachCo(pNtk, pPo, j)
        Vec_PtrPush(vPoNames, Abc_ObjName(pPo));

    // Dump miter to a blif
    Aig_ManDumpBlif(pMiterAig, const_cast<char*>(rawPath.c_str()), vPiNames, vPoNames);

    // Create exdc blif
    WriteExdcBlif(argv[1], rawPath, outputPath);

    // Cleanup
    Aig_ManCleanup(pMiterAig);
    std::string raw = "temp/" + filename + "_raw.blif";
    Aig_ManDumpBlif(pMiterAig, const_cast<char*>(raw.c_str()), vPiNames, vPoNames);
    Aig_ManStop(pAig);
    Abc_NtkDelete(pStrash);
    Abc_Stop();

    // Show commands and run SIS
    RunSIS(outputPath, filename);

    return 0;
}
