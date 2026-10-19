#!/usr/bin/env python3
"""Compile an unmodified pinned re3 tree for ARM, excluding no .cpp units.
This is an object compilation audit, NOT a linked/runnable game. Audio backend
macros are deliberately unset: actual OpenAL/mpg123 integration is outstanding.
"""
import argparse,pathlib,subprocess,json,os,concurrent.futures,hashlib,time
p=argparse.ArgumentParser();p.add_argument('--zig',type=pathlib.Path,required=True);p.add_argument('--re3',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--jobs',type=int,default=4)
a=p.parse_args();project=pathlib.Path(__file__).resolve().parents[1];re3=a.re3.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
commit=subprocess.check_output(['git','-C',str(re3),'rev-parse','HEAD'],text=True).strip()
if commit!='3233ffe1c4b99e8efb4c41c6794b4fce880cf503':raise ValueError('wrong re3 revision')
source=re3/'src';paths=sorted(source.rglob('*.cpp'));includes=sorted({f.parent for f in source.rglob('*.h')})
flags=[str(a.zig.resolve()),'c++','-target','arm-linux-musleabihf','-mcpu=cortex_a7','-std=c++11','-O1','-w','-DLIBRW','-DRW_NULL','-DNDEBUG','-DCMAKE_NO_AUTOLINK','-include',str(project/'src/re3_soft_compat.hpp'),'-I'+str(re3/'vendor/librw'),'-I'+str(project/'src'),'-I'+str(re3)]
flags+=['-I'+str(f) for f in includes];env=os.environ.copy();env.setdefault('ZIG_GLOBAL_CACHE_DIR',str(out/'zig-cache'))
def compile_unit(f):
 rel=f.relative_to(source).as_posix();name=rel.replace('/','_');log=out/(name+'.log');obj=out/(name+'.o');start=time.monotonic()
 with log.open('w') as stream:r=subprocess.run(flags+['-c',str(f),'-o',str(obj)],env=env,stdout=stream,stderr=stream)
 return dict(source=rel,success=r.returncode==0,seconds=round(time.monotonic()-start,3),object_bytes=obj.stat().st_size if r.returncode==0 else 0,error=log.read_text()[:8000] if r.returncode else '')
results=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=max(1,min(a.jobs,8))) as executor:
 for r in executor.map(compile_unit,paths):results.append(r)
report=dict(re3=commit,target='ARMv7 Cortex-A7 musl hard-float',compiled=sum(r['success'] for r in results),total=len(results),audio_backend='unset for compile audit',linked=False,results=results)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print(f"re3 ARM object audit: {report['compiled']}/{report['total']} compiled; not linked; report {out/'report.json'}")
for r in results:
 if not r['success']:print(r['source']+': '+next((line for line in r['error'].splitlines() if 'error:' in line),r['error'].splitlines()[0]))

raise SystemExit(0 if report['compiled']==report['total'] else 1)
