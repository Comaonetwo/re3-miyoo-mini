import pathlib,struct,sys,unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'scripts'))
from txd_decode import decode_txd

def chunk(ident,data):return struct.pack('<III',ident,len(data),0x1803ffff)+data

def native(name='test',palette=True,rgba=(255,0,0,255)):
    head=struct.pack('<II32s32sIIHHBBBB',8,0x1106,name.encode(),b'',0x2500 if palette else 0x500,1,2,2,8 if palette else 32,1,4,0)
    if palette:
        p=bytearray(1024);p[4:8]=bytes(rgba);p[8:12]=bytes([0,255,0,0]);p[12:16]=bytes([0,0,255,255]);p[16:20]=bytes([255,255,255,128]);raw=bytes([1,2,3,4]);head+=p
    else:
        r,g,b,a=rgba;raw=bytes([b,g,r,a])*4
    return chunk(21,chunk(1,head+struct.pack('<I',len(raw))+raw)+chunk(3,b''))

def txd(*rasters):return chunk(22,chunk(1,struct.pack('<HH',len(rasters),8))+b''.join(rasters)+chunk(3,b''))

class Tests(unittest.TestCase):
    def test_pal8_rgba_order_and_alpha(self):
        w,h,data=decode_txd(txd(native()))['test']
        self.assertEqual((w,h),(2,2));self.assertEqual(data,struct.pack('<HBHBHBHB',0xf800,255,0x7e0,0,0x1f,255,0xffff,128))
    def test_bgra8888(self):
        self.assertEqual(decode_txd(txd(native(palette=False,rgba=(0,0,255,128))))['test'][2],struct.pack('<HB',0x1f,128)*4)
    def test_first_duplicate_wins(self):
        self.assertEqual(decode_txd(txd(native(),native(rgba=(0,0,255,255))))['test'][2][:3],struct.pack('<HB',0xf800,255))
    def test_truncated_and_corrupt(self):
        data=txd(native())
        for n in range(len(data)):
            with self.assertRaises(ValueError):decode_txd(data[:n])
        raster=bytearray(native());raster[24+87]=1 # compression in nested struct
        with self.assertRaises(ValueError):decode_txd(txd(raster))
        with self.assertRaises(ValueError):decode_txd(data,65)
    def test_img_padding(self):self.assertEqual(decode_txd(txd(native())+b'\0'*100)['test'][0],2)

if __name__=='__main__':unittest.main()
