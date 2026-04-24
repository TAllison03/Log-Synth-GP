#include <iostream>
#include <vector>
#include <string>
#include <fstream>

extern "C"
{
    #include "base/main/main.h"
    #include "base/abc/abc.h"   // For Abc_Ntk_t
    #include "aig/aig/aig.h"    // For Aig_Man_t
    extern Aig_Man_t* Abc_NtkToDar( Abc_Ntk_t* pNtk, int fExors, int fRegisters );
}

Aig_Man_t* MakeOCDMiter(Aig_Man_t* pAig, Aig_Obj_t* pTargetWire) {
    
    // Make a new AIG to hold the Miter with plenty of space for universe 0 and 1
    Aig_Man_t* pMiterAig = Aig_ManStart(Aig_ManNodeNum(pAig) * 2);

    // Maps of when circuit target = 0 and when target = 1
    int arr0Size = Aig_ManObjNumMax(pAig);
    int arr1Size = Aig_ManObjNumMax(pAig);
    std::vector<Aig_Obj_t*> map0(arr0Size, nullptr); // target = 0
    std::vector<Aig_Obj_t*> map1(arr1Size, nullptr); // target = 1

    // Map constants between old and new nodes
    map0[Aig_ManConst1(pAig)->Id] =  Aig_ManConst1(pMiterAig);
    map1[Aig_ManConst1(pAig)->Id] =  Aig_ManConst1(pMiterAig);

    Aig_Obj_t* pObj;
    int i;

    // Loop through all combinational inputs
    Aig_ManForEachCi(pAig, pObj, i)
    {
        Aig_Obj_t* pNewCi = Aig_ObjCreateCi(pMiterAig);

        // Update maps and set new input pins
        map0[pObj->Id] = pNewCi;
        map1[pObj->Id] = pNewCi;
    }

    // Force target wire to 0 and 1 in each map
    map0[pTargetWire->Id] = Aig_Not(Aig_ManConst1(pMiterAig));
    map1[pTargetWire->Id] = Aig_ManConst1(pMiterAig);

    // Loop through all AND gates
    Aig_ManForEachNode( pAig, pObj, i ) 
    {
        // Skip target wire
        if (pObj == pTargetWire) 
        {
            continue;
        }

        // Find pointers for 0 and 1 children for universe 0
        Aig_Obj_t* pChild0_0 = Aig_NotCond(map0[Aig_ObjFanin0(pObj)->Id], Aig_ObjFaninC0(pObj));
        Aig_Obj_t* pChild1_0 = Aig_NotCond(map0[Aig_ObjFanin0(pObj)->Id], Aig_ObjFaninC0(pObj));
        map0[pObj->Id] = Aig_And(pMiterAig, pChild0_0, pChild1_0); // New AND gate

        // Find pointers for 0 and 1 children for universe 1
        Aig_Obj_t* pChild0_1 = Aig_NotCond(map1[Aig_ObjFanin0(pObj)->Id], Aig_ObjFaninC0(pObj));
        Aig_Obj_t* pChild1_1 = Aig_NotCond(map1[Aig_ObjFanin0(pObj)->Id], Aig_ObjFaninC0(pObj));
        map1[pObj->Id] = Aig_And(pMiterAig, pChild0_1, pChild1_1); // New AND gate
    }

    Aig_Obj_t* pCareSet = Aig_Not(Aig_ManConst1(pMiterAig)); //Set care set to 0
    Aig_ManForEachCo(pAig, pObj, i)
    {
        // Find memory addresses for final gates of universe 0 and 1
        Aig_Obj_t* pOut0 = Aig_NotCond(map0[Aig_ObjFanin0(pObj)->Id], Aig_ObjFaninC0(pObj));
        Aig_Obj_t* pOut1 = Aig_NotCond(map1[Aig_ObjFanin0(pObj)->Id], Aig_ObjFaninC0(pObj));

        // Create Miter
        Aig_Obj_t* pMiterXor = Aig_Exor(pMiterAig, pOut0, pOut1);

        // Accumulate the care sets 
        pCareSet = Aig_Or(pMiterAig, pCareSet, pMiterXor);
    }

    // OCD is inverse of care set
    Aig_Obj_t* pODC = Aig_Not(pCareSet);
    Aig_ObjCreateCo(pMiterAig, pODC);  // ODC is output pin of new Miter

    // Cleanup
    map0.clear();
    map1.clear();

    return pMiterAig;
}

