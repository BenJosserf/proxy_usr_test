
#include "proxy_url_extractor.h"
#include <fstream>
#include <vector>
#include "tokener.h"
#include <stdio.h>

using namespace std;
namespace qh
{

    namespace {

        template< class _StringVector, 
        class StringType,
        class _DelimType> 
            inline void StringSplit(  
            const StringType& str, 
            const _DelimType& delims, 
            unsigned int maxSplits, 
            _StringVector& ret)
        {
            unsigned int numSplits = 0;

            // Use STL methods
            size_t start, pos;
            start = 0;

            do
            {
                pos = str.find_first_of( delims, start );

                if ( pos == start )
                {
                    ret.push_back(StringType());
                    start = pos + 1;
                }
                else if ( pos == StringType::npos || ( maxSplits && numSplits + 1== maxSplits ) )
                {
                    // Copy the rest of the string
                    ret.push_back(StringType());
                    *(ret.rbegin()) = StringType(str.data() + start, str.size() - start);
                    break;
                }
                else
                {
                    // Copy up to delimiter
                    //ret.push_back( str.substr( start, pos - start ) );
                    ret.push_back(StringType());
                    *(ret.rbegin()) = StringType(str.data() + start, pos - start);
                    start = pos + 1;
                }

                ++numSplits;

            }
            while ( pos != StringType::npos );
        }
    }

    ProxyURLExtractor::ProxyURLExtractor()
    {
    }

    bool ProxyURLExtractor::Initialize( const std::string& param_keys_path )
    {
        std::ifstream ifs;
        ifs.open(param_keys_path.data(), std::fstream::in);
        typedef std::vector<std::string> stringvector;
        stringvector keysvect;
        
        while (!ifs.eof()) {
            std::string line;
            getline(ifs, line);
            if (ifs.fail() && !ifs.eof()) {
                fprintf(stderr, "SubUrlExtractor::LoadParamKeysFile readfile_error=[%s] error!!", param_keys_path.data());
                ifs.close();
                return false;
            }
            if (line.empty()) continue;

            keysvect.clear();
            StringSplit(line, ",", static_cast<unsigned int>(-1), keysvect);
            assert(keysvect.size() >= 1);
            keys_set_.insert(keysvect.begin(), keysvect.end());
            keys_set_.erase("");
        }

        ifs.close();

        return true;
    }

    std::string ProxyURLExtractor::Extract( const std::string& raw_url )
    {
        std::string sub_url;
        ProxyURLExtractor::Extract(keys_set_, raw_url, sub_url);
        return sub_url;
    }
    //time complexity o(k(nm)){k:numbers of expected keys,n:numbers of raw_urls
    // m:numbers of contanning keys in raw_url}
    // space complexity o(n+m+k){n:Tokener size,m:keyset size,k:size of source urls}
    void ProxyURLExtractor::Extract( const KeyItems& keys, const std::string& raw_url, std::string& sub_url )
    {
#if 1
        //TODO 请面试者在这里添加自己的代码实现以完成所需功能
        //soving '???'
        Tokener token(raw_url);
        token.skipTo('?');
        token.next();
        while(token.current() == '?'){
        token.next();
        }
        std::string key;
        //back point after check
        char  temChar='?';
        while(!token.isEnd()){
        //soving key-style like "?&key=.."
        char currChar=token.current();
          if (currChar=='&'){continue;}
          
          std::string temStr=token.nextString('&');
          //Don't point to next '&',point the previous position
          token.back();
          //back to the start position behaind the  previous '?' or '&'
          token.skipBackTo(temChar);
          //at the last key-value substring position
          if (temStr==""){
            temStr=token.nextString();
            token.skipBackTo(temChar);
            //soving '&key&'  
             if(temStr.find('=',0)==string::npos){
             key=token.nextString();
            }
          }
          //at key-value substrings except the last one
          else{
            //soving '&key&'
            if(temStr.find('=',0)==string::npos){
            key=token.nextString('&');
            }
         }
          
         //fprintf(stderr,"temStr:[%s]\n",temStr.data());
          if(temStr.find('=',0)!=string::npos){
            //soving '&key=[.]*&'
            key=token.nextString('=');
          }
         //fprintf(stderr,"key:[%s]\n",key.data());
            //soving key is included by keys
          if(keys.find(key)!=keys.end()){
            //soving '&key&'
            if(temStr.find('=',0)==string::npos){
            sub_url="";
            break;
          }
           else{
            //soving '&key=&'
            if(token.current()=='&'){
             sub_url="";
             break;
            }
           //soving '&key=[.]+[&]*'
            int nreadable = token.getReadableSize();
            const char* curpos = token.getCurReadPos();
            sub_url = token.nextString('&');
           //soving '&key=[.]+[^&]'
           if (sub_url.empty() && nreadable > 0) {
             assert(curpos);
             sub_url.assign(curpos, nreadable);
             curpos=NULL;
           }
          }
         //fprintf(stderr,"sub_url:[%s]\n",sub_url.data());
        } 
        //skip to next key-value substring
        if(token.skipTo('&')!=0){
        temChar='&';
        token.next();//skip one char : '&'
        }
        //already last key-value substring,set point to '\0'
        else{
        while(!token.isEnd()){
         token.next(); 
         }
        }
  }
//reset tokener
token.reset("",0);
#else
        //这是一份参考实现，但在特殊情况下工作不能符合预期
        Tokener token(raw_url);
        token.skipTo('?');
        token.next(); //skip one char : '?' 
        std::string key;
        while (!token.isEnd()) {
            key = token.nextString('=');
            if (keys.find(key) != keys.end()) {
                const char* curpos = token.getCurReadPos();
                int nreadable = token.getReadableSize();

                /**
                * case 1: 
                *  raw_url="http://www.microsofttranslator.com/bv.aspx?from=&to=zh-chs&a=http://hnujug.com/&xx=yy"
                *  sub_url="http://hnujug.com/"
                */
                sub_url = token.nextString('&');

                if (sub_url.empty() && nreadable > 0) {
                    /**
                    * case 2: 
                    * raw_url="http://www.microsofttranslator.com/bv.aspx?from=&to=zh-chs&a=http://hnujug.com/"
                    * sub_url="http://hnujug.com/"
                    */
                    assert(curpos);
                    sub_url.assign(curpos, nreadable);
                }
            }
            token.skipTo('&');
            token.next();//skip one char : '&'
        }
}
#endif
    }

    std::string ProxyURLExtractor::Extract( const KeyItems& keys, const std::string& raw_url )
    {
        std::string sub_url;
        ProxyURLExtractor::Extract(keys, raw_url, sub_url);
        return sub_url;
    }
}

