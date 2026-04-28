#include "miter.h"

Aig_Man_t* MakeOCDMiter(Aig_Man_t* pAig, Aig_Obj_t* pTargetWire) {
    
    // Make a new AIG to hold the Miter with plenty of space for universe 0 and 1
    Aig_Man_t* pMiterAig = Aig_ManStart(Aig_ManNodeNum(pAig) * 2);

    // Maps of when circuit target = 0 and when target = 1
    int arr0Size = Aig_ManObjNumMax(pAig);
    int arr1Size = Aig_ManObjNumMax(pAig);
    std::vector<Aig_Obj_t*> map0(arr0Size, nullptr); // target = 0
    std::vector<Aig_Obj_t*> map1(arr1Size, nullptr); // target = 1

    // Map constants between old and new nodes
    map0[Aig_ManConst1(pAig)->Id] = Aig_ManConst1(pMiterAig);
    map1[Aig_ManConst1(pAig)->Id] = Aig_ManConst1(pMiterAig);

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
    Aig_ManForEachNode(pAig, pObj, i)
    {
        // Skip target wire
        if (pObj == pTargetWire)
            continue;

        // Find pointers for 0 and 1 children for universe 0
        Aig_Obj_t* pChild0_0 = Aig_NotCond(map0[Aig_ObjFanin0(pObj)->Id], Aig_ObjFaninC0(pObj));
        Aig_Obj_t* pChild1_0 = Aig_NotCond(map0[Aig_ObjFanin1(pObj)->Id], Aig_ObjFaninC1(pObj));
        map0[pObj->Id] = Aig_And(pMiterAig, pChild0_0, pChild1_0);

        // Find pointers for 0 and 1 children for universe 1
        Aig_Obj_t* pChild0_1 = Aig_NotCond(map1[Aig_ObjFanin0(pObj)->Id], Aig_ObjFaninC0(pObj));
        Aig_Obj_t* pChild1_1 = Aig_NotCond(map1[Aig_ObjFanin1(pObj)->Id], Aig_ObjFaninC1(pObj));
        map1[pObj->Id] = Aig_And(pMiterAig, pChild0_1, pChild1_1);
    }

    Aig_Obj_t* pCareSet = Aig_Not(Aig_ManConst1(pMiterAig)); // Set care set to 0
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

    // ODC is inverse of care set
    Aig_Obj_t* pODC = Aig_Not(pCareSet);
    Aig_ObjCreateCo(pMiterAig, pODC);

    // Cleanup
    map0.clear();
    map1.clear();

    return pMiterAig;
}
