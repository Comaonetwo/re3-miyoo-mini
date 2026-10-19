#include "re3_soft_compat.hpp"
// Compile against the unmodified, pinned re3/fakerw/rwcore.h (not a local copy).
void checkRealRe3VertexAPI(){
    RwIm2DVertex a;a.setScreenX(1);a.setScreenY(2);a.setScreenZ(0);
    a.setCameraZ(2);a.setRecipCameraZ(.5f);a.setU(0,.5f);a.setV(1,.5f);a.setColor(1,2,3,255);
    RwIm3DVertex b;b.setX(1);b.setY(2);b.setZ(3);b.setU(0);b.setV(1);b.setColor(1,2,3,255);
    static_assert(sizeof(RwImVertexIndex)==2,"re3 immediate indices");
}
