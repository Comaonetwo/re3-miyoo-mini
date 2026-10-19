#pragma once
#include <cstdint>
namespace soft {
// Exact 8-bit four-tap bilinear interpolation. Difference form needs three
// integer multiplies instead of four; all intermediates fit signed 32-bit.
inline unsigned bilinearChannel(unsigned a,unsigned b,unsigned c,unsigned d,unsigned fx,unsigned fy){
    int upper=int(a)*256+int(fx)*(int(b)-int(a));
    int lower=int(c)*256+int(fx)*(int(d)-int(c));
    return unsigned((upper*256+int(fy)*(lower-upper)+32768)>>16);
}
}
