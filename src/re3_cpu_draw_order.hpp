#pragma once
#include <vector>
#include <cstdint>
class CEntity;
void cpuOpaqueOrderEnabled(bool);
uint64_t cpuOpaqueOrderObjects();
// Reorder only certified opaque building slots for this render pass; always
// restore the engine's list before any subsequent render/update stage.
class CpuOpaqueOrder {
    CEntity** entities_;std::vector<unsigned> slots_;
    std::vector<CEntity*> original_;
public:
    CpuOpaqueOrder(CEntity**,int);
    ~CpuOpaqueOrder();
    CpuOpaqueOrder(const CpuOpaqueOrder&)=delete;
    CpuOpaqueOrder& operator=(const CpuOpaqueOrder&)=delete;
};
