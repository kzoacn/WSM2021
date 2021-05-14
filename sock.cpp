#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <iostream> // For cout
#include <unistd.h> // For read
#include <iostream>
#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <fstream>
#include "pugixml.hpp"
#include <map>
#include <cmath>
#include <vector>
using namespace std;
using namespace pugi;



//#define RANKING_JACCARD
//#define RANKING_LOGFREQUENCY
//#define RANKING_TFIDF
//#define RANKING_TFIDFV2
#define RANKING_TFIDFV3


const int buf_len=1024*(1<<20);//512MB

char buf[buf_len];



namespace String{

    void trim(string &s){ 
        string ns;
        for (int j = 0; j < s.length(); j++)
            if (isalnum(s[j])||isblank(s[j]))
                ns += s[j];
        s = ns;
    }

    vector<string> split(const string &s){
        vector<string>res;
        res.push_back("");
        for(int i=0;i<s.length();i++){
            if(isblank(s[i])){
                if(res.back()!="")
                    res.push_back("");
            }else{
                res.back()+=s[i];
            }
        }
        if(res.back()=="")res.pop_back();
        return res;
    }

    void to_lower(string &s){
        for(auto &c:s)
            c=tolower(c);
    }
    map<string,string>LM;
    void init(){
        LM["am"]="be";
        LM["is"]="be";
        LM["are"]="be";
        LM["was"]="be";
        LM["were"]="be";
        LM["being"]="be";
    }
    void lemma(string &s){
        if(LM.count(s))
            s=LM[s];
    }
    void stem(string &s){
        if(s.length()>=4&&s.substr(s.length()-4,4)=="sses"){
            s.pop_back();
            s.pop_back();
        }
        if(s.length()>=4&&s.substr(s.length()-3,3)=="ies"){
            s.pop_back();
            s.pop_back();
        }
        if(s.length()>=2&&s.substr(s.length()-1,1)=="s"){
            s.pop_back(); 
        }
        if(s.length()>=6&&s.substr(s.length()-5,5)=="ement"){
            s.pop_back();
            s.pop_back();
            s.pop_back();
            s.pop_back();
            s.pop_back();
        }
    }
    vector<string> permute(string s){
        vector<string>all;
        for(int i=0;i<s.length();i++){
            string a=s.substr(0,s.length()-i);
            string b=s.substr(s.length()-i,i);
            all.push_back(b+a); 
        } 
        return all;
    }
    map<string,int>M;
    map<int,string>rM;
    int total=0;

    void modify(string &s){
        to_lower(s);
        lemma(s);
        stem(s);
    }

    int token(string s){// input is plain
        modify(s);
        s+="$";
        if(M.count(s))
            return M[s];
        for(auto s2:permute(s)){
            M[s2]=total;
        }
        rM[total]=s;
        total++;
        return M[s];
    }
    int get_token(string s){
        if(M.count(s))
            return M[s];
        return M.size();
    }
};


struct Page{
    string url;
    string title;
    string abstract;
    vector<string>links;
    vector<string>anchors;
    vector<int>tokens;
};
vector<Page>pages;
vector<int>token_to_list;
vector<vector<int> >token_list;

void get_list(int id,vector<int>&res){
    res.clear();
    if(id<token_list.size())
        res=token_list[id];
}

vector<int> vec_intersect(const vector<int>&A,const vector<int> &B){
    vector<int>C;
    std::set_intersection(std::begin(A), std::end(A), std::begin(B), std::end(B),std::inserter(C, std::begin(C)));
    return C;
}
vector<int> vec_union(const vector<int>&A,const vector<int> &B){
    vector<int>C;
    std::set_union(std::begin(A), std::end(A), std::begin(B), std::end(B),std::inserter(C, std::begin(C)));
    return C;
}

void add_pages(string filename){ 
    xml_document  doc;
    doc.load_file(filename.c_str());

    int cnt=0;
    for (auto node : doc.child("feed").children())
    {
        Page page;
        page.url=node.child("url").text().as_string();  

        page.title=node.child("title").text().as_string();
        String::trim(page.title);

        page.abstract=node.child("abstract").text().as_string();
        String::trim(page.abstract); 

        for(auto s:String::split(page.title))
            page.tokens.push_back(String::token(s));
        for(auto s:String::split(page.abstract))
            page.tokens.push_back(String::token(s)); 
        
        sort(page.tokens.begin(),page.tokens.end());
        page.tokens.erase(unique(page.tokens.begin(),page.tokens.end()),page.tokens.end());
        pages.emplace_back(page);
    }
}

void generate_tokens(){
    token_list.resize(String::M.size());
    int id=0;
    for(auto &page:pages){
        for(auto t:page.tokens)
            token_list[t].push_back(id);
        id++;
    }
}

