#include "miter.h"

Aig_Man_t* MakeOCDMiterWithInputs(Aig_Man_t* pAig, Aig_Obj_t* pTargetWire, const std::vector<Aig_Obj_t*>& vMiterInputs) {
    
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

    // Create miter inputs only for the selected fanins of the target wire
    for (Aig_Obj_t* pInput : vMiterInputs)
    {
        pInput = Aig_Regular(pInput);

        if (map0[pInput->Id] != nullptr)
            continue;

        Aig_Obj_t* pNewCi = Aig_ObjCreateCi(pMiterAig);

        map0[pInput->Id] = pNewCi;
        map1[pInput->Id] = pNewCi;
    }

    // Force target wire to 0 and 1 in each map
    map0[pTargetWire->Id] = Aig_Not(Aig_ManConst1(pMiterAig));
    map1[pTargetWire->Id] = Aig_ManConst1(pMiterAig);

    // Loop through all AND gates
    Aig_ManForEachNode(pAig, pObj, i)
    {
        // Skip target wire and any nodes already mapped as miter inputs
        if (pObj == pTargetWire || map0[pObj->Id] != nullptr)
            continue;

        Aig_Obj_t* pFanin0 = Aig_ObjFanin0(pObj);
        Aig_Obj_t* pFanin1 = Aig_ObjFanin1(pObj);

        // If this node depends on something outside the target fanins,
        // we cannot express it in this restricted miter.
        if (map0[pFanin0->Id] == nullptr || map0[pFanin1->Id] == nullptr ||
            map1[pFanin0->Id] == nullptr || map1[pFanin1->Id] == nullptr)
            continue;

        Aig_Obj_t* pChild0_0 = Aig_NotCond(map0[pFanin0->Id], Aig_ObjFaninC0(pObj));
        Aig_Obj_t* pChild1_0 = Aig_NotCond(map0[pFanin1->Id], Aig_ObjFaninC1(pObj));
        map0[pObj->Id] = Aig_And(pMiterAig, pChild0_0, pChild1_0);

        Aig_Obj_t* pChild0_1 = Aig_NotCond(map1[pFanin0->Id], Aig_ObjFaninC0(pObj));
        Aig_Obj_t* pChild1_1 = Aig_NotCond(map1[pFanin1->Id], Aig_ObjFaninC1(pObj));
        map1[pObj->Id] = Aig_And(pMiterAig, pChild0_1, pChild1_1);
    }

    Aig_Obj_t* pCareSet = Aig_Not(Aig_ManConst1(pMiterAig)); // constant 0
    bool sawRepresentableOutput = false;

    Aig_ManForEachCo(pAig, pObj, i)
    {
        Aig_Obj_t* pFanin = Aig_ObjFanin0(pObj);

        if (map0[pFanin->Id] == nullptr || map1[pFanin->Id] == nullptr)
        continue;

        sawRepresentableOutput = true;

        // Find memory addresses for final gates of universe 0 and 1
        Aig_Obj_t* pOut0 = Aig_NotCond(map0[Aig_ObjFanin0(pObj)->Id], Aig_ObjFaninC0(pObj));
        Aig_Obj_t* pOut1 = Aig_NotCond(map1[Aig_ObjFanin0(pObj)->Id], Aig_ObjFaninC0(pObj));

        // Create Miter
        Aig_Obj_t* pMiterXor = Aig_Exor(pMiterAig, pOut0, pOut1);

        // Accumulate the care sets
        pCareSet = Aig_Or(pMiterAig, pCareSet, pMiterXor);
    }

    // If nothing could be represented using only the target fanins,
    // return constant 0 ODC, meaning "no don't-cares."
    Aig_Obj_t* pODC = sawRepresentableOutput
        ? Aig_Not(pCareSet)
        : Aig_Not(Aig_ManConst1(pMiterAig));

    Aig_ObjCreateCo(pMiterAig, pODC);

    // Cleanup
    map0.clear();
    map1.clear();

    return pMiterAig;
}
