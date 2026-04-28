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
* Makes the miter on the target wire for the exdc
*/
Aig_Man_t* MakeOCDMiter(Aig_Man_t* pAig, Aig_Obj_t* pTargetWire);
