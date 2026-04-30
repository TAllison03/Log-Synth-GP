#include "fileio.h"
#include <iostream>
#include <fstream>

void WriteLocalNodeExdcBlif(const std::string& rawPath,
                            const std::string& outputPath,
                            const std::string& modelName,
                            const std::vector<std::string>& inputNames,
                            const std::string& targetName,
                            const std::string& targetOnCube,
                            bool targetHasOneCube)
{
    std::ifstream odc(rawPath);
    std::ofstream out(outputPath);

    if (!odc.is_open()) {
        std::cerr << "Error: could not open raw ODC BLIF: "
                  << rawPath << std::endl;
        return;
    }

    if (!out.is_open()) {
        std::cerr << "Error: could not create local EXDC BLIF: "
                  << outputPath << std::endl;
        return;
    }

    out << ".model " << modelName << "\n";

    out << ".inputs";
    for (const std::string& name : inputNames)
        out << " " << name;
    out << "\n";

    out << ".outputs " << targetName << "\n\n";

    // Original target-node implementation.
    // For now this is one AIG AND node, so it is represented by one BLIF cube.
    out << ".names";
    for (const std::string& name : inputNames)
        out << " " << name;
    out << " " << targetName << "\n";

    if (targetHasOneCube)
        out << targetOnCube << " 1\n";

    out << "\n.exdc\n";

    std::string line;

    // Copy raw ODC miter logic.
    // It should already have matching .inputs and .outputs.
    while (std::getline(odc, line))
    {
        if (line.rfind(".model", 0) == 0) continue;
        if (line.rfind(".end", 0) == 0) continue;
        if (line.rfind("#", 0) == 0) continue;

        out << line << "\n";
    }

    out << ".end\n";
}

void RunSIS(const std::string& outputPath, const std::string& filename)
{
    std::cout << "\n===== ODC calculation complete =====" << std::endl;
    std::cout << "Run the following in SIS to optimize:" << std::endl;
    std::cout << "  read_blif " << outputPath << std::endl;
    std::cout << "  full_simplify" << std::endl;
    std::cout << "  write_blif outputs/" << filename << "_optimized.blif" << std::endl;
    std::cout << "  quit" << std::endl;
    std::cout << "\nOpening SIS..." << std::endl;
    system("./sis");
}
