#pragma once

#include <vector>

extern "C"
{
    #include "base/main/main.h"
    #include "base/abc/abc.h"
    #include "aig/aig/aig.h"
    extern Aig_Man_t* Abc_NtkToDar( Abc_Ntk_t* pNtk, int fExors, int fRegisters );
}

/*
 * Builds an ODC miter for a target wire using the provided local input set.
 */
Aig_Man_t* MakeOCDMiterWithInputs(
    Aig_Man_t* pAig,
    Aig_Obj_t* pTargetWire,
    const std::vector<Aig_Obj_t*>& vMiterInputs
);