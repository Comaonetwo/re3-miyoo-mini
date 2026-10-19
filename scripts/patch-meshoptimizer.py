#!/usr/bin/env python3
"""Add stable original triangle IDs to pinned v0.25; other upstream APIs unchanged."""
from pathlib import Path
import argparse,hashlib
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args();assert hashlib.sha256(a.source.read_bytes()).hexdigest()=='a92aca2fa4a0e8e1aca91dbdd23b1c92a1ec2ff748a0827ce47cc8fb940477e8', 'requires original pinned v0.25 simplifier'
s=a.source.read_text()
def replace(old,new):
 global s
 assert s.count(old)==1,(old,s.count(old));s=s.replace(old,new)
replace('const unsigned int* remap)\n{\n\tsize_t write = 0;', 'const unsigned int* remap, unsigned int* triangle_ids = NULL)\n{\n\tsize_t write = 0;')
replace('\t\t\tindices[write + 0] = v0;\n\t\t\tindices[write + 1] = v1;\n\t\t\tindices[write + 2] = v2;\n\t\t\twrite += 3;\n\t\t}\n\t}\n\n\treturn write;', '\t\t\tif (triangle_ids) triangle_ids[write / 3] = triangle_ids[i / 3];\n\t\t\tindices[write + 0] = v0;\n\t\t\tindices[write + 1] = v1;\n\t\t\tindices[write + 2] = v2;\n\t\t\twrite += 3;\n\t\t}\n\t}\n\n\treturn write;')
replace('unsigned int options, float* out_result_error)\n{\n\tusing namespace meshopt;', 'unsigned int options, float* out_result_error, unsigned int* triangle_ids = NULL)\n{\n\tusing namespace meshopt;')
replace('remapIndexBuffer(result, result_count, collapse_remap, remap);', 'remapIndexBuffer(result, result_count, collapse_remap, remap, triangle_ids);')
s+='''\n// Miyoo adapter: retain stable source triangle IDs during compaction so the\n// caller preserves original material assignment and global submission order.\nextern "C" size_t meshopt_simplifyWithTriangleIds(unsigned int* destination,const unsigned int* indices,size_t index_count,const float* positions,size_t vertex_count,size_t stride,const float* attributes,size_t attribute_stride,const float* weights,size_t attribute_count,const unsigned char* lock,size_t target,float error,unsigned int options,float* result_error,unsigned int* triangle_ids){\n assert(!(options & meshopt_SimplifyPrune));\n return meshopt_simplifyEdge(destination,indices,index_count,positions,vertex_count,stride,attributes,attribute_stride,weights,attribute_count,lock,target,error,options,result_error,triangle_ids);\n}\n'''
a.output.write_text(s);print('Patched pinned simplifier:',hashlib.sha256(s.encode()).hexdigest())
