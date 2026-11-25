#pragma once
// Optional libsmb2 adapter. No SMB writes, anonymous access or unsigned fallback.
#include "LanPeerProbe.h"
#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#include <poll.h>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

class SmbReadSession {
public:
    enum State { Idle, Connecting, Opening, Listing, Ready, Failed, Cancelled };
    struct Entry { std::string name; uint64_t size; bool directory; };
    static bool Field(const std::string& s,size_t max,bool empty=false){
        if(s.size()>max||(!empty&&s.empty()))return false;
        for(unsigned char c:s)if(c<32||c==127)return false;
        return true;
    }
    static bool Share(const std::string& s){
        return Field(s,80)&&s!="."&&s!=".."&&s.find_first_of("/\\:*?<>|\"")==std::string::npos;
    }
    static bool Name(const std::string& s){
        return Field(s,255)&&s!="."&&s!=".."&&s.find_first_of("/\\:")==std::string::npos;
    }
    static bool Path(const std::string& s){
        if(s.empty())return true;
        if(s.size()>1024)return false;
        size_t pos=0;
        while(pos<s.size()){
            const auto end=s.find('/',pos);
            if(!Name(s.substr(pos,end==std::string::npos?end:end-pos)))return false;
            if(end==std::string::npos)return true;
            pos=end+1;
        }
        return false;
    }
    SmbReadSession()=default;
    ~SmbReadSession(){Close();}
    SmbReadSession(const SmbReadSession&)=delete;
    SmbReadSession& operator=(const SmbReadSession&)=delete;
    State Status()const{return state_;}
    bool Busy()const{return state_==Connecting||state_==Opening||state_==Listing;}
    const std::vector<Entry>& Entries()const{return entries_;}
    const std::string& Label()const{return label_;}
    bool Truncated()const{return truncated_;}
    unsigned Dialect()const{return dialect_;}
    void Cancel(){Close();entries_.clear();state_=Cancelled;label_="Sessao cancelada.";}
    bool Start(const std::string& ip,const std::string& share,const std::string& user,
               const std::string& domain,const std::string& password){
        Close();entries_.clear();truncated_=false;dialect_=0;
        if(!LanPeerProbe::Private(ip)||!Share(share)||!Field(user,128)||
           !Field(domain,128,true)||!Field(password,256)||user=="guest"||user=="Guest"){
            Fail("Servidor privado, compartilhamento e credenciais explicitas obrigatorios.");return false;
        }
        ctx_=smb2_init_context();
        if(!ctx_){Fail("Provider SMB ou entropia indisponivel.");return false;}
        smb2_set_version(ctx_,SMB2_VERSION_0311);
        smb2_set_authentication(ctx_,SMB2_SEC_NTLMSSP);
        smb2_set_sign(ctx_,1);
        smb2_set_seal(ctx_,1);
        smb2_set_timeout(ctx_,10);
        smb2_set_user(ctx_,user.c_str());
        smb2_set_domain(ctx_,domain.c_str());
        smb2_set_password(ctx_,password.c_str());
        state_=Connecting;label_="Conectando SMB 3.1.1 com assinatura e criptografia.";start_=Now();
        if(smb2_connect_share_async(ctx_,ip.c_str(),share.c_str(),user.c_str(),Connected,this)<0){
            Fail("Nao foi possivel iniciar sessao SMB.");return false;
        }
        return true;
    }
    bool Open(const std::string& path){
        if(state_!=Ready||!ctx_||!Path(path))return false;
        entries_.clear();truncated_=false;
        state_=Opening;start_=Now();label_="Consultando diretorio.";
        if(smb2_opendir_async(ctx_,path.c_str(),Opened,this)<0){Fail("Consulta de diretorio recusada.");return false;}
        return true;
    }
    bool Tick(){
        if(state_==Failed&&ctx_){Close();return true;}
        if(!Busy())return false;
        const auto now=Now();
        if(now<start_||now-start_>=15000){Fail("Prazo da operacao SMB excedido.");return true;}
        if(state_==Listing){
            // Bound per frame and number of entries retained by the UI.
            // libsmb2 may buffer more entries internally before the callback.
            for(unsigned i=0;i<16;++i){
                auto* row=smb2_readdir(ctx_,dir_);
                if(!row||entries_.size()>=512){
                    truncated_=row!=nullptr;smb2_closedir(ctx_,dir_);dir_=nullptr;
                    state_=Ready;label_=truncated_?"Listagem limitada a 512 itens.":"Diretorio consultado.";return true;
                }
                if(!row->name)continue;
                const std::string name(row->name);
                if(!Name(name))continue;
                if(row->st.smb2_type!=SMB2_TYPE_DIRECTORY&&row->st.smb2_type!=SMB2_TYPE_FILE)continue;
                entries_.push_back({name,row->st.smb2_size,row->st.smb2_type==SMB2_TYPE_DIRECTORY});
            }
            return true;
        }
        pollfd p{};p.fd=smb2_get_fd(ctx_);p.events=static_cast<short>(smb2_which_events(ctx_));
        if(p.fd<0){Fail("Socket SMB indisponivel.");return true;}
        const int rc=poll(&p,1,0);
        if(rc<0){if(errno==EINTR)return false;Fail("Falha de socket SMB.");return true;}
        const auto before=state_;
        // Service the library's timeout bookkeeping even without an event.
        if(smb2_service(ctx_,rc?p.revents:0)<0){Fail("Sessao SMB recusada ou interrompida.");return true;}
        if(state_==Failed)Close();
        return state_!=before;
    }
private:
    static uint64_t Now(){return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
    static void Connected(smb2_context* ctx,int status,void*,void* opaque){
        auto& self=*static_cast<SmbReadSession*>(opaque);
        if(self.state_!=Connecting)return;
        if(status||smb2_get_dialect(ctx)!=0x0311){
            self.state_=Failed;self.label_="Login ou protecao SMB 3.1.1 recusados.";return;
        }
        self.dialect_=smb2_get_dialect(ctx);
        self.state_=Ready;self.label_="Sessao estabelecida pelo provider.";
    }
    static void Opened(smb2_context*,int status,void* data,void* opaque){
        auto& self=*static_cast<SmbReadSession*>(opaque);
        if(self.state_!=Opening)return;
        if(status||!data){self.state_=Failed;self.label_="Diretorio remoto indisponivel.";return;}
        self.dir_=static_cast<smb2dir*>(data);self.state_=Listing;
    }
    void Close(){
        if(ctx_){
            // Prevent cancellation callbacks from reopening operations.
            state_=Idle;
            if(dir_){smb2_closedir(ctx_,dir_);dir_=nullptr;}
            auto* old=ctx_;ctx_=nullptr;smb2_destroy_context(old);
        }
    }
    void Fail(const char* label){Close();entries_.clear();state_=Failed;label_=label;}
    smb2_context* ctx_=nullptr;smb2dir* dir_=nullptr;
    State state_=Idle;uint64_t start_=0;unsigned dialect_=0;bool truncated_=false;
    std::string label_="Sessao nao iniciada.";
    std::vector<Entry> entries_;
};
