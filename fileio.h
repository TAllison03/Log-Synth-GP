#pragma once

#include <string>
#include "miter.h"

/*
 * Writes the blif to our temp file folder
 */
void WriteExdcBlif(const std::string& origPath,
                   const std::string& rawPath,
                   const std::string& outputPath);

/*
 * Prints command to user and runs SIS
 */
void RunSIS(const std::string& outputPath,
            const std::string& filename);
