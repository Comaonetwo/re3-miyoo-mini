import unittest,tempfile,pathlib,sys,json,hashlib,importlib.util,shutil,io
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]))
import backend as b
import threading
class Transactions(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.root=pathlib.Path(self.temp.name);self.card=self.root/'card';self.card.mkdir();(self.card/'.tmp_update').mkdir();(self.card/'Emu/PORTS').mkdir(parents=True);(self.card/'Emu/PORTS/launch_standalone.sh').write_text('test')
  self.port=self.root/'port';self.game=self.root/'game';(self.game/'audio_pcm').mkdir(parents=True)
  tracks=[]
  for i in range(196):
   data=bytes([i,0])*8;(self.game/'audio_pcm'/('%03d.pcm'%i)).write_bytes(data);tracks.append({'id':i,'pcm_sha256':hashlib.sha256(data).hexdigest()})
  (self.game/'audio_pcm/manifest.json').write_text(json.dumps({'tracks':tracks}))
  self.target=self.card/'Roms/PORTS/Games/GTA3Core14';self.target.mkdir(parents=True);(self.target/'re3_cpu').write_bytes(b'old engine');(self.target/'settings.cfg').write_bytes(b'custom settings');(self.target/'game/userfiles').mkdir(parents=True);(self.target/'game/userfiles/1.b').write_bytes(b'user save')
  for n in ['Games/GTA3Core14','Shortcuts','Imgs']:(self.port/'Roms/PORTS'/n).mkdir(parents=True)
  (self.port/'Roms/PORTS/Games/GTA3Core14/re3_cpu').write_bytes(b'new engine')
  for n in ['Shortcuts/game.port','Imgs/game.png']:
   (self.port/'Roms/PORTS'/n).write_bytes(b'new');p=self.card/'Roms/PORTS'/n;p.parent.mkdir(exist_ok=True);p.write_bytes(b'old')
  b.CANCEL=self.root/'cancel';self.identity=b.validate_card(self.card,True)
 def tearDown(self):self.temp.cleanup();b.CANCEL=None
 def check_old(self):
  self.assertEqual((self.target/'re3_cpu').read_bytes(),b'old engine');self.assertEqual((self.target/'game/userfiles/1.b').read_bytes(),b'user save');self.assertEqual((self.card/'Roms/PORTS/Shortcuts/game.port').read_bytes(),b'old');self.assertFalse((self.card/'.gta3-installer.lock').exists())
 def test_success_preserves_state_and_backup(self):
  backup=b.commit_card(self.card,self.game,self.port,self.identity,True)
  self.assertEqual((self.target/'re3_cpu').read_bytes(),b'new engine');self.assertEqual((self.target/'settings.cfg').read_bytes(),b'custom settings');self.assertEqual((self.target/'game/userfiles/1.b').read_bytes(),b'user save');self.assertEqual((backup/'re3_cpu').read_bytes(),b'old engine');self.assertFalse((self.card/'.gta3-installer.lock').exists())
 def test_game_swap_failure_rolls_back(self):
  def fault(stage):raise OSError('injected failure')
  with self.assertRaises(OSError):b.commit_card(self.card,self.game,self.port,self.identity,True,fault)
  self.check_old()
 def test_auxiliary_failure_rolls_back(self):
  def fault(stage):
   if stage=='after_aux':raise OSError('injected cover/shortcut failure')
  with self.assertRaises(OSError):b.commit_card(self.card,self.game,self.port,self.identity,True,fault)
  self.check_old()
 def test_cancel_before_copy(self):
  b.CANCEL.touch()
  with self.assertRaises(b.Cancelled):b.commit_card(self.card,self.game,self.port,self.identity,True)
  self.check_old()
 def test_cancel_during_copy(self):
  real=b.copy_checked
  def copy(src,dst):real(src,dst);b.CANCEL.touch()
  b.copy_checked=copy
  try:
   with self.assertRaises(b.Cancelled):b.commit_card(self.card,self.game,self.port,self.identity,True)
  finally:b.copy_checked=real
  self.check_old()
 def test_bad_card(self):
  (self.card/'Emu/PORTS/launch_standalone.sh').unlink()
  with self.assertRaises(b.InstallError):b.validate_card(self.card,True)
 def test_symlink_parent_rejected(self):
  shutil.rmtree(self.card/'Roms/PORTS/Imgs');(self.card/'Roms/PORTS/Imgs').symlink_to(self.root,target_is_directory=True)
  with self.assertRaises(b.InstallError):b.validate_card(self.card,True)
 def test_corrupt_pcm_rolls_back(self):
  (self.game/'audio_pcm/123.pcm').write_bytes(b'corrupted')
  with self.assertRaises(b.InstallError):b.commit_card(self.card,self.game,self.port,self.identity,True)
  self.check_old()
 def test_existing_lock_untouched(self):
  (self.card/'.gta3-installer.lock').mkdir()
  with self.assertRaises(b.InstallError):b.commit_card(self.card,self.game,self.port,self.identity,True)
  self.assertTrue((self.card/'.gta3-installer.lock').exists())
 def test_select_same_disc_fails_before_card_write(self):
  p=self.root/'disc.iso';p.write_bytes(b'not iso')
  with self.assertRaises(b.InstallError):b.install(p,p,self.card,self.root,self.root/'log',True)
  self.check_old()
 def test_cancel_child_process(self):
  timer=threading.Timer(.2,b.CANCEL.touch);timer.start()
  try:
   with self.assertRaises(b.Cancelled):b.run([sys.executable,'-c','import time;time.sleep(30)'],io.StringIO())
  finally:timer.join()
  self.check_old()
 def test_wrong_volume_rejected(self):
  with self.assertRaises(b.InstallError):b.commit_card(self.card,self.game,self.port,self.identity+1,True)
  self.check_old()
 def test_iso_extraction(self):
  p=self.root/'tiny.iso';iso=b.pycdlib.PyCdlib();iso.new();data=io.BytesIO(b'our own test data');iso.add_fp(data,17,iso_path='/TEST.TXT;1');iso.write(str(p));iso.close();entries=b.list_iso(p);self.assertIn('test.txt',entries);b.extract_iso(p,entries,self.root/'extracted');self.assertEqual((self.root/'extracted/test.txt').read_bytes(),b'our own test data')
 def test_no_free_space(self):
  real=shutil.disk_usage;shutil.disk_usage=lambda path:real(path)._replace(free=0)
  try:
   with self.assertRaises(b.InstallError):b.commit_card(self.card,self.game,self.port,self.identity,True)
  finally:shutil.disk_usage=real
  self.check_old()
if __name__=='__main__':unittest.main()
