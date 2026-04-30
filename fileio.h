#pragma once

#include <string>
#include <vector>
#include "miter.h"

/*
 * Writes a local BLIF where the selected target node is the only output.
 * The raw ODC BLIF is inserted as the .exdc for that target node.
 */
void WriteLocalNodeExdcBlif(const std::string& rawPath,
                            const std::string& outputPath,
                            const std::string& modelName,
                            const std::vector<std::string>& inputNames,
                            const std::string& targetName,
                            const std::string& targetOnCube,
                            bool targetHasOneCube);

/*
 * Prints command to user and runs SIS
 */
void RunSIS(const std::string& outputPath,
            const std::string& filename);
