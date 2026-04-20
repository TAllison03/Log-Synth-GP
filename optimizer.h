#ifndef OPTIMIZER_H
#define OPTIMIZER_H

/**
* @brief Creates a Miter of ODCs for a provided AIG
*
* @param pAig: The and-inverter-graph to make the miter out of
* @param pTargetWire: The wire to be optimized
*/
Aig_Man_t* MakeOCDMiter(Aig_Man_t* pAig, Aig_Obj_t* pTargetWire);

#endif