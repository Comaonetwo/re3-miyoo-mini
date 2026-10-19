import json, pathlib, sys, struct, unittest, copy
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'scripts'))
from surface_data import rich_row,pack_surface
class SurfaceTests(unittest.TestCase):
 def fixture(self):return dict(p=[[0,0,1],[1,0,1],[0,1,1]],uv=[[0,0],[1,0],[0,1]],rgba=[[255,0,0,255],[0,255,0,255],[0,0,255,255]],normals=[[0,0,1]]*3,material=[60,255,0,51],ambient=1,diffuse=.5,flags=15,texture='CarBody',address=4354)
 def test_roundtrip_properties(self):
  s=self.fixture();row=rich_row(s);self.assertEqual(row[18],'carbody');self.assertEqual(row[20]['material'][3],51)
  packed=pack_surface(s,[[0,0,2],[0,0,1],[0,0,1]],paint=True);self.assertEqual(len(packed),61)
  self.assertEqual(list(packed[12:16]),[45,105,190,51]);self.assertEqual(packed[-1],31);self.assertEqual(struct.unpack_from('<3f',packed,16),(0,0,1));self.assertEqual(s['material'],[60,255,0,51])
 def test_invalid_input(self):
  for key,value in [('ambient',float('nan')),('diffuse',-1),('flags',128),('material',[0,0,0,256]),('uv',[[0,0]])]:
   s=self.fixture();s[key]=value
   with self.assertRaises(ValueError):rich_row(s)
  s=self.fixture();s['p'][0][0]=float('inf')
  with self.assertRaises(ValueError):rich_row(s)
if __name__=='__main__':unittest.main()
