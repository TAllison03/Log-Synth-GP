#include "fileio.h"
#include <iostream>
#include <fstream>

void WriteExdcBlif(const std::string& origPath,
                   const std::string& rawPath,
                   const std::string& outputPath)
{
    std::ifstream orig(origPath);
    std::ifstream odc(rawPath);
    std::ofstream out(outputPath);

    std::string line;

    // Copy original blif file up to .end or .exdc
    while (std::getline(orig, line))
    {
        if (line == ".end" || line == ".exdc")
            break;
        out << line << "\n";
    }

    // Start the exdc block
    out << "\n.exdc\n";

    // Copy ODC logic from raw miter blif, skipping model/comment lines
    while (std::getline(odc, line))
    {
        if (line.rfind(".model", 0) == 0) continue;
        if (line.rfind("#", 0) == 0) continue;
        out << line << "\n";
    }

    out.close();
    odc.close();
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
