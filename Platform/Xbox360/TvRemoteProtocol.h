#pragma once
#include <string>
namespace TvRemote {
enum class Key { None,Up,Down,Left,Right,Accept,Back,Mute,Quality,Quieter,Louder,Channels,Guide,Favorite,Refresh,PageUp,PageDown,Status,Snapshot,Games,TvCatalog,Tune };
inline bool ValidSecret(const std::string& s){
    if(s.size()!=32)return false;
    for(char c:s)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    return true;
}
inline bool SameSecret(const std::string& a,const std::string& b){
    if(a.size()!=32||b.size()!=32)return false;
    unsigned diff=0;for(size_t i=0;i<32;++i)diff|=static_cast<unsigned char>(a[i]^b[i]);return diff==0;
}
inline Key Command(const std::string& s){
    const char* names[]={"up","down","left","right","accept","back","mute","quality","quieter","louder","channels","guide","favorite","refresh","pageup","pagedown","status","snapshot"};
    for(unsigned i=0;i<18;++i)if(s==names[i])return static_cast<Key>(i+1);
    return Key::None;
}
struct Request {bool page=false;Key key=Key::None;unsigned offset=0;std::string query;};
inline bool ValidChannelId(const std::string& id){
    if(id.empty()||id.size()>20||id[0]=='0')return false;
    for(char c:id)if(c<'0'||c>'9')return false;
    return id.size()<20||id<="18446744073709551615";
}
inline Request CatalogRequest(const std::string& command){
    const bool games=command.compare(0,6,"games/")==0,tv=command.compare(0,3,"tv/")==0;
    if(!games&&!tv)return {};
    const size_t start=games?6:3,sep=command.find('/',start);
    if(sep==std::string::npos||sep==start||sep-start>6)return {};
    unsigned offset=0;
    for(size_t i=start;i<sep;++i){char c=command[i];if(c<'0'||c>'9')return {};offset=offset*10+unsigned(c-'0');}
    const auto encoded=command.substr(sep+1);if(encoded.size()>128||encoded.size()%2)return {};
    std::string query;
    auto digit=[](char c)->int{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
    for(size_t i=0;i<encoded.size();i+=2){const int a=digit(encoded[i]),b=digit(encoded[i+1]);if(a<0||b<0)return {};const unsigned char ch=a*16+b;if(ch<32||ch==127)return {};query+=char(ch);}
    return {false,games?Key::Games:Key::TvCatalog,offset,query};
}
inline Request Parse(const std::string& request,const std::string& host,const std::string& secret){
    if(request.size()>2048||!ValidSecret(secret))return {};
    const auto first=request.find("\r\n"),end=request.find("\r\n\r\n");
    if(first==std::string::npos||end==std::string::npos||end+4!=request.size())return {};
    const auto line=request.substr(0,first);
    std::string seenHost,token;bool hasHost=false,hasToken=false,hasLength=false;
    for(size_t pos=first+2;pos<end;){
        const auto stop=request.find("\r\n",pos);if(stop==std::string::npos||stop>end)return {};
        const auto colon=request.find(':',pos);if(colon==std::string::npos||colon>=stop)return {};
        auto name=request.substr(pos,colon-pos),value=request.substr(colon+1,stop-colon-1);
        for(char& c:name)if(c>='A'&&c<='Z')c+=32;
        const auto start=value.find_first_not_of(" \t"),last=value.find_last_not_of(" \t");
        value=start==std::string::npos?"":value.substr(start,last-start+1);
        if(name=="host"){if(hasHost)return {};hasHost=true;seenHost=value;}
        else if(name=="x-infinity-key"){if(hasToken)return {};hasToken=true;token=value;}
        else if(name=="content-length"){if(hasLength||value!="0")return {};hasLength=true;}
        else if(name=="transfer-encoding")return {};
        pos=stop+2;
    }
    if(!hasHost||seenHost!=host)return {};
    if(line=="GET / HTTP/1.1")return {true,Key::None};
    const std::string prefix="POST /cmd/",suffix=" HTTP/1.1";
    if(line.size()<=prefix.size()+suffix.size()||line.compare(0,prefix.size(),prefix)||line.compare(line.size()-suffix.size(),suffix.size(),suffix)||!SameSecret(token,secret))return {};
    const auto command=line.substr(prefix.size(),line.size()-prefix.size()-suffix.size());
    if(command.compare(0,5,"tune/")==0){const auto id=command.substr(5);return ValidChannelId(id)?Request{false,Key::Tune,0,id}:Request{};}
    const auto catalog=CatalogRequest(command);if(catalog.key!=Key::None)return catalog;
    return {false,Command(command)};
}
inline const char* Page(){return R"HTML(<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>INFINITY 360 IPTV</title><style>
*{box-sizing:border-box}body{margin:0;background:#101c16;color:#eef6ef;font:16px system-ui;padding:20px}main{max-width:440px;margin:auto}h1{color:#a8ff00;font-size:24px}input,button{font:inherit;border:1px solid #47634f;border-radius:12px;background:#20382b;color:white;padding:14px;min-height:52px}input{width:100%;margin:8px 0 16px}section{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:12px 0}button:active{background:#426b36}p{color:#bbcec0;font-size:14px}button{touch-action:manipulation}
</style><main><h1>INFINITY 360</h1><p>Dashboard &amp; IPTV &middot; Controle local</p><label for="key">Chave de pareamento</label><input id="key" type="password" maxlength="32" autocomplete="off" autocapitalize="none" spellcheck="false"><section>
<button data-key="channels">Canais</button><button data-key="up">↑</button><button data-key="quality">Qualidade</button>
<button data-key="left">←</button><button data-key="accept">A / OK</button><button data-key="right">→</button>
<button data-key="quieter">Vol −</button><button data-key="down">↓</button><button data-key="louder">Vol +</button>
<button data-key="mute">Mudo</button><button data-key="back">Voltar</button><button data-key="guide">Guide</button><button data-key="favorite">Favorito</button><button data-key="refresh">Atualizar</button><button data-key="status">Testar chave</button></section><p id="status" role="status">Digite a chave configurada no console.</p><p>HTTP local sem criptografia. Use somente uma rede confiável.</p></main><script>
let busy=false;document.querySelectorAll('button[data-key]').forEach(b=>b.onclick=async()=>{if(busy)return;const key=document.getElementById('key').value,status=document.getElementById('status');if(!/^[a-f0-9]{32}$/.test(key)){status.textContent='Chave inválida.';return}busy=true;try{const r=await fetch('/cmd/'+b.dataset.key,{method:'POST',headers:{'X-Infinity-Key':key},cache:'no-store'});status.textContent=r.ok?'Comando enviado.':'Comando recusado; confira a chave.'}catch{status.textContent='Console indisponível ou reprodução encerrada.'}finally{busy=false}});</script></html>)HTML";}
}
