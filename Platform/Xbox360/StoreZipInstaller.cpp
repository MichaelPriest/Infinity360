#include "StoreZipInstaller.h"
#include "StoreInstallPath.h"
#include "DashboardUpdate.h"
#include "ContentCatalog.h"
#include "StorageInfo.h"
#include "XexLaunchCheck.h"
#include "miniz.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
namespace fs=std::filesystem;
namespace {
constexpr unsigned long long MaxPackage=512ULL*1024*1024,MaxExpanded=1024ULL*1024*1024;
constexpr size_t Budget=12*1024*1024;
struct Allocation {size_t used=0;};
struct alignas(std::max_align_t) Block {size_t bytes;};
void* allocate(void* opaque,size_t items,size_t size){
    auto& b=*static_cast<Allocation*>(opaque);
    if(size&&items>Budget/size)return nullptr;
    const size_t n=items*size;
    if(n>Budget-b.used)return nullptr;
    auto* p=static_cast<Block*>(std::malloc(sizeof(Block)+n));
    if(!p)return nullptr;p->bytes=n;b.used+=n;return p+1;
}
void release(void* opaque,void* pointer){
    if(!pointer)return;
    auto* p=static_cast<Block*>(pointer)-1;
    static_cast<Allocation*>(opaque)->used-=p->bytes;std::free(p);
}
void* resize(void* opaque,void* pointer,size_t items,size_t size){
    if(!pointer)return allocate(opaque,items,size);
    if(size&&items>Budget/size)return nullptr;
    const size_t n=items*size;
    auto* old=static_cast<Block*>(pointer)-1;
    auto& b=*static_cast<Allocation*>(opaque);
    if(n>Budget-b.used+old->bytes)return nullptr;
    const size_t before=old->bytes;
    auto* p=static_cast<Block*>(std::realloc(old,sizeof(Block)+n));
    if(!p)return nullptr;p->bytes=n;b.used=b.used-before+n;return p+1;
}
struct Reader {
    std::ifstream input;
    unsigned long long size;
};
size_t readAt(void* opaque,mz_uint64 offset,void* buffer,size_t n){
    auto& r=*static_cast<Reader*>(opaque);
    if(offset>r.size||n>r.size-offset)return 0;
    r.input.clear();r.input.seekg(static_cast<std::streamoff>(offset));
    if(!r.input)return 0;
    r.input.read(static_cast<char*>(buffer),static_cast<std::streamsize>(n));
    return static_cast<size_t>(r.input.gcount());
}
struct Archive {
    mz_zip_archive zip{};
    Allocation budget;
    bool opened=false;
    ~Archive(){if(opened)mz_zip_reader_end(&zip);}
    bool Open(Reader& r){
        mz_zip_zero_struct(&zip);zip.m_pRead=readAt;zip.m_pIO_opaque=&r;
        zip.m_pAlloc=allocate;zip.m_pFree=release;zip.m_pRealloc=resize;zip.m_pAlloc_opaque=&budget;
        opened=mz_zip_reader_init(&zip,r.size,0)!=0;return opened;
    }
};
std::string lower(std::string s){
    for(char& ch:s)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}
struct Node {std::string name;bool directory;unsigned index;unsigned long long size;};
bool plan(Archive& a,std::map<std::string,Node>& nodes,unsigned long long& expanded,unsigned long long& xexSize){
    const unsigned count=mz_zip_reader_get_num_files(&a.zip);
    if(!count||count>4096)return false;
    std::set<std::string> explicitNames;
    for(unsigned i=0;i<count;++i){
        mz_zip_archive_file_stat st{};
        if(!mz_zip_reader_file_stat(&a.zip,i,&st)||st.m_is_encrypted||!st.m_is_supported||
           (st.m_method!=0&&st.m_method!=8))return false;
        const unsigned mode=(st.m_external_attr>>16)&0170000;
        if(mode&&mode!=(st.m_is_directory?0040000u:0100000u))return false;
        if(st.m_external_attr&8u)return false;
        const unsigned len=mz_zip_reader_get_filename(&a.zip,i,nullptr,0);
        if(len<2||len>142)return false;
        char name[142]{};
        if(mz_zip_reader_get_filename(&a.zip,i,name,sizeof name)!=len||std::strlen(name)!=len-1)return false;
        std::string rel(name);
        if(st.m_is_directory&&rel.back()=='/')rel.pop_back();
        if(rel.empty()||rel.size()>140||rel.front()=='/'||rel.back()=='/'||rel.find('\\')!=std::string::npos)return false;
        const std::string key=lower(rel);
        if(!explicitNames.insert(key).second||key=="infinity-install.txt")return false;
        if(st.m_is_directory&&st.m_uncomp_size)return false;
        if(!st.m_is_directory){
            if(st.m_uncomp_size>MaxPackage||st.m_uncomp_size>MaxExpanded-expanded)return false;
            if(key.size()>=4&&key.substr(key.size()-4)==".xex"&&key!="default.xex")return false;
            if(key=="default.xex"){if(rel!="default.xex"||st.m_uncomp_size<4096)return false;xexSize=st.m_uncomp_size;}
            expanded+=st.m_uncomp_size;
        }
        size_t begin=0;unsigned depth=0;
        for(;;){
            const size_t slash=rel.find('/',begin);
            const bool last=slash==std::string::npos;
            const std::string part=rel.substr(begin,last?std::string::npos:slash-begin);
            if(++depth>12||!FileWorkspace::ValidName(part))return false;
            // Only ASCII FATX names, independent of the active locale.
            for(unsigned char ch:part)if(ch>=128)return false;
            const std::string prefix=last?rel:rel.substr(0,slash);
            const bool dir=!last||st.m_is_directory;
            const std::string k=lower(prefix);
            auto old=nodes.find(k);
            if(old!=nodes.end()){
                if(old->second.name!=prefix||!old->second.directory||!dir)return false;
            }else{
                if(nodes.size()>=8192)return false;
                nodes.emplace(k,Node{prefix,dir,i,dir?0:st.m_uncomp_size});
            }
            if(last)break;
            begin=slash+1;
        }
    }
    return xexSize>0;
}
using Progress=std::function<bool(unsigned long long,unsigned long long)>;
bool digest(Reader& reader,const StoreItem& item,const Progress& progress,unsigned long long base,unsigned long long total){
    StoreSha256 sha;unsigned char buf[32768];unsigned long long pos=0;
    while(pos<item.bytes){
        const size_t n=static_cast<size_t>(std::min<unsigned long long>(sizeof buf,item.bytes-pos));
        if(readAt(&reader,pos,buf,n)!=n)return false;
        sha.Update(buf,n);pos+=n;
        if(!progress(base+pos,total))return false;
    }
    reader.input.clear();reader.input.seekg(0,std::ios::end);
    return reader.input.tellg()>=0&&static_cast<unsigned long long>(reader.input.tellg())==item.bytes&&sha.Finish()==item.sha256;
}
struct Output {
    int fd;unsigned long long limit,done,base,total;
    const Progress& progress;
};
size_t writeChunk(void* opaque,mz_uint64 offset,const void* data,size_t n){
    auto& o=*static_cast<Output*>(opaque);
    if(offset!=o.done||n>o.limit-o.done)return 0;
    size_t pos=0;auto* bytes=static_cast<const unsigned char*>(data);
    while(pos<n){
        const ssize_t wrote=write(o.fd,bytes+pos,n-pos);
        if(wrote<0&&errno==EINTR)continue;
        if(wrote<=0)return 0;pos+=static_cast<size_t>(wrote);
    }
    o.done+=n;return o.progress(o.base+o.done,o.total)?n:0;
}
int exclusive(const std::string& path){
    int flags=O_WRONLY|O_CREAT|O_EXCL;
#ifndef __XSDK__
    flags|=O_NOFOLLOW;
#endif
    return open(path.c_str(),flags,0600);
}
}
bool StoreInstallPackage(const FileWorkspace& ws,const StoreItem& item,const std::string& root,
    const Progress& progress,std::string& error){
    error.clear();
    auto reject=[&](const char* reason){error=reason;return false;};
    const bool dashboard=item.category=="dashboard";
    if(dashboard){
        StoreItem approved{};
        if(!DashboardUpdate::Enabled(root)||!DashboardUpdate::Read(root,approved)||approved.id!=item.id||approved.sha256!=item.sha256||approved.bytes!=item.bytes||approved.version!=item.version)
            return reject("Manifesto de atualizacao ausente, alterado ou nao autorizado.");
    }
    const std::string folder=StoreInstallPath::Folder(root,item);
    if(folder.empty()||item.bytes<4||item.bytes>MaxPackage||item.rights.empty()||
       item.sha256.size()!=64||item.sha256.find_first_not_of("0123456789abcdef")!=std::string::npos)
        return reject("Pacote autorizado invalido.");
    const auto personal=StoreStorage::Personal(root,item.storage);
    const auto source=FileWorkspace::Join(FileWorkspace::Join(personal,"Store"),item.id+".pkg");
    if(!ws.CanReadPersonalFile(source))return reject("Pacote local indisponivel.");
    Reader r{std::ifstream(source.c_str(),std::ios::binary|std::ios::ate),item.bytes};
    if(!r.input||r.input.tellg()<0||static_cast<unsigned long long>(r.input.tellg())!=item.bytes)return reject("Tamanho divergente.");
    char magic[4];if(readAt(&r,0,magic,4)!=4)return reject("Pacote ilegivel.");
    if(std::memcmp(magic,"XEX2",4)==0){r.input.close();return StoreInstallXex(ws,item,root,progress,error);}
    if(std::memcmp(magic,"PK\003\004",4)!=0)return reject("Formato permitido: XEX2 ou ZIP.");
    std::ifstream flag(FileWorkspace::Join(root,"enable-store-install.txt").c_str());std::string marker;
    if(!std::getline(flag,marker)||marker!="ENABLE-STORE-INSTALL"||!ws.CanWrite(personal))
        return reject("Instalacao bloqueada: habilite escrita e instalador.");
    Archive a;
    if(!progress(0,1)||!a.Open(r))return reject("ZIP invalido ou limite de memoria.");
    std::map<std::string,Node> nodes;unsigned long long expanded=0,xexSize=0;
    if(!plan(a,nodes,expanded,xexSize))return reject("ZIP recusado: caminhos, duplicatas, tamanho ou default.xex.");
    if(dashboard){
        const auto required=[&](const char* key,unsigned long long min,unsigned long long max){auto it=nodes.find(key);return it!=nodes.end()&&!it->second.directory&&it->second.size>=min&&it->second.size<=max;};
        if(expanded>128ULL*1024*1024||!required("home.tga",18,8*1024*1024)||!required("cacert.pem",1024,1024*1024)||!required("menupreview/infinity-preview.txt",24,64))return reject("Recursos obrigatorios da dashboard ausentes.");
        for(const auto& entry:nodes){const auto& key=entry.first;
            if(key=="default.xex"||key=="home.tga"||key=="cacert.pem"||key=="leia-me.txt"||key=="menupreview"||key=="menupreview/infinity-preview.txt"||key=="menupreview/userfiles"||key=="licenses"||key.compare(0,9,"licenses/")==0)continue;
            return reject("Arquivo inesperado no pacote de atualizacao.");
        }
    }
    const unsigned long long total=item.bytes*2+expanded;
    if(!digest(r,item,progress,0,total))return reject("SHA-256 divergente ou cancelado.");
    const auto capacity=StorageInfo::Read(personal);
    if(!capacity.available||capacity.writableBytes<expanded+nodes.size()*65536ULL+16ULL*1024*1024)
        return reject("Espaco livre insuficiente.");
    const std::string category=StoreInstallPath::Category(item),parent=FileWorkspace::Join(personal,category);
    if(!ws.CanWrite(parent)&&!ws.Create(personal,category))return reject("Categoria indisponivel.");
    std::string stage;
    if(!ws.BeginDiscStage(parent,item.id,stage))return reject("Destino ja existe ou indisponivel.");
    std::vector<std::string> made;
    auto fail=[&](const char* reason){
        for(auto i=made.rbegin();i!=made.rend();++i){std::error_code ec;fs::remove(*i,ec);}
        std::error_code ec;fs::remove(stage,ec);error=reason;return false;
    };
    unsigned long long done=item.bytes;
    for(const auto& entry:nodes){
        const auto& node=entry.second;const std::string path=FileWorkspace::Join(stage,node.name);
        if(path.size()>240||!progress(done,total))return fail("Caminho invalido ou cancelado.");
        if(node.directory){
            std::error_code ec;if(!fs::create_directory(path,ec)||ec)return fail("Falha ao criar pasta.");
            made.push_back(path);continue;
        }
        const int fd=exclusive(path);
        if(fd<0)return fail("Falha ao criar arquivo exclusivo.");
        made.push_back(path);Output out{fd,node.size,0,done,total,progress};
        const bool extracted=mz_zip_reader_extract_to_callback(&a.zip,node.index,writeChunk,&out,0)!=0;
        const bool synced=fsync(fd)==0;const bool closed=close(fd)==0;
        if(!extracted||out.done!=node.size||!synced||!closed)return fail("Extracao/CRC/escrita invalida ou cancelada.");
        done+=node.size;
    }
    if(!digest(r,item,progress,done,total))return fail("Pacote alterado durante extracao ou cancelado.");
    {std::ifstream xex(FileWorkspace::Join(stage,"default.xex").c_str(),std::ios::binary);
     if(!XexLaunchCheck::Header(xex))return fail("default.xex invalido.");}
    if(dashboard){
        XboxExecutionInfo info{};
        if(!ContentCatalog::ReadExecutionInfo(FileWorkspace::Join(stage,"default.xex"),info)||info.titleId!=0x494E4633u)return fail("Title ID da dashboard divergente.");
        std::ifstream marker(FileWorkspace::Join(FileWorkspace::Join(stage,"MenuPreview"),"infinity-preview.txt").c_str());std::string line,extra;
        if(!std::getline(marker,line)||line!="INFINITY360-XSDK-PREVIEW"||std::getline(marker,extra))return fail("Marcador da dashboard invalido.");
        std::ifstream atlas(FileWorkspace::Join(stage,"home.tga").c_str(),std::ios::binary);unsigned char header[18];
        if(!atlas.read(reinterpret_cast<char*>(header),18)||header[1]!=0||header[2]!=10||header[16]!=32)return fail("Atlas TGA invalido.");
    }
    const std::string receipt=FileWorkspace::Join(stage,"infinity-install.txt");
    int fd=exclusive(receipt);
    if(fd<0)return fail("Falha ao criar registro de instalacao.");
    made.push_back(receipt);
    const std::string data=item.sha256+"\t"+std::to_string(xexSize)+"\n";
    const bool saved=write(fd,data.data(),data.size())==static_cast<ssize_t>(data.size());
    const bool synced=fsync(fd)==0;const bool closed=close(fd)==0;
    if(!saved||!synced||!closed)return fail("Falha ao gravar registro.");
    if(!ws.PublishDiscStage(parent,stage,item.id))return fail("Falha ao publicar sem sobrescrever.");
    return true;
}

