#pragma once

#include <string>
#include <vector>

#include "miter.h"

struct TargetInfo {
    Aig_Obj_t* node = nullptr;

    std::vector<Aig_Obj_t*> inputs;
    std::vector<std::string> inputNames;

    std::string targetName;
    std::string modelName;

    std::string onCube;
    bool hasOneCube = false;
};

void PrintAndNodes(Aig_Man_t* pAig);

bool BuildTargetInfo(Aig_Man_t* pAig,
                     Abc_Ntk_t* pNtk,
                     int argc,
                     char* argv[],
                     TargetInfo& target);

Vec_Ptr_t* MakeNameVec(const std::vector<std::string>& names);