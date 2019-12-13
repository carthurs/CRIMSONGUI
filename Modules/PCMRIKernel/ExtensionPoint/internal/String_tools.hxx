#ifndef STRINGTOOLS_H_
#define STRINGTOOLS_H_


#include <string>
#include <vector>
#include <sstream>
#include <iostream>

namespace echo{
	namespace str {


	//std::string & removeExtension(std::string &input);


	inline std::vector< std::string> Tokenize(const std::string& str,const std::string& delimiters);

	inline std::string ExtractFolder(const std::string& str);

	template <class NumberType>	std::string toString (NumberType n)
	{
		std::ostringstream ss;
		ss << n;
		return ss.str();
	};


/**
* Extract folder with final slash
*/
std::string ExtractFolder(const std::string& str){
	std::string::size_type delimpos = 0;
	delimpos = str.find_last_of("/");

	return str.substr(0, delimpos+1);


}



std::vector< std::string> Tokenize(const std::string& str,const std::string& delimiters)
 	{
 	 std::vector< std::string> tokens;
 	 std::string::size_type delimPos = 0, tokenPos = 0, pos = 0;

 	 if(str.length()<1)  return tokens;
 	 while(1){
 	 	//std::cout << "string"<<std::endl<< str<<std::endl;
 	   delimPos = str.find_first_of(delimiters, pos);
 	   if (delimPos == pos){
 	   	pos = pos+1;
 	   	/// There are two delimiters in a row!
		continue;
 	   }

 	   tokenPos = str.find_first_not_of(delimiters, pos);
		//std::cout <<"   first pos "<< delimPos<<std::endl;
		//std::cout <<"   last pos "<< tokenPos<<std::endl;
 	   if(std::string::npos != delimPos){
 	     if(std::string::npos != tokenPos){
 	       if(tokenPos<delimPos){
 	         tokens.push_back(str.substr(pos,delimPos-pos));
 	       }else{
 	         tokens.push_back("");
 	       }
 	     }else{
 	       tokens.push_back("");
 	     }
 	     pos = std::max(delimPos+1, tokenPos+1);
 	   } else {
 	     if(std::string::npos != tokenPos){
 	       tokens.push_back(str.substr(pos));
 	     } else {
 	       tokens.push_back("");
 	     }
 	     break;
 	   }
 	 }
 	 return tokens;
 	}



	}
}

#endif /* STRINGTOOLS_H_ */

