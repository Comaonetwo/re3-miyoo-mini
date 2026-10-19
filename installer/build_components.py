from pathlib import Path
import subprocess,tarfile,os,shutil,concurrent.futures,json,sys,shlex
import argparse,urllib.request,hashlib
ap=argparse.ArgumentParser();ap.add_argument('--work',required=True,type=Path);ap.add_argument('--zig',required=True,type=Path);args=ap.parse_args()
r=Path(__file__).resolve().parents[1];w=args.work.resolve();w.mkdir(parents=True,exist_ok=True);zig=args.zig.resolve();os.environ['ZIG_GLOBAL_CACHE_DIR']=str(w/'zig-cache')
lock=json.loads((r/'installer/dependencies.json').read_text())
for archive,folder in [('ffmpeg.tar.xz','ffmpeg-source'),('zlib.tar.gz','zlib-source'),('unshield.tar.gz','unshield-source')]:
 src=r/'installer/third_party'/archive
 assert hashlib.sha256(src.read_bytes()).hexdigest()==lock[archive]['sha256']
 dst=w/folder
 if not dst.exists():
  dst.mkdir()
  with tarfile.open(src)as t:
   for member in t.getmembers():
    if archive!='unshield.tar.gz':member.name='/'.join(member.name.split('/')[1:])
    if member.name:t.extract(member,dst)
for tag in ['mac-arm64','mac-x64','win-x64']:
 name='python-'+tag+'.tar.gz';archive=w/name
 if not archive.exists():urllib.request.urlretrieve(lock[name]['url'],archive)
 assert hashlib.sha256(archive.read_bytes()).hexdigest()==lock[name]['sha256']
 dst=w/('runtime-'+tag)
 if not dst.exists():
  dst.mkdir()
  with tarfile.open(archive)as t:t.extractall(dst)

def build(tag):
 out=w/('tools-'+tag);out.mkdir(exist_ok=True);log=(w/('build-'+tag+'.txt')).open('w');mac=tag.startswith('mac');arch='arm64'if tag=='mac-arm64'else'x86_64'
 cc=['/usr/bin/clang','-arch',arch,'-mmacosx-version-min=12.0']if mac else[str(zig),'cc','-target','x86_64-windows-gnu']
 wrapper=out/'cc';wrapper.write_text('#!/bin/sh\nexec '+' '.join("'"+s+"'"for s in cc)+' "$@"\n');wrapper.chmod(0o755)
 if not mac:wrapper.write_text('#!'+sys.executable+'\nimport os,sys\ncc='+repr(cc)+'\nos.execv(cc[0],cc+[a for a in sys.argv[1:] if not a.startswith("-Wl,--pic-executable")])\n')
 src=w/'unshield-source';cfg=out/'lib';cfg.mkdir(exist_ok=True)
 (cfg/'unshield_config.h').write_text('#define HAVE_STDLIB_H 1\n#define HAVE_STDINT_H 1\n#define HAVE_STRING_H 1\n#define HAVE_STRINGS_H 1\n#define HAVE_SYS_STAT_H 1\n#define HAVE_SYS_TYPES_H 1\n#define HAVE_UNISTD_H 1\n#define HAVE_MEMORY_H 1\n#define HAVE_INTTYPES_H 1\n#define USE_OUR_OWN_MD5 1\n#define SIZE_FORMAT "zi"\n#define VERSION "1.6.2"\n#define __USE_MINGW_ANSI_STDIO 1\n')
 sources=[src/'src/unshield.c']+list((src/'lib').glob('*.c'))+list((src/'lib/md5').glob('*.c'))
 # Select only actual upstream library files, not test programs.
 sources=[f for f in sources if f.name in ['unshield.c','component.c','converter.c','directory.c','file.c','file_group.c','helper.c','libunshield.c','log.c','md5.c','md5c.c']]
 flags=['-Os','-DHAVE_CONFIG_H','-I'+str(out),'-I'+str(src/'lib')]
 if mac:link=['-lz']
 else:
  z=w/'zlib-source';flags+=['-I'+str(z)]
  sources += [z/(n+'.c')for n in ['adler32','crc32','inflate','inffast','inftrees','zutil']];link=[]
 exe=out/('unshield'if mac else'unshield.exe')
 if not mac:
  rc=out/'manifest.rc';rc.write_text('1 24 \"'+str(r/'installer/native/windows.manifest')+'\"\n')
  resource=out/'manifest.res';subprocess.run([str(zig),'rc','/fo',str(resource),'--',str(rc)],check=True);link+=[str(resource)]
 subprocess.run(cc+flags+[str(f)for f in sources]+link+['-o',str(exe)],stdout=log,stderr=subprocess.STDOUT,check=True)
 ar=out/'ar';ar.write_text('#!/bin/sh\nexec '+shlex.quote(str(zig))+' ar \"$@\"\n');ar.chmod(0o755)
 ranlib=out/'ranlib';ranlib.write_text('#!/bin/sh\nexec '+shlex.quote(str(zig))+' ranlib \"$@\"\n');ranlib.chmod(0o755)
 ff=out/'ffbuild';ff.mkdir(exist_ok=True)
 opts=['--disable-everything','--enable-ffmpeg','--disable-ffplay','--disable-ffprobe','--disable-doc','--disable-debug','--disable-network','--disable-autodetect','--disable-asm','--disable-iconv','--disable-zlib','--disable-bzlib','--disable-lzma','--enable-avcodec','--enable-avformat','--enable-avfilter','--enable-swresample','--enable-protocol=file','--enable-demuxer=wav,mp3','--enable-decoder=mp3float,mp3,pcm_s16le,pcm_u8,adpcm_ima_wav','--enable-encoder=pcm_s16le','--enable-muxer=pcm_s16le','--enable-filter=aresample,anull,aformat','--enable-static','--disable-shared','--enable-cross-compile','--cc='+str(wrapper),'--extra-cflags=-Os','--arch='+('aarch64'if arch=='arm64'else'x86_64'),'--target-os='+('darwin'if mac else'mingw32')]
 if not mac:opts+=['--ar='+str(ar),'--ranlib='+str(ranlib),'--disable-stripping']
 subprocess.run([str(w/'ffmpeg-source/configure')]+opts,cwd=ff,stdout=log,stderr=subprocess.STDOUT,check=True)
 subprocess.run(['make','-j4','ffmpeg'+(''if mac else'.exe')],cwd=ff,stdout=log,stderr=subprocess.STDOUT,check=True)
 built=ff/('ffmpeg'if mac else'ffmpeg.exe')
 if not built.exists()and not mac:built=ff/'ffmpeg_g.exe'
 shutil.copy2(built,out/('ffmpeg'if mac else'ffmpeg.exe'))
 (out/'ffmpeg-build.json').write_text(json.dumps({'configure':opts,'compiler':cc},indent=2)+'\n')
 log.close();print('BUILT',tag,flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=3)as pool:list(pool.map(build,['mac-arm64','mac-x64','win-x64']))
