#pragma once
#include <string>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
namespace soft {
// Copy old CPU slots; never remove originals or overwrite a canonical slot.
inline unsigned importLegacySaves(const std::string& root,const std::string& folder){unsigned imported=0;
 for(unsigned i=1;i<=8;++i){std::string dest=folder+"/GTA3sf"+std::to_string(i)+".b";if(!access(dest.c_str(),F_OK))continue;
  std::string source=root+"/"+std::to_string(i)+".b";FILE* input=std::fopen(source.c_str(),"rb");if(!input)continue;
  std::string temp=dest+".import-XXXXXX";int fd=mkstemp(&temp[0]);if(fd<0){std::fclose(input);continue;}
  bool ok=true;unsigned char block[8192];size_t n;while((n=std::fread(block,1,sizeof(block),input))){size_t written=0;while(written<n){ssize_t result=write(fd,block+written,n-written);if(result<0&&errno==EINTR)continue;if(result<=0){ok=false;break;}written+=size_t(result);}if(!ok)break;}
  if(std::ferror(input))ok=false;std::fclose(input);if(fsync(fd))ok=false;if(close(fd))ok=false;
  if(ok&&access(dest.c_str(),F_OK)&&!std::rename(temp.c_str(),dest.c_str())){++imported;std::printf("SAVE imported legacy slot=%u original_preserved=1\n",i);}else std::remove(temp.c_str());
 }return imported;
}
}