bool StoreInstallManaged(const FileWorkspace& ws,const StoreItem& item,const std::string& root,
    const Progress& progress,std::string& error){
    if(item.category=="dashboard")return StoreInstallPackage(ws,item,root,progress,error);
    StoreCatalog catalog(root);
    if(catalog.Installed(item)){error="Versao atual ja instalada.";return false;}
    std::string saved;
    if(catalog.InstalledFolder(item)){
        // Verify the new package before displacing an existing application.
        const auto source=FileWorkspace::Join(FileWorkspace::Join(StoreStorage::Personal(root,item.storage),"Store"),item.id+".pkg");
        if(!catalog.Staged(item)||!ws.CanReadPersonalFile(source)||item.bytes>MaxPackage){error="Novo pacote indisponivel.";return false;}
        Reader r{std::ifstream(source.c_str(),std::ios::binary),item.bytes};
        if(!r.input||!digest(r,item,progress,0,item.bytes)){error="Novo pacote divergente ou cancelado.";return false;}
        if(!catalog.ArchiveInstalled(item,saved)){error="Backup da versao anterior recusado.";return false;}
    }
    if(StoreInstallPackage(ws,item,root,progress,error))return true;
    if(!saved.empty()){
        // Restore precisely the directory displaced by this update.
        const auto active=StoreInstallPath::Folder(root,item);
        if(ws.MovePersonalExclusive(saved,active,true))error+=" | Versao anterior restaurada.";
        else error+=" | Backup preservado em StoreBackups; restauracao manual.";
    }
    return false;
}
