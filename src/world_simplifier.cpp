#include <rw.h>
#include "world_simplifier.hpp"
#include "model_clusters.hpp"
#include "meshoptimizer.h"
#include <vector>
#include <array>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <cstdint>
extern "C" size_t meshopt_simplifyWithTriangleIds(unsigned int*,const unsigned int*,size_t,const float*,size_t,size_t,const float*,size_t,const float*,size_t,const unsigned char*,size_t,float,unsigned int,float*,unsigned int*);
namespace soft {
SimplifiedWorld simplifyWorldGeometry(rw::Geometry* g,size_t budget,const Vec3* protectedCamera,float protectedRadius,bool intermediate){SimplifiedWorld result;result.input=g?g->numTriangles:0;
 if(!g||g->numMorphTargets!=1||g->numVertices<16||g->numVertices>8192||g->numTriangles<96||g->numTriangles>4096||g->numTexCoordSets>1||g->matList.numMaterials<1||g->matList.numMaterials>32||(g->flags&rw::Geometry::NATIVE)||!g->morphTargets[0].vertices||!g->triangles)return result;
 if((g->flags&rw::Geometry::NORMALS)&&!g->morphTargets[0].normals)return result;
 if((g->flags&rw::Geometry::PRELIT)&&!g->colors)return result;if(g->numTexCoordSets&&!g->texCoords[0])return result;
 auto positions=g->morphTargets[0].vertices;ModelBounds box;unsigned witness[6]={};std::vector<bool> referenced(g->numVertices);for(int i=0;i<g->numTriangles;++i)for(unsigned k=0;k<3;++k){unsigned v=g->triangles[i].v[k];if(v>=unsigned(g->numVertices))return result;referenced[v]=true;}bool first=true;
 for(int i=0;i<g->numVertices;++i){if(!referenced[i])continue;if(first){for(auto& v:witness)v=i;first=false;}auto p=positions[i];if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||std::fabs(p.x)>1e6f||std::fabs(p.y)>1e6f||std::fabs(p.z)>1e6f)return result;box.include({p.x,p.y,p.z});const float v[]={p.x,p.y,p.z};for(unsigned k=0;k<3;++k){const float a[]={positions[witness[k*2]].x,positions[witness[k*2]].y,positions[witness[k*2]].z},b[]={positions[witness[k*2+1]].x,positions[witness[k*2+1]].y,positions[witness[k*2+1]].z};if(v[k]<a[k])witness[k*2]=i;if(v[k]>b[k])witness[k*2+1]=i;}}
 float extent=std::max({box.maximum.x-box.minimum.x,box.maximum.y-box.minimum.y,box.maximum.z-box.minimum.z});if(!(extent>0))return result;
 std::vector<unsigned char> lock(g->numVertices);for(auto i:witness)lock[i]=1;
 if(protectedCamera&&protectedRadius>0){for(int i=0;i<g->numTriangles;++i){ModelBounds b;for(unsigned k=0;k<3;++k){auto p=positions[g->triangles[i].v[k]];b.include({p.x,p.y,p.z});}float x=std::max({b.minimum.x-protectedCamera->x,0.f,protectedCamera->x-b.maximum.x}),y=std::max({b.minimum.y-protectedCamera->y,0.f,protectedCamera->y-b.maximum.y}),z=std::max({b.minimum.z-protectedCamera->z,0.f,protectedCamera->z-b.maximum.z});if(x*x+y*y+z*z<=protectedRadius*protectedRadius)for(unsigned k=0;k<3;++k)lock[g->triangles[i].v[k]]|=meshopt_SimplifyVertex_Lock;}}
 // The intermediate recipe keeps all ground/roof/ramp faces verbatim.
 // They bypass simplification and are merged back by original source ID.
 std::vector<uint8_t> preserved(g->numTriangles);
 if(intermediate)for(int i=0;i<g->numTriangles;++i){auto t=g->triangles[i];auto a=positions[t.v[0]],b=positions[t.v[1]],c=positions[t.v[2]];
  double ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z,vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
  double nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx,length2=nx*nx+ny*ny+nz*nz;
  if(!(length2>0)||nz*nz>=length2*.1225)preserved[i]=1;
 }
 std::vector<std::array<float,10>> attributes(g->numVertices);float weights[]={2,2,.1f,.1f,.1f,.3f,.3f,.3f,.3f,10};if(intermediate){weights[0]=weights[1]=.5f;weights[2]=weights[3]=weights[4]=.03f;weights[5]=weights[6]=weights[7]=.1f;}
 for(int i=0;i<g->numVertices;++i){auto& a=attributes[i];if(g->numTexCoordSets&&g->texCoords[0]){a[0]=g->texCoords[0][i].u;a[1]=g->texCoords[0][i].v;}if(g->morphTargets[0].normals){auto n=g->morphTargets[0].normals[i];a[2]=n.x;a[3]=n.y;a[4]=n.z;}if((g->flags&rw::Geometry::PRELIT)&&g->colors){auto c=g->colors[i];a[5]=c.red/255.f;a[6]=c.green/255.f;a[7]=c.blue/255.f;a[8]=c.alpha/255.f;}for(float v:a)if(!std::isfinite(v)||std::fabs(v)>1e5)return result;}
 struct PositionKey{uint32_t x,y,z;bool operator==(const PositionKey& b)const{return x==b.x&&y==b.y&&z==b.z;}};struct PositionHash{size_t operator()(const PositionKey& k)const{return size_t(k.x)*73856093u^size_t(k.y)*19349663u^size_t(k.z)*83492791u;}};struct Seam{unsigned first;bool protect;Seam(unsigned f=0,bool p=false):first(f),protect(p){}};
 auto key=[](rw::V3d p){PositionKey k;float v[]={p.x==0?0:p.x,p.y==0?0:p.y,p.z==0?0:p.z};std::memcpy(&k,v,sizeof k);return k;};
 std::unordered_map<PositionKey,Seam,PositionHash> seams;seams.reserve(g->numVertices);
 for(unsigned i=0;i<unsigned(g->numVertices);++i){auto k=key(positions[i]);auto found=seams.find(k);if(found==seams.end())seams.emplace(k,Seam{i,false});else{auto& group=found->second;auto& a=attributes[group.first];auto& b=attributes[i];if(std::fabs(a[0]-b[0])>1e-6f||std::fabs(a[1]-b[1])>1e-6f)group.protect=true;}}
 for(unsigned i=0;i<unsigned(g->numVertices);++i)if(seams[key(positions[i])].protect&&!intermediate)lock[i]|=meshopt_SimplifyVertex_Protect;
 std::vector<unsigned> input(g->numTriangles*3),ids(g->numTriangles),firstMaterial(g->numVertices,~0u),indices(input.size());std::vector<unsigned short> materials;std::vector<unsigned> materialMask(g->numVertices);
 for(int i=0;i<g->numTriangles;++i){auto t=g->triangles[i];if(t.matId>=g->matList.numMaterials)return result;ids[i]=i;for(unsigned k=0;k<3;++k){unsigned v=t.v[k];if(v>=unsigned(g->numVertices))return result;input[i*3+k]=v;materialMask[v]|=1u<<t.matId;if(!intermediate||!preserved[i]){if(firstMaterial[v]==~0u)firstMaterial[v]=t.matId;else if(firstMaterial[v]!=t.matId)lock[v]=1;}}}
 // Material IDs are a high-weight categorical certificate, not a rendered
 // vertex attribute. Shared-material boundaries are also locked explicitly.
 for(int i=0;i<g->numVertices;++i)attributes[i][9]=firstMaterial[i]==~0u?0:float(firstMaterial[i]);
 std::vector<unsigned> wallSource,wallInput;
 const unsigned* simplifyInput=input.data();size_t simplifyCount=input.size();
 if(intermediate){for(unsigned i=0;i<unsigned(g->numTriangles);++i)if(!preserved[i]){wallSource.push_back(i);for(unsigned k=0;k<3;++k)wallInput.push_back(input[i*3+k]);}if(wallInput.empty()){result.reason=9;return result;}simplifyInput=wallInput.data();simplifyCount=wallInput.size();}
 size_t target=std::max(size_t(3),(simplifyCount*3/5)/3*3);float error=0;
 size_t count=meshopt_simplifyWithTriangleIds(indices.data(),simplifyInput,simplifyCount,&positions[0].x,g->numVertices,sizeof(rw::V3d),attributes[0].data(),sizeof(attributes[0]),weights,10,lock.data(),target,(intermediate?std::min(.5f,extent*.012f):std::min(.35f,extent*.004f)),(intermediate?0:meshopt_SimplifyLockBorder)|meshopt_SimplifyErrorAbsolute|meshopt_SimplifySparse|meshopt_SimplifyPermissive,&error,ids.data());
 if(intermediate){
  if(count%3||count>simplifyCount)return result;for(unsigned i=0;i<count/3;++i){if(ids[i]>=wallSource.size())return result;ids[i]=wallSource[ids[i]];}
  std::vector<unsigned> combined,combinedIds;combined.reserve(input.size());combinedIds.reserve(g->numTriangles);unsigned cursor=0;
  for(unsigned i=0;i<unsigned(g->numTriangles);++i){if(preserved[i]){combinedIds.push_back(i);for(unsigned k=0;k<3;++k)combined.push_back(input[i*3+k]);}
   else if(cursor<count/3&&ids[cursor]==i){combinedIds.push_back(i);for(unsigned k=0;k<3;++k)combined.push_back(indices[cursor*3+k]);++cursor;}}
  if(cursor!=count/3)return result;indices.swap(combined);ids.swap(combinedIds);count=indices.size();
 }
 if(!count||count>input.size()||count%3)return result;
 if(intermediate){unsigned cursor=0;for(unsigned i=0;i<unsigned(g->numTriangles);++i){bool retained=cursor<count/3&&ids[cursor]==i;if(preserved[i]){if(!retained){result.reason=8;return result;}for(unsigned k=0;k<3;++k)if(indices[cursor*3+k]!=g->triangles[i].v[k]){result.reason=8;return result;}}if(retained)++cursor;}}
 result.error=error;indices.resize(count);materials.reserve(count/3);
 for(unsigned i=0;i<count/3;++i){if(ids[i]>=unsigned(g->numTriangles)||(i&&ids[i]<=ids[i-1]))return result;unsigned mat=g->triangles[ids[i]].matId;for(unsigned k=0;k<3;++k)if(!(materialMask[indices[i*3+k]]&(1u<<mat)))return result;materials.push_back(mat);}

