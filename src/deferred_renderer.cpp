#include "soft_renderer.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <stdexcept>
#include <cmath>
#include <cstdlib>
namespace soft {
struct Renderer::DeferredPool {
 std::unique_ptr<Renderer> top;Renderer bottom;
 std::vector<Command> jobs;std::mutex mutex;std::condition_variable cv;
 bool stop=false,pending=false,finished=false,pipelineJob=false,running=false,needsSeed=true;
 const Renderer* source=nullptr;unsigned split=0,height=0;
 std::exception_ptr failure;std::thread worker;
 DeferredPool():worker([this]{std::unique_lock<std::mutex> lock(mutex);for(;;){
  cv.wait(lock,[this]{return pending||stop;});if(stop)return;pending=false;
  const Renderer* current=source;const unsigned lo=split,hi=height;const bool pipeline=pipelineJob;lock.unlock();
  std::exception_ptr error;try{
   if(!pipeline){bottom.syncRaster(*current);bottom.resetReplay(*current,lo,hi);}
   bottom.stats_={};if(pipeline)bottom.replayCommands(jobs,0,hi);else bottom.replayFrom(*current,lo,hi);
  }catch(...){error=std::current_exception();}
  lock.lock();failure=error;finished=true;cv.notify_one();
 }}){}
 ~DeferredPool(){ {std::lock_guard<std::mutex> lock(mutex);stop=true;}cv.notify_one();worker.join();}
 void wait(){std::unique_lock<std::mutex> lock(mutex);cv.wait(lock,[this]{return finished;});}
 // Estimate row work from clipped command boxes, without scanning pixels.
 // Screen halves often contain very different amounts of city geometry.
 static unsigned balancedSplit(const Renderer& r){
  const unsigned h=r.renderHeight();const char* env=std::getenv("GTA3_BALANCED_BANDS");if(env&&env[0]=='0')return h/2;
  float diff[241]={},weight[240]={};
  for(const auto& c:r.commands_){
   if(!std::isfinite(c.low)||!std::isfinite(c.high))continue;
   unsigned lo=unsigned(std::max(0.f,std::min(float(h),std::floor(c.low)))),hi=unsigned(std::max(0.f,std::min(float(h),std::ceil(c.high))));if(lo>=hi)continue;
   float width=0;
   if(c.hasSetup)width=float(c.setup.x1-c.setup.x0+1);
   else{float left=float(r.renderWidth()),right=0;for(const auto& v:c.v){float x=c.kind==2?(std::fabs(v.reciprocalZ)>1e-8f?(160+c.focal*v.x/v.reciprocalZ)/r.scale_:0):c.kind==1?v.x:v.x/r.scale_;if(!std::isfinite(x)){left=0;right=float(r.renderWidth());break;}left=std::min(left,x);right=std::max(right,x);}width=std::min(float(r.renderWidth()),right)-std::max(0.f,left);}
   if(width>0&&std::isfinite(width)){diff[lo]+=width;diff[hi]-=width;}
  }
  float row=0,total=0;for(unsigned y=0;y<h;++y){row+=diff[y];weight[y]=std::max(0.f,row);total+=weight[y];}
  if(!(total>0)||!std::isfinite(total))return h/2;
  float prefix=0;for(unsigned y=0;y+1<h;++y){prefix+=weight[y];if(prefix>=total*.5f)return y+1;}return h/2;
 }
 void draw(const Renderer& current){if(!top)top.reset(new Renderer);const unsigned hi=current.renderHeight(),mid=balancedSplit(current);
  {std::lock_guard<std::mutex> lock(mutex);source=&current;height=hi;split=mid;pipelineJob=false;failure=nullptr;finished=false;pending=true;}cv.notify_one();
  std::exception_ptr error;try{top->syncRaster(current);top->resetReplay(current,0,mid);top->stats_={};top->replayFrom(current,0,mid);}catch(...){error=std::current_exception();}
  wait();if(!error)error=failure;if(error)std::rethrow_exception(error);
 }
};
void Renderer::syncRaster(const Renderer& s){
#define COPY(name) name=s.name
 COPY(tileScan_);COPY(rainComposite_);COPY(rowExpansion_);COPY(skyComposite_);COPY(skyChannels_);COPY(flatSpans_);COPY(worldChannels_);COPY(scale_);COPY(requestedScale_);COPY(occlusionEnabled_);COPY(tileDepth_);COPY(lazyDepth_);COPY(lazyFull_);COPY(lazyThisFrame_);COPY(lazySampleMask_);
 COPY(rgbRegister_);COPY(sparseHud_);COPY(alphaReject_);COPY(filterStages_);COPY(vectorFilter_);COPY(alphaSpans_);COPY(smallDepth_);COPY(whiteBlend_);COPY(constantAlpha_);COPY(additiveBounds_);COPY(earlyDepth_);COPY(nearestOpacity_);COPY(nearestCoverage_);COPY(leanSetup_);COPY(transparentWorld_);COPY(flatAlpha_);
 COPY(effectSpans_);COPY(particleSpans_);COPY(zeroMasks_);COPY(blackAdd_);COPY(additiveCache_);COPY(screenCertificates_);COPY(opaqueOverlay_);COPY(exactAdvance_);COPY(preparedWorld_);COPY(modulationTables_);COPY(packedWorld_);COPY(flatOcclusion_);COPY(batchCounters_);COPY(batchGeneral_);COPY(depthRows_);COPY(depthBlocks_);COPY(gradientDepthRows_);COPY(opaqueTextures_);COPY(edgeRuns_);COPY(texelRuns_);COPY(packetSampler_);
#undef COPY
 nativeUi_=hudOverlay_=rainActive_=skyActive_=radarClip_=false;
 deferredActive_=false;
}
void Renderer::addRasterStats(const Stats& value){
#include "stats_accumulate.inc"
}
void Renderer::beginDeferred(unsigned mode,unsigned limit){
 endDeferred();if(mode<1||mode>3||nativeUi_||hudOverlay_||rainActive_||skyActive_)return;
 deferredLimit_=std::max(64u,std::min(4096u,limit));
 try{commands_.reserve(deferredLimit_);}catch(const std::bad_alloc&){++deferredFallbacks_;return;}
 if(mode>=2){try{if(!deferredPool_||!deferredPool_.unique())deferredPool_.reset(new DeferredPool);if(mode==3){deferredPool_->jobs.reserve(deferredLimit_);deferredPool_->needsSeed=true;}}catch(const std::exception&){mode=1;++deferredFallbacks_;}}
 deferredMode_=mode;deferredActive_=true;
}
void Renderer::endDeferred(){flushDeferred();deferredActive_=false;deferredMode_=0;}
bool Renderer::deferCommand(const ScreenVertex* input,const Texture* texture,const ScreenState& state,bool verified,const WorldTriangleSetup* setup,unsigned kind,uint16_t tint,uint8_t au,uint8_t av,unsigned alpha,float focal){
 if(!deferredActive_||deferredReplaying_)return false;
 if(nativeUi_||hudOverlay_||rainActive_||skyActive_||(kind==0&&(!state.zTest||state.screenDepth||state.blend>1))){flushDeferred();return false;}
 if(commands_.size()>=deferredLimit_){if(deferredMode_==3)dispatchPipeline();else flushDeferred();}
 recordCommand(input,texture,state,verified,setup,kind,tint,au,av,alpha,focal);++deferredCommands_;return true;
}
void Renderer::finishPipeline(){
 if(!deferredPool_||!deferredPool_->running)return;auto& pool=*deferredPool_;pool.wait();pool.running=false;
 if(pool.failure){deferredMode_=1;++deferredFallbacks_;replayCommands(pool.jobs,0,renderHeight());pool.needsSeed=true;}
 else{mergeRows(pool.bottom,0,renderHeight());addRasterStats(pool.bottom.stats_);}
 pool.jobs.clear();
}
void Renderer::dispatchPipeline(){
 finishPipeline();if(commands_.empty())return;if(deferredMode_!=3){flushDeferred();return;}
 auto& pool=*deferredPool_;
 if(pool.needsSeed){try{pool.bottom.syncRaster(*this);pool.bottom.resetReplay(*this,0,renderHeight());if(pool.bottom.flatAlpha_&&pool.bottom.flatAlphaTables_.empty())pool.bottom.flatAlphaTables_.resize(4);pool.needsSeed=false;}catch(...){deferredMode_=1;++deferredFallbacks_;flushDeferred();return;}}
 commands_.swap(pool.jobs);++deferredBatches_;
 {std::lock_guard<std::mutex> lock(pool.mutex);pool.source=nullptr;pool.height=renderHeight();pool.split=0;pool.pipelineJob=true;pool.failure=nullptr;pool.finished=false;pool.pending=true;pool.running=true;}pool.cv.notify_one();
}
void Renderer::flushDeferred(){
 if(!deferredActive_||deferredReplaying_)return;
 if(deferredMode_==3||(deferredPool_&&deferredPool_->running)){
  finishPipeline();if(!commands_.empty()){if(deferredMode_==3){dispatchPipeline();finishPipeline();}else{++deferredBatches_;replayFrom(*this,0,renderHeight());commands_.clear();}}
  if(deferredPool_)deferredPool_->needsSeed=true;return;
 }
 if(commands_.empty())return;++deferredBatches_;
 if(deferredMode_==2&&deferredPool_){
  bool done=false;try{deferredPool_->draw(*this);done=true;}catch(...){deferredMode_=1;++deferredFallbacks_;}
  if(done){const unsigned hi=renderHeight(),mid=deferredPool_->split;mergeRows(*deferredPool_->top,0,mid);mergeRows(deferredPool_->bottom,mid,hi);addRasterStats(deferredPool_->top->stats_);addRasterStats(deferredPool_->bottom.stats_);commands_.clear();return;}
 }
 replayFrom(*this,0,renderHeight());commands_.clear();
}
size_t Renderer::deferredBytes()const{
 size_t n=commands_.capacity()*sizeof(Command);
 if(deferredPool_){n+=sizeof(DeferredPool)+deferredPool_->jobs.capacity()*sizeof(Command)+deferredPool_->bottom.bufferBytes()+deferredPool_->bottom.flatAlphaTables_.capacity()*sizeof(FlatAlphaTable);if(deferredPool_->top)n+=sizeof(Renderer)+deferredPool_->top->bufferBytes()+deferredPool_->top->flatAlphaTables_.capacity()*sizeof(FlatAlphaTable);}
 return n;
}
}
