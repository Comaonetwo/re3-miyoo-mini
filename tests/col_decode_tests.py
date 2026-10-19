import unittest,sys,pathlib,struct
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'scripts'))
from col_decode import decode_col,surfaces

def fixture():
    body=struct.pack('<24s10f',b'road',10,0,0,0,-1,-1,-1,1,1,1)
    body+=struct.pack('<h2x',1)+struct.pack('<4fBB2x',1,0,0,0,0,0)
    body+=struct.pack('<h2x',0)+struct.pack('<h2x',1)+struct.pack('<6fBB2x',-1,-1,-1,1,1,1,0,0)
    body+=struct.pack('<h2x',3)+struct.pack('<9f',0,0,0,1,0,0,0,1,0)
    body+=struct.pack('<h2x',1)+struct.pack('<iiiBB2x',0,1,2,0,0)
    return b'COLL'+struct.pack('<I',len(body))+body

class Tests(unittest.TestCase):
    def test_layout_and_primitives(self):
        model=decode_col(fixture())['road']
        self.assertEqual(model[0][0],((0,0,0),(1,0,0),(0,1,0)))
        self.assertEqual(model[1][0],((-1,-1,-1),(1,1,1)))
        self.assertEqual(model[2][0],(1,(0,0,0)))
        self.assertEqual(len(list(surfaces(model))),1+12+48)
    def test_truncation(self):
        raw=fixture()
        for n in range(1,len(raw)):
            with self.assertRaises(ValueError):decode_col(raw[:n])
    def test_reference_and_filter(self):
        raw=bytearray(fixture());struct.pack_into('<i',raw,len(raw)-16,3)
        with self.assertRaises(ValueError):decode_col(raw)
        self.assertEqual(decode_col(raw,{'other'}),{})
    def test_format(self):
        raw=bytearray(fixture());raw[:4]=b'COL2'
        with self.assertRaises(ValueError):decode_col(raw)
if __name__=='__main__':unittest.main()