 result.output=indices.size()/3;result.reason=5;
 // Track every removed or changed source face plus its new extent. Stable
 // source IDs preserve materials and allow conservative camera-distance proof.
 struct Cell{int x,y,z;bool operator==(const Cell& b)const{return x==b.x&&y==b.y&&z==b.z;}};struct CellHash{size_t operator()(const Cell& k)const{return unsigned(k.x)*73856093u^unsigned(k.y)*19349663u^unsigned(k.z)*83492791u;}};
 std::unordered_map<Cell,ModelBounds,CellHash> regions;unsigned cursor=0;
 for(unsigned i=0;i<unsigned(g->numTriangles);++i){bool retained=cursor<count/3&&ids[cursor]==i;bool changed=!retained;if(retained)for(unsigned k=0;k<3;++k)changed|=indices[cursor*3+k]!=g->triangles[i].v[k];if(changed){ModelBounds b;for(unsigned k=0;k<3;++k){auto p=positions[g->triangles[i].v[k]];b.include({p.x,p.y,p.z});if(retained){auto q=positions[indices[cursor*3+k]];b.include({q.x,q.y,q.z});}}Cell cell{int(std::floor((b.minimum.x+b.maximum.x)/16)),int(std::floor((b.minimum.y+b.maximum.y)/16)),int(std::floor((b.minimum.z+b.maximum.z)/16))};auto& region=regions[cell];region.include(b.minimum);region.include(b.maximum);}if(retained)++cursor;}
 if(regions.size()>128){ModelBounds whole;for(auto& entry:regions){whole.include(entry.second.minimum);whole.include(entry.second.maximum);}result.changedBounds.push_back(whole);}else for(auto& entry:regions)result.changedBounds.push_back(entry.second);
 if(indices.size()*10>=size_t(g->numTriangles)*3*9)return result; // require >10% actual reduction
 std::vector<unsigned> remap(g->numVertices,~0u),source;source.reserve(g->numVertices);for(auto& v:indices){if(v>=unsigned(g->numVertices))return result;if(remap[v]==~0u){remap[v]=source.size();source.push_back(v);}v=remap[v];}
 result.reason=6;ModelBounds reducedBox;for(auto v:source){auto p=positions[v];reducedBox.include({p.x,p.y,p.z});}if(reducedBox.minimum.x!=box.minimum.x||reducedBox.minimum.y!=box.minimum.y||reducedBox.minimum.z!=box.minimum.z||reducedBox.maximum.x!=box.maximum.x||reducedBox.maximum.y!=box.maximum.y||reducedBox.maximum.z!=box.maximum.z)return result;
 result.reason=7;size_t bytes=512+result.changedBounds.capacity()*sizeof(ModelBounds)+source.size()*(12+((g->flags&rw::Geometry::NORMALS)?12:0)+((g->flags&rw::Geometry::PRELIT)?4:0)+g->numTexCoordSets*8)+materials.size()*sizeof(rw::Triangle)+g->matList.numMaterials*sizeof(void*);if(bytes>budget||source.size()>65535)return result;
 auto low=rw::Geometry::create(source.size(),materials.size(),g->flags);if(!low)return result;if(low->numTexCoordSets!=g->numTexCoordSets){low->destroy();return result;}
 for(int i=0;i<g->matList.numMaterials;++i){if(!g->matList.materials[i]){low->destroy();return result;}low->matList.appendMaterial(g->matList.materials[i]);}
 for(unsigned i=0;i<source.size();++i){unsigned v=source[i];low->morphTargets[0].vertices[i]=positions[v];if(low->morphTargets[0].normals)low->morphTargets[0].normals[i]=g->morphTargets[0].normals[v];if(low->colors)low->colors[i]=g->colors[v];if(low->numTexCoordSets&&low->texCoords[0])low->texCoords[0][i]=g->texCoords[0][v];}
 for(unsigned i=0;i<materials.size();++i){auto& t=low->triangles[i];t.matId=materials[i];for(unsigned k=0;k<3;++k)t.v[k]=indices[i*3+k];}low->calculateBoundingSphere();result.reason=0;result.geometry=low;result.bytes=bytes;return result;
}
}