float score(vector<int>&tokens,int pageid){
#ifdef RANKING_JACCARD
    //Case 1: JACCARD :  |A intersect B| / |A union B|
    int size1=vec_intersect(tokens,pages[pageid].tokens).size();
    int size2=vec_union(tokens,pages[pageid].tokens).size();
    return 1.0*size1/size2;
#endif

#ifdef RANKING_LOGFREQUENCY
    //Case 2: LOG-FREQUENCY
    float sum=0;
    auto tmp=vec_intersect(tokens,pages[pageid].tokens);
    for(auto t:tmp){
        int w=1;
        for(auto x:pages[pageid].tokens)
            w+=(x==t);
        sum+=(1+log(w)); 
    }
    return sum;
#endif


#ifdef RANKING_TFIDF
    float sum=0;
    
    auto tmp=vec_intersect(tokens,pages[pageid].tokens);
    for(auto t:tmp){
        float idf=log(pages.size())-log(token_list[t].size());
        int w=1;
        for(auto x:pages[pageid].tokens)
            w+=(x==t);
        sum+=(1+log(w))*idf; 
    } 
    return sum;    
#endif

#ifdef RANKING_TFIDFV2
    float sum=0;
    
    auto tmp=vec_intersect(tokens,pages[pageid].tokens);
    for(auto t:tmp){
        float idf=max(log(pages.size()-token_list[t].size())-log(token_list[t].size()),0.0);
        int w=1;
        for(auto x:pages[pageid].tokens)
            w+=(x==t);
        sum+=(1+log(w))*idf; 
    } 
    return sum;    

#endif
#ifdef RANKING_TFIDFV3
    float sum=0;
    
    auto tmp=vec_intersect(tokens,pages[pageid].tokens);
    for(auto t:tmp){
        float idf=log(pages.size())-log(token_list[t].size());
        int w=0;
        for(auto x:pages[pageid].tokens)
            w+=(x==t);
        sum+=(w>0?1:0)*idf; 
    } 
    return sum;    

#endif
}

vector<int> get_match(string q){ 

    vector<int>res;
    int num=0,pos=0;
    for(int i=0;i<q.length();i++)
        if(q[i]=='*'){
            num++;
            pos=i;
        }

    if(num>1){
        cerr<<"does not support more that one wildcard"<<endl;
        return res;
    }
    
    if(num==0){
        String::modify(q);
        res.push_back(String::get_token(q+'$'));
        //cerr<<"return "<<res.back()<<endl;
        return res;
    }
    string a=q.substr(0,pos);
    string b=q.substr(pos+1,q.length()-(pos+1));
    String::to_lower(a);
    String::to_lower(b);
    q=b+"$"+a;
    string prefix=q;
    //cerr<<"prefix "<<prefix<<endl;
    for(auto it=String::M.lower_bound(prefix);it!=String::M.end();it++){
        string t=it->first;
        //cerr<<"it "<<t<<endl;
        if(t.length()<prefix.length() || t.substr(0,prefix.length())!=prefix)
            break;
        res.push_back(it->second);
    }

    return res;
}

void query(vector<string>querys,vector<string> &result){
    vector<int>list,list2,tokens,tmp;
    bool first=true;
    for(auto &q:querys){
        list2.clear();
        for(auto id:get_match(q)){
            if(id>=token_list.size())
                continue;
            tokens.push_back(id);
            get_list(id,tmp);
            list2=vec_union(list2,tmp);
        }
        if(first){
            list=list2;
            first=false;
        }else{
            list=vec_intersect(list,list2);
        }
    }

    vector<pair<float,int> >rankings;
    for(auto x:list)
        rankings.push_back(make_pair(score(tokens,x),x));
    sort(rankings.begin(),rankings.end(),greater<pair<float,int> >());
    for(auto x: rankings)
        result.push_back(pages[x.second].url);
    
}
 

string run(vector<string>querys){ 
    vector<string>res;
    double st=clock();
    query(querys,res);
    string ans;
    double t=(clock()-st)/CLOCKS_PER_SEC;
    ans+="Return "+to_string(res.size())+" pages in "+to_string(t)+" sec<br>";
    for(auto s: res)
      ans+=s+"<br>"; 
    return ans;
}

string query(string qq){
  return  run(String::split(qq)); 
}

const int buffer_len=1000;
bool parse(string buf,string &ans){
  int pos=buf.find("querys=");
  if(pos==buf.npos)
    return false;
  ans="";
  for(int i=pos+7;!isblank(buf[i]);i++){
    char c=buf[i];
    if(c=='+')c=' ';
    ans+=c;
  }

  return true;
}

int main() { 

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);  
  sockaddr_in sockaddr;
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = INADDR_ANY;
  sockaddr.sin_port = htons(8080);  
  if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {perror("failed");}



  cerr<<"Loading"<<endl;
  ifstream flist("xml_list");
    string filename;
  while(flist>>filename){ 
    cerr<<"Adding "<<filename<<endl;
    add_pages(filename);
  }
    generate_tokens();

    cerr<<"num of tokens = "<<String::M.size()<<endl;
    cerr<<"num of pages = "<<pages.size() <<endl;
    cerr<<"finish"<<endl;
 
while(1){ 
  if (listen(sockfd, 1) < 0){perror("failed");return -1;} 
  auto addrlen = sizeof(sockaddr);
  int connection = accept(sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
  if (connection < 0){perror("failed");}
 
  char buffer[buffer_len];
  memset(buffer,0,sizeof(buffer));
  auto bytesRead = read(connection, buffer, buffer_len);
  std::cout << buffer;
  string ans;
  
  std::string html;

  if(parse(buffer,ans)){
    html="<!DOCTYPE html><html><body>"+query(ans)+"</body></html>";    
  }else{

 
html="<!DOCTYPE html><html><body>"
"<form action=\"/search\">"
"Wiki Search:<br>"
"<input type=\"text\" name=\"querys\">"
"<br>"
"<input type=\"submit\" value=\"Submit\">"
"</form>"
"</body>"
"</html>";    
  }



  std::string response = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
  response+=html;
    cerr<<response<<endl;
  send(connection, response.c_str(), response.size(), 0);
  close(connection); 
} 
}
