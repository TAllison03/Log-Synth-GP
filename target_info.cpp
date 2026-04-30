#include "target_info.h"

#include <cstdlib>
#include <iostream>

static Aig_Obj_t* FindFirstAndNode(Aig_Man_t* pAig)
{
    Aig_Obj_t* pObj;
    int i;

    Aig_ManForEachNode(pAig, pObj, i)
        return pObj;

    return nullptr;
}

static Aig_Obj_t* FindAndNodeById(Aig_Man_t* pAig, int id)
{
    if (id < 0 || id >= Aig_ManObjNumMax(pAig))
        return nullptr;

    Aig_Obj_t* pObj = Aig_ManObj(pAig, id);

    if (pObj == nullptr || !Aig_ObjIsNode(pObj))
        return nullptr;

    return pObj;
}

static Aig_Obj_t* SelectTargetNode(Aig_Man_t* pAig, int argc, char* argv[])
{
    if (argc >= 3) {
        int targetId = std::atoi(argv[2]);
        return FindAndNodeById(pAig, targetId);
    }

    return FindFirstAndNode(pAig);
}

static std::vector<Aig_Obj_t*> CollectTargetFanins(Aig_Obj_t* pTargetWire)
{
    std::vector<Aig_Obj_t*> fanins;

    Aig_Obj_t* pFanin0 = Aig_Regular(Aig_ObjFanin0(pTargetWire));
    Aig_Obj_t* pFanin1 = Aig_Regular(Aig_ObjFanin1(pTargetWire));

    if (!Aig_ObjIsConst1(pFanin0))
        fanins.push_back(pFanin0);

    if (!Aig_ObjIsConst1(pFanin1) && pFanin1 != pFanin0)
        fanins.push_back(pFanin1);

    return fanins;
}

static bool ValidateTargetFaninsArePis(const std::vector<Aig_Obj_t*>& fanins)
{
    for (Aig_Obj_t* pInput : fanins) {
        if (!Aig_ObjIsCi(pInput)) {
            std::cerr << "Error: target fanin " << pInput->Id
                      << " is not a primary input.\n"
                      << "For this simple local-node version, pick a target whose fanins are PIs.\n";
            return false;
        }
    }

    return true;
}

static bool BuildCiNameLookup(Aig_Man_t* pAig,
                              Abc_Ntk_t* pNtk,
                              std::vector<std::string>& ciNameByAigId)
{
    ciNameByAigId.assign(Aig_ManObjNumMax(pAig), "");

    Aig_Obj_t* pCi;
    int ciIndex = 0;

    Aig_ManForEachCi(pAig, pCi, ciIndex)
    {
        Abc_Obj_t* pAbcPi = Abc_NtkCi(pNtk, ciIndex);

        if (pAbcPi == nullptr) {
            std::cerr << "Error: could not map AIG CI index "
                      << ciIndex << " back to ABC PI.\n";
            return false;
        }

        ciNameByAigId[pCi->Id] = Abc_ObjName(pAbcPi);
    }

    return true;
}

static bool FillInputNames(const std::vector<Aig_Obj_t*>& inputs,
                           const std::vector<std::string>& ciNameByAigId,
                           std::vector<std::string>& inputNames)
{
    inputNames.clear();
    inputNames.reserve(inputs.size());

    for (Aig_Obj_t* pInput : inputs) {
        const std::string& name = ciNameByAigId[pInput->Id];

        if (name.empty()) {
            std::cerr << "Error: no BLIF name found for miter input object ID "
                      << pInput->Id << ".\n";
            return false;
        }

        inputNames.push_back(name);
    }

    return true;
}

static bool BuildTargetAndCube(Aig_Obj_t* pTargetWire,
                               const std::vector<Aig_Obj_t*>& inputs,
                               std::string& cube)
{
    cube.assign(inputs.size(), '-');

    auto applyLiteral = [&](Aig_Obj_t* pFanin, bool isComplemented) -> bool
    {
        Aig_Obj_t* pRegular = Aig_Regular(pFanin);

        if (Aig_ObjIsConst1(pRegular))
            return !isComplemented;

        int index = -1;

        for (int i = 0; i < static_cast<int>(inputs.size()); ++i) {
            if (Aig_Regular(inputs[i]) == pRegular) {
                index = i;
                break;
            }
        }

        if (index == -1) {
            std::cerr << "Error: target fanin " << pRegular->Id
                      << " was not found in local input list.\n";
            return false;
        }

        char requiredValue = isComplemented ? '0' : '1';

        if (cube[index] == '-') {
            cube[index] = requiredValue;
            return true;
        }

        return cube[index] == requiredValue;
    };

    bool ok0 = applyLiteral(Aig_ObjFanin0(pTargetWire), Aig_ObjFaninC0(pTargetWire));
    bool ok1 = applyLiteral(Aig_ObjFanin1(pTargetWire), Aig_ObjFaninC1(pTargetWire));

    return ok0 && ok1;
}

void PrintAndNodes(Aig_Man_t* pAig)
{
    Aig_Obj_t* pObj;
    int i;

    std::cout << "\nCandidate AND nodes:\n";

    Aig_ManForEachNode(pAig, pObj, i)
    {
        Aig_Obj_t* pFanin0 = Aig_Regular(Aig_ObjFanin0(pObj));
        Aig_Obj_t* pFanin1 = Aig_Regular(Aig_ObjFanin1(pObj));

        std::cout << "  id " << pObj->Id
                  << " = AND("
                  << (Aig_ObjFaninC0(pObj) ? "!" : "") << pFanin0->Id
                  << ", "
                  << (Aig_ObjFaninC1(pObj) ? "!" : "") << pFanin1->Id
                  << ")\n";
    }
}

bool BuildTargetInfo(Aig_Man_t* pAig,
                     Abc_Ntk_t* pNtk,
                     int argc,
                     char* argv[],
                     TargetInfo& target)
{
    target.node = SelectTargetNode(pAig, argc, argv);

    if (target.node == nullptr) {
        if (argc >= 3) {
            std::cerr << "Error: target ID " << argv[2]
                      << " is not a valid AND node.\n";
        } else {
            std::cerr << "Error: no valid target AND node found.\n";
        }

        return false;
    }

    target.inputs = CollectTargetFanins(target.node);

    if (!ValidateTargetFaninsArePis(target.inputs))
        return false;

    std::vector<std::string> ciNameByAigId;

    if (!BuildCiNameLookup(pAig, pNtk, ciNameByAigId))
        return false;

    if (!FillInputNames(target.inputs, ciNameByAigId, target.inputNames))
        return false;

    target.targetName = "target_" + std::to_string(target.node->Id);
    target.modelName = "local_target_" + std::to_string(target.node->Id);

    target.hasOneCube = BuildTargetAndCube(
        target.node,
        target.inputs,
        target.onCube
    );

    return true;
}

Vec_Ptr_t* MakeNameVec(const std::vector<std::string>& names)
{
    Vec_Ptr_t* vec = Vec_PtrAlloc(names.size());

    for (const std::string& name : names)
        Vec_PtrPush(vec, const_cast<char*>(name.c_str()));

    return vec;
}