int main(int argc, char *argv[])
{

    // ===== Commands to Take in blif file =====

    // Check that blif was actually passed in and build command
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.blif>" << std::endl;
        return 1;
    }
    std::string readCmd = std::string("read_blif ") + argv[1];

    // Build output path
    std::string inputPath = argv[1];
    size_t lastSlash = inputPath.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? inputPath : inputPath.substr(lastSlash + 1);

    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos)
        filename = filename.substr(0, dotPos);

    std::string outputPath = "outputs/" + filename + "_exdc.blif";

    Abc_Start();
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    // Reads the selected .blif file
    if (Cmd_CommandExecute(pAbc, readCmd.c_str())) {
        return 1;
    }
    //Cmd_CommandExecute(pAbc, "print_stats");

    Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
    if (pNtk == NULL) {
        return 1;
    }

     Abc_Ntk_t* pStrash = Abc_NtkStrash(pNtk, 0, 1, 0);
    if ( pStrash == NULL ) {
        return 1;
    }

    Aig_Man_t * pAig = Abc_NtkToDar(pStrash, 0, 0);
    if (pAig == NULL){
        return 1;
    }

    int numInputs = Aig_ManCiNum(pAig); // Combination Inputs
    int numOutputs = Aig_ManCoNum(pAig); // Combinational Outputs
    int numAndGates = Aig_ManAndNum(pAig); // AND Gates

    std::cout << "Primary Inputs:  " << numInputs << std::endl;
    std::cout << "Primary Outputs: " << numOutputs << std::endl;
    std::cout << "Total AND Gates: " << numAndGates << std::endl;

    // ===== Setup and Run MakeOCDMiter =====

    Aig_Obj_t* pTargetWire = NULL;
    if (numAndGates > 0)
    {
        // Hardcoded to test the first internal logic gate, needs to be 
        // updated to iterate through every gate.
        pTargetWire = Aig_ManObj(pAig, numInputs+1);
    } else
    {
        return 1;
    }

    Aig_Man_t* pMiterAig = MakeOCDMiter(pAig, pTargetWire);

    if (pMiterAig == NULL)
    {
        return 1;
    }

    std::cout << "Miter Primary Inputs:  " << Aig_ManCiNum(pMiterAig) << std::endl;
    std::cout << "Miter Primary Outputs: " << Aig_ManCoNum(pMiterAig) << std::endl;
    std::cout << "Miter Total AND Gates: " << Aig_ManAndNum(pMiterAig) << std::endl;

        // Code to take ODC and put it in exdc form
    Abc_Obj_t* pPi;
    int i;

    // Loops over and gets all used variable names
    Vec_Ptr_t* vPiNames = Vec_PtrAlloc(Aig_ManCiNum(pAig));

    Abc_NtkForEachCi(pNtk, pPi, i) {
        Vec_PtrPush(vPiNames, Abc_ObjName(pPi));
    }

    // Gets the output names for the file
    Vec_Ptr_t* vPoNames = Vec_PtrAlloc(1);
    Vec_PtrPush(vPoNames, (void*)"odc");

    // Puts these names and the miter into a new blif
    std::string raw = "temp/" + filename + "_raw.blif";
    Aig_ManDumpBlif(pMiterAig, const_cast<char*>(raw.c_str()), vPiNames, vPoNames);

    // Code to add ODC to blif file
    std::ifstream orig(argv[1]);
    std::ifstream odc(raw);
    std::ofstream out(outputPath);

    std::string line;

    // Copy original blif file up to .end
    while (std::getline(orig, line))
    {
        if (line == ".end")
            break;

        out << line << "\n";
    }

    // Start the exdc block
    out << "\n.exdc\n";
    out << ".inputs a b c\n";
    out << ".outputs odc\n";


    // Add the odc logic computed in this file
    bool copy = false;

    while (std::getline(odc, line))
    {
        // start copying after first .names
        if (line.rfind(".names", 0) == 0)
            copy = true;

        // stop at end
        if (line == ".end")
            break;

        // skip headers (.inputs/.outputs)
        if (copy && line[0] != '.')
            out << line << "\n";
    }

    // Close the file
    out << ".end\n";
    
    // Cleanup
    Aig_ManStop(pMiterAig);
    Aig_ManStop(pAig);
    Abc_NtkDelete(pStrash);
    Abc_Stop();
    return 0;
}
