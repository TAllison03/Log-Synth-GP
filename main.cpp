#include <iostream>
#include <string>
#include <vector>

#include "miter.h"
#include "fileio.h"
#include "target_info.h"

struct Paths {
    std::string filename;
    std::string rawPath;
    std::string outputPath;
};

static Paths BuildPaths(const std::string& inputPath)
{
    size_t lastSlash = inputPath.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos)
        ? inputPath
        : inputPath.substr(lastSlash + 1);

    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos)
        filename = filename.substr(0, dotPos);

    return {
        filename,
        "temp/" + filename + "_raw.blif",
        "temp/" + filename + "_local_exdc.blif"
    };
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <file.blif> [target_aig_node_id]\n";
        return 1;
    }

    const std::string inputPath = argv[1];
    const Paths paths = BuildPaths(inputPath);

    Abc_Start();

    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t* pNtk = nullptr;
    Abc_Ntk_t* pStrash = nullptr;
    Aig_Man_t* pAig = nullptr;
    Aig_Man_t* pMiterAig = nullptr;

    auto cleanup = [&]() {
        if (pMiterAig != nullptr)
            Aig_ManStop(pMiterAig);

        if (pAig != nullptr)
            Aig_ManStop(pAig);

        if (pStrash != nullptr)
            Abc_NtkDelete(pStrash);

        Abc_Stop();
    };

    std::string readCmd = "read_blif " + inputPath;

    if (Cmd_CommandExecute(pAbc, readCmd.c_str())) {
        cleanup();
        return 1;
    }

    pNtk = Abc_FrameReadNtk(pAbc);

    if (pNtk == nullptr) {
        cleanup();
        return 1;
    }

    pStrash = Abc_NtkStrash(pNtk, 0, 1, 0);

    if (pStrash == nullptr) {
        cleanup();
        return 1;
    }

    pAig = Abc_NtkToDar(pStrash, 0, 0);

    if (pAig == nullptr) {
        cleanup();
        return 1;
    }

    std::cout << "Primary Inputs:  " << Aig_ManCiNum(pAig) << "\n";
    std::cout << "Primary Outputs: " << Aig_ManCoNum(pAig) << "\n";
    std::cout << "Total AND Gates: " << Aig_ManAndNum(pAig) << "\n";

    if (Aig_ManAndNum(pAig) == 0) {
        std::cerr << "Error: no AND nodes found.\n";
        cleanup();
        return 1;
    }

    PrintAndNodes(pAig);

    TargetInfo target;

    if (!BuildTargetInfo(pAig, pNtk, argc, argv, target)) {
        cleanup();
        return 1;
    }

    std::cout << "\nSelected target wire ID: " << target.node->Id << "\n";

    std::cout << "Target fanin IDs used as miter inputs:";
    for (Aig_Obj_t* pInput : target.inputs)
        std::cout << " " << pInput->Id;
    std::cout << "\n";

    std::cout << "Local target output name: " << target.targetName << "\n";

    if (target.hasOneCube)
        std::cout << "Target node on-cube: " << target.onCube << "\n";
    else
        std::cout << "Target node has no on-cube, treating it as constant 0.\n";

    pMiterAig = MakeOCDMiterWithInputs(
        pAig,
        target.node,
        target.inputs
    );

    if (pMiterAig == nullptr) {
        cleanup();
        return 1;
    }

    Aig_ManCleanup(pMiterAig);

    std::cout << "Miter Primary Inputs:  " << Aig_ManCiNum(pMiterAig) << "\n";
    std::cout << "Miter Primary Outputs: " << Aig_ManCoNum(pMiterAig) << "\n";
    std::cout << "Miter Total AND Gates: " << Aig_ManAndNum(pMiterAig) << "\n";

    Vec_Ptr_t* vPiNames = MakeNameVec(target.inputNames);

    std::vector<std::string> outputNames = { target.targetName };
    Vec_Ptr_t* vPoNames = MakeNameVec(outputNames);

    Aig_ManDumpBlif(
        pMiterAig,
        const_cast<char*>(paths.rawPath.c_str()),
        vPiNames,
        vPoNames
    );

    Vec_PtrFree(vPiNames);
    Vec_PtrFree(vPoNames);

    WriteLocalNodeExdcBlif(
        paths.rawPath,
        paths.outputPath,
        target.modelName,
        target.inputNames,
        target.targetName,
        target.onCube,
        target.hasOneCube
    );

    cleanup();

    RunSIS(paths.outputPath, paths.filename);

    return 0;
}