#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <utility>
#include <cstdlib>

namespace local {
using Fields=std::vector<std::pair<std::string,std::string>>;
inline std::string get(const Fields &f,const std::string &key){for(const auto &x:f)if(x.first==key)return x.second;return "";}
inline bool equalSecret(const std::string &a,const std::string &b){
  if(a.size()!=b.size()||a.empty())return false;
  unsigned difference=0;for(size_t i=0;i<a.size();++i)difference|=unsigned(a[i]^b[i]);return difference==0;
}
inline bool number(const std::string &s,uint32_t &value){
  if(s.empty()||s.size()>10)return false;uint64_t n=0;
  for(char c:s){if(c<'0'||c>'9')return false;n=n*10+unsigned(c-'0');if(n>UINT32_MAX)return false;}
  value=uint32_t(n);return true;
}
inline bool identifier(const std::string &s){if(s.size()<8||s.size()>64)return false;for(char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;return true;}
inline int hex(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
inline bool decode(const std::string &s,std::string &out){
  out.clear();for(size_t i=0;i<s.size();++i){char c=s[i];if(c=='+')c=' ';else if(c=='%'){
    if(i+2>=s.size()||hex(s[i+1])<0||hex(s[i+2])<0)return false;
    c=char(hex(s[i+1])*16+hex(s[i+2]));i+=2;
  }if(static_cast<unsigned char>(c)<32||c==127)return false;out+=c;}return true;
}
inline bool form(const std::string &body,Fields &out){
  out.clear();size_t pos=0;while(pos<body.size()){
    size_t end=body.find('&',pos);if(end==std::string::npos)end=body.size();
    size_t eq=body.find('=',pos);if(eq==std::string::npos||eq>=end||out.size()>=16)return false;
    std::string key,value;if(!decode(body.substr(pos,eq-pos),key)||!decode(body.substr(eq+1,end-eq-1),value)||key.empty())return false;
    for(const auto &x:out)if(x.first==key)return false;out.push_back({key,value});pos=end+1;
  }return true;
}
inline std::string quote(const std::string &s){std::string out="\"";for(unsigned char c:s){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c>=32)out+=char(c);}return out+'"';}
// apClient is transport metadata, never parsed from headers or form fields.
struct Request {std::string method,path,bearer,body;bool apClient=false;};
struct Reply {int code;std::string body;};
enum class Role {None,Operator,Admin};
struct Access {
  std::string operatorToken,adminToken,pairCode;
  Role pairRole=Role::None;
  uint32_t pairAt=0;
  unsigned attempts=0;
  std::string used[32];size_t cursor=0;
  Role role(const std::string &token)const{
    if(equalSecret(token,adminToken))return Role::Admin;
    if(equalSecret(token,operatorToken))return Role::Operator;
    return Role::None;
  }
  void openPair(const std::string &code,Role r,uint32_t now){pairCode=code;pairRole=r;pairAt=now;attempts=0;}
  Role redeem(const std::string &code,uint32_t now){
    if(pairRole==Role::None||uint32_t(now-pairAt)>=120000||attempts>=5)return Role::None;
    ++attempts;if(!equalSecret(code,pairCode))return Role::None;
    Role r=pairRole;pairRole=Role::None;pairCode.clear();return r;
  }
  bool fresh(const Fields &fields,uint32_t boot){
    uint32_t supplied=0;std::string id=get(fields,"request_id");
    if(!number(get(fields,"boot_id"),supplied)||supplied!=boot||!identifier(id))return false;
    for(const auto &s:used)if(s==id)return false;
    used[cursor++%32]=id;return true;
  }
};

// Strict bounded HTTP/1.1 framing, independent of WiFi. No chunked bodies,
// duplicate headers, pipelining, query commands, or unbounded read loops.
struct HttpFrame {
  std::string bytes;size_t needed=0;bool header=false,failed=false,complete=false;
  Request request;
  void feed(char c){
    if(failed||complete)return;
    bytes+=c;if(bytes.size()>6144){failed=true;return;}
    if(!header&&bytes.size()>=4&&bytes.compare(bytes.size()-4,4,"\r\n\r\n")==0){
      size_t end=bytes.find("\r\n"),a=bytes.find(' '),b=bytes.find(' ',a+1);
      if(a==std::string::npos||b==std::string::npos||b>=end||bytes.substr(b+1,end-b-1)!="HTTP/1.1"){failed=true;return;}
      request.method=bytes.substr(0,a);request.path=bytes.substr(a+1,b-a-1);
      if(request.path.find('?')!=std::string::npos){failed=true;return;}
      Fields headers;size_t pos=end+2;uint32_t length=0;
      while(pos<bytes.size()-2){end=bytes.find("\r\n",pos);size_t sep=bytes.find(':',pos);
        if(sep==std::string::npos||sep>=end){failed=true;return;}
        std::string key=bytes.substr(pos,sep-pos);for(char &ch:key)if(ch>='A'&&ch<='Z')ch=char(ch+32);
        std::string value=bytes.substr(sep+1,end-sep-1);while(!value.empty()&&value.front()==' ')value.erase(0,1);
        for(const auto &x:headers)if(x.first==key){failed=true;return;}
        headers.push_back({key,value});pos=end+2;
      }
      const auto len=get(headers,"content-length");
      if(!get(headers,"transfer-encoding").empty()||(!len.empty()&&(!number(len,length)||length>4096))||
          (request.method=="POST"&&(len.empty()||get(headers,"content-type")!="application/x-www-form-urlencoded"))){failed=true;return;}
      const auto auth=get(headers,"authorization");if(auth.compare(0,7,"Bearer ")==0)request.bearer=auth.substr(7);
      header=true;needed=bytes.size()+length;
    }
    if(header&&bytes.size()==needed){complete=true;size_t start=bytes.find("\r\n\r\n")+4;request.body=bytes.substr(start);}
    if(!header&&bytes.size()>2048)failed=true;
  }
};
} // namespace local